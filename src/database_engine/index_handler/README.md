# B+ Tree Index Handler

## Overview
This module implements a **B+ Tree index** for efficient key-based record retrieval. The B+ tree is a balanced tree structure optimized for disk-based systems, where all data resides in leaf nodes and internal nodes serve as routing structures.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      IndexManager                                │
│              (Static Factory for Index Creation)                 │
├─────────────────────────────────────────────────────────────────┤
│  • create_index(filename, type, len)                             │
│  • destroy_index(filename)                                       │
│  • open_index(filename) → IndexHandle                            │
│  • get_index_name(table_name, col_idx) → string                  │
└──────────────────────────┬──────────────────────────────────────┘
                           │ creates/opens
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│                      IndexHandle                                 │
│              (B+ Tree Operations Interface)                      │
├─────────────────────────────────────────────────────────────────┤
│  Public API:                                                     │
│  • insert_entry(key, rid)    - Insert key → RID mapping         │
│  • delete_entry(key, rid)    - Remove key → RID mapping         │
│  • lower_bound(key) → IndexID - First key ≥ target              │
│  • upper_bound(key) → IndexID - First key > target              │
│  • leaf_begin() → IndexID     - Start of leaf linked list       │
│  • leaf_end() → IndexID       - End sentinel                    │
│  • get_value(IndexID) → RID   - Retrieve record location        │
└──────────────────────────┬──────────────────────────────────────┘
                           │ manages
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│                 B+ Tree Structure (Disk-Based)                   │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  Root Page (page 0)                                              │
│  ┌──────────────────────────────────────┐                       │
│  │ IndexFileHeader                       │                       │
│  │ • ColumnType key_type                 │                       │
│  │ • int key_len                         │                       │
│  │ • int root_node (page #)              │                       │
│  │ • int first_leaf (page #)             │                       │
│  └──────────────────────────────────────┘                       │
│                    │                                             │
│                    ↓                                             │
│  ┌─────────────────────────────────────────────────┐            │
│  │        Internal Nodes (Non-Leaf Pages)          │            │
│  ├─────────────────────────────────────────────────┤            │
│  │ IndexNodeHeader:                                 │            │
│  │ • bool is_leaf = false                           │            │
│  │ • int num_key (key count)                        │            │
│  │ • int parent (parent page #)                     │            │
│  │                                                  │            │
│  │ Data: [key₀ | key₁ | ... | keyₙ]                │            │
│  │       [child₀ | child₁ | ... | childₙ₊₁]        │            │
│  │                                                  │            │
│  │ ⚠️  Special: key[i] = MAX key in child[i]       │            │
│  │    (Non-standard B+ tree variant!)               │            │
│  └───────────────────┬─────────────────────────────┘            │
│                      │                                           │
│                      ↓                                           │
│  ┌─────────────────────────────────────────────────┐            │
│  │          Leaf Nodes (Data Pages)                 │            │
│  ├─────────────────────────────────────────────────┤            │
│  │ IndexNodeHeader:                                 │            │
│  │ • bool is_leaf = true                            │            │
│  │ • int num_key (key count)                        │            │
│  │ • int parent (parent page #)                     │            │
│  │ • int prev_leaf (sibling page #)                 │            │
│  │ • int next_leaf (sibling page #)                 │            │
│  │                                                  │            │
│  │ Data: [key₀ | key₁ | ... | keyₙ]                │            │
│  │       [rid₀ | rid₁ | ... | ridₙ]                │            │
│  │                                                  │            │
│  │ Sorted by key, forms doubly-linked list         │            │
│  │ ← prev_leaf ═══ [Leaf] ═══ next_leaf →          │            │
│  └─────────────────────────────────────────────────┘            │
└──────────────────────────┬──────────────────────────────────────┘
                           │ uses
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│                   IndexNodeHandle                                │
│              (In-Memory Node Operations)                         │
├─────────────────────────────────────────────────────────────────┤
│  • get_key(i) / set_key(i, key)                                  │
│  • get_rid(i) / set_rid(i, rid)   [leaf only]                   │
│  • get_child(i) / set_child(i, page) [internal only]             │
│  • insert_key(i, key, val)                                       │
│  • remove_key(i)                                                 │
│  • lower_bound(key) / upper_bound(key)                           │
│  • is_underflow() / is_overflow()                                │
└──────────────────────────┬──────────────────────────────────────┘
                           │ delegates I/O to
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│                      PF_Pager                                    │
│              (Page File Handler - Buffering)                     │
├─────────────────────────────────────────────────────────────────┤
│  • fetch_page(page_no) → Page*                                   │
│  • mark_dirty(page_no)                                           │
│  • unpin_page(page_no)                                           │
│  • allocate_page() → page_no                                     │
│  • dispose_page(page_no)                                         │
│  • force_pages() - Write dirty pages to disk                     │
└─────────────────────────────────────────────────────────────────┘
```

**Data Flow:**
```
insert_entry(key, rid)
    ↓
IndexHandle::insert_entry()
    ↓ traverse tree
IndexHandle::insert_into_leaf()
    ↓ if split needed
IndexHandle::split_leaf()
    ↓ propagate up
IndexHandle::insert_into_parent()
    ↓ manipulate nodes
IndexNodeHandle (get_key, set_key, insert_key)
    ↓ read/write pages
PF_Pager (fetch_page, mark_dirty)
    ↓
Disk I/O (.ndx file)
```

## Index Type: B+ Tree with Maximum-Key Design

### What Makes This Design Special?

**Standard B+ Tree (Separator Keys):**
```
Internal Node: [15 | 30 | 50]
               /    |    |    \
         child0  child1 child2 child3
         
child0: keys ≤ 15
child1: 15 < keys ≤ 30  
child2: 30 < keys ≤ 50
child3: keys > 50
```
Search: Find which *range* the key falls into.

**This Implementation (Maximum Keys):**
```
Internal Node: [15 | 30 | 50]
               /    |    |    \
         child0  child1 child2 child3

key[0]=15: max key in child0
key[1]=30: max key in child1
key[2]=50: max key in child2
child3: any larger keys
```
Search: Find first child whose *maximum* covers your key.

**Example:** To find key `25`:
- Standard: Check ranges → falls between 15 and 30 → go to child1
- This design: Find first max ≥ 25 → max=30 covers it → go to child1

**Why It's Simpler:** During node splits, just copy the existing maximum key to the parent—no need to compute a separator value.

### Key Characteristic
**Internal nodes store the MAXIMUM key** of each child's subtree, not separator keys. This is a non-standard variant:
- Internal node: `key[i] = max_key(child[i])`
- When searching for key `k`, traverse to the first child where `k ≤ max_key[i]`
    ├── open_index() → returns IndexHandle*
    └── destroy_index()

IndexHandle (ndx_handler.h)
    ├── insert_entry(key, rid)
    ├── delete_entry(key, rid)
    ├── lower_bound(key) → IndexID
    ├── upper_bound(key) → IndexID
    └── get_node(page_no) → IndexNodeHandle

IndexNodeHandle (ndx_handler.h)
    ├── lower_bound(key) → slot
    ├── upper_bound(key) → slot
    ├── insert_key() / erase_key()
    └── insert_record_id() / erase_record_id()

IndexIterator (ndx_iterator.h)
    ├── next()
    ├── is_end()
    └── get_RecordID()
```

### Relationships
- **IndexManager**: Static factory class for lifecycle management
- **IndexHandle**: Represents an open B+ tree, manages tree-level operations
- **IndexNodeHandle**: Handle to a single page/node, provides low-level operations
- **IndexIterator**: Range scan iterator over leaf nodes

## Data Structures

### IndexFileHeader (Global Metadata)
```cpp
- root_page: Root node page number
- first_leaf, last_leaf: Leaf linked list boundaries
- btree_order: Maximum children per node
- column_type, col_len: Key type and size
- first_free, num_pages: Free space management
```

### IndexPageHeader (Per-Node Metadata)
```cpp
- parent: Parent node page number
- num_key, num_child: Key/child count
- is_leaf: Node type flag
- prev_leaf, next_leaf: Leaf doubly-linked list (leaves only)
```

### Page Layout
```
|--Page Header--|--Keys Array--|--RecordID/Child Pointers--|
                 key_offset     rid_offset
```

## Core Algorithms

### 1. Insertion (`insert_entry`)

```
insert_entry(key, rid):
    1. Find insertion position:
       iid = upper_bound(key)  // Returns leaf position
       node = get_node(iid.page_no)
    
    2. Insert at position:
       node.insert_key(iid.slot_no, key)
       node.insert_record_id(iid.slot_no, rid)
    
    3. Update parent if max key changed:
       if iid.slot_no == last_slot:
           maintain_parent(node)
    
    4. Handle overflow (while num_child > order):
       if no_parent:
           create_new_root()
       
       brother = create_node()
       split_idx = num_child / 2
       
       // Transfer upper half to brother
       brother.insert_keys(split_keys[split_idx:])
       brother.insert_rids(split_rids[split_idx:])
       node.num_key = split_idx
       
       if is_leaf:
           // Maintain leaf linked list
           brother.next_leaf = node.next_leaf
           brother.prev_leaf = node.page_no
           next.prev_leaf = brother.page_no  // Critical!
           node.next_leaf = brother.page_no
       
       // Promote max key to parent
       parent.insert_key(child_idx, node.max_key)
       parent.insert_rid(child_idx+1, brother_rid)
       
       node = parent  // Propagate overflow upward
```

**Key Points:**
- Uses `upper_bound()` to find insertion position (first key > target)
- Splits at midpoint when overflow occurs
- **Must update `prev_leaf` pointer** of the next leaf during split
- Promotes the **maximum key** of the left node to the parent

### 2. Deletion (`delete_entry`)

```
delete_entry(key, rid):
    1. Locate entry:
       for scan in range(lower_bound(key), upper_bound(key)):
           if scan.rid == rid:
               break
    
    2. Delete from node:
       node.erase_key(slot)
       node.erase_record_id(slot)
       maintain_parent(node)  // Update max key
    
    3. Handle underflow (while num_child < ⌈order/2⌉):
       if is_root:
           if empty_internal_root:
               promote_child_to_root()
           break
       
       // Try borrowing from siblings
       if left_sibling.is_rich():
           borrow_from_left()
           break
       if right_sibling.is_rich():
           borrow_from_right()
           break
       
       // Merge with sibling
       if has_left_sibling:
           merge_with_left()
       else:
           merge_with_right()
       
       node = parent  // Propagate underflow upward
```

**Key Points:**
- Maintains minimum occupancy (⌈order/2⌉ children)
- Tries to borrow before merging
- Updates leaf linked list when merging
- May reduce tree height by deleting root

### 3. Search (`lower_bound` / `upper_bound`)

```
upper_bound(key):
    node = root
    
    // Traverse to leaf
    while not node.is_leaf:
        slot = node.upper_bound(key)  // Binary search
        if slot >= num_key:
            return leaf_end()
        node = get_node(node.child[slot])
    
    // Search in leaf
    slot = node.upper_bound(key)
    return IndexID(node.page_no, slot)

// Node-level upper_bound (binary search)
node.upper_bound(key):
    lo = 0, hi = num_key
    while lo < hi:
        mid = (lo + hi) / 2
        if compare(key, node.key[mid]) < 0:
            hi = mid
        else:
            lo = mid + 1
    return lo
```

**Critical Details:**
- `lower_bound(k)`: First position where key ≥ k
- `upper_bound(k)`: First position where key > k
- **Comparison order matters**: `compare(target, node_key, ...)` not `compare(node_key, target, ...)`
- Bounds check: `if slot >= num_key` prevents out-of-bounds access

### 4. Helper: `maintain_parent`

```
maintain_parent(node):
    curr = node
    while curr has parent:
        parent = get_parent(curr)
        child_idx = parent.find_child(curr)
        
        // Update parent's key to child's max
        if parent.key[child_idx] != curr.max_key:
            parent.key[child_idx] = curr.max_key
            curr = parent
        else:
            break  // No change needed
```

Ensures internal nodes always store the correct maximum key for each child.

## Iterator (Range Scan)

```cpp
IndexIterator scan(index_handle, lower, upper);
while (!scan.is_end()) {
    RecordID rid = scan.get_RecordID();
    // Process rid...
    scan.next();
}
```

Iterator traverses the leaf-level doubly-linked list from `lower` to `upper`.

## Time Complexity

| Operation | Average | Worst Case |
|-----------|---------|------------|
| Search    | O(log n)| O(log n)   |
| Insert    | O(log n)| O(log n)   |
| Delete    | O(log n)| O(log n)   |
| Range Scan| O(log n + k) | O(log n + k) |

where `n` = total keys, `k` = range size

## Disk I/O Optimization

- **Page-based storage**: Each node is one disk page
- **Leaf linked list**: Enables efficient sequential scans without tree traversal
- **Maximum key design**: Simplifies key promotion during splits (use existing max)
- **Binary search**: Reduces comparisons within nodes (configurable via `binary_search` flag)

## Files

- `ndx_defs.h`: Data structures (headers, IndexID)
- `ndx_handler.h/cpp`: Core B+ tree logic
- `ndx_manager.h/cpp`: Index lifecycle management
- `ndx_iterator.h/cpp`: Range scan implementation
- `ndx_test.cpp`: Test suite
