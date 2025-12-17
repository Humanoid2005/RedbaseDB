#include "ndx_handler.h"
#include "ndx_defs.h"
#include "../datetime.h"
#include "../db_defs.h"
#include <cassert>
#include "ndx_iterator.h"

int compare_keys(const u_int8_t * a,const u_int8_t * b,ColumnType type,int column_length){
    switch(type){
        case TYPE_INT:{
            int ia = *(int*)a;
            int ib = *(int*)b;
            if(ia==ib){
                return 0;
            }
            else if(ia<ib){
                return -1;
            }
            else{
                return 1;
            }
            break;
        }
        case TYPE_FLOAT:{
            float fa = *(float*)a;
            float fb = *(float*)b;
            if(fa==fb){
                return 0;
            }
            else if(fa<fb){
                return -1;
            }
            else{
                return 1;
            }
            break;
        }
        case TYPE_STRING:{
            return memcmp(a,b,column_length);
        }
        case TYPE_DATETIME:{
            DateTime da = *(DateTime*)a;
            DateTime db = *(DateTime*)b;

            if(da==db){
                return 0;
            }
            if(da<db){
                return -1;
            }
            else{
                return 1;
            }
            break;
        }
        default:{
            throw InternalError("Unexpected data type");
            break;
        }
    }
}

/*
Implementing B+ tree node functions
*/
int IndexNodeHandle::lower_bound(const uint8_t * target) const {
    if(binary_search){
        int lo = 0, hi = this->header->num_key;
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            uint8_t *key_addr = get_key(mid);
            if (compare_keys(target, key_addr, this->index_header->column_type, this->index_header->col_len) <= 0) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }
        return lo;
    }
    else{
        int key_idx = 0;
        while (key_idx < this->header->num_key) {
            uint8_t *key_addr = get_key(key_idx);
            if (compare_keys(target, key_addr, this->index_header->column_type, this->index_header->col_len) <= 0) {
                break;
            }
            key_idx++;
        }
        return key_idx;
    }
}

int IndexNodeHandle::upper_bound(const u_int8_t *target) const
{
    if(binary_search){
        int lo = 0, hi = this->header->num_key;
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            uint8_t *key_slot = get_key(mid);
            if (compare_keys(target, key_slot, this->index_header->column_type, this->index_header->col_len) < 0) {
                hi = mid;
            } else {
                lo = mid + 1;
            }
        }
        return lo;
    }
    else{
        int key_idx = 0;
        while (key_idx < this->header->num_key) {
            uint8_t *key_addr = get_key(key_idx);
            if (compare_keys(target, key_addr, this->index_header->column_type, this->index_header->col_len) < 0) {
                break;
            }
            key_idx++;
        }
        return key_idx;
    }
}

/*
The below insertion and deletion functions' implementation logic is same as how we do insertions and deletions at a particular position in a C array
*/

void IndexNodeHandle::insert_keys(int pos,const uint8_t* keys,int n){
    uint8_t* key_slot = get_key(pos);
    memmove(key_slot + n*(index_header->col_len),key_slot,(this->header->num_key-pos)*this->index_header->col_len);// shifting [pos...num_key-pos] to [pos+n,...,num_key+n]
    memcpy(key_slot,keys,n*(this->index_header->col_len));//adding keys to [0,...pos]
    this->header->num_key += n;//updating number of keys present in node
}

void IndexNodeHandle::insert_key(int pos,const uint8_t * key){
    insert_keys(pos,key,1);
}

void IndexNodeHandle::erase_key(int pos){
    uint8_t* key = get_key(pos);
    memmove(key, key + this->index_header->col_len, (this->header->num_key - pos - 1) * (this->index_header->col_len));//shifting  all records after pos one index back for deletion
    this->header->num_key--;
}

void IndexNodeHandle::insert_record_ids(int pos, const RecordID *record_id, int n) {
    RecordID *record_id_slot = get_record_id(pos);
    memmove(record_id_slot + n, record_id_slot, (this->header->num_child - pos) * sizeof(RecordID));
    memcpy(record_id_slot, record_id, n * sizeof(RecordID));
    this->header->num_child += n;
}

void IndexNodeHandle::insert_record_id(int pos, const RecordID &rid) {
    insert_record_ids(pos, &rid, 1); 
}

void IndexNodeHandle::erase_record_id(int pos) {
    RecordID *rid = get_record_id(pos);
    memmove(rid, rid + 1, (this->header->num_child - pos - 1) * sizeof(RecordID));
    this->header->num_child--;
}

/*
Function to get index/position of any of the child node of the parent node
*/
int IndexNodeHandle::find_child(IndexNodeHandle &child){
    int crank = 0;
    for(crank=0;crank<this->header->num_child;crank++){
        RecordID * rid = get_record_id(crank);
        if(rid->page_no==child.pages->id.page_no){
            break;
        }
    }
    assert(crank<this->header->num_child);
    return crank;
}

/*
Implementing B+ tree functions
*/


IndexNodeHandle IndexHandle::create_node() {
    Page *page;
    IndexNodeHandle node;
    if (this->index_header.first_free == NDX_NO_PAGE) {
        page = PF_Manager::pager.create_page(fd, this->index_header.num_pages);
        this->index_header.num_pages++;
        node = IndexNodeHandle(&this->index_header, page);
    } else {
        page = PF_Manager::pager.fetch_page(fd, this->index_header.first_free);
        node = IndexNodeHandle(&this->index_header, page);
        this->index_header.first_free = node.header->next_free;
    }
    page->mark_dirty();
    return node;
}

IndexNodeHandle IndexHandle::get_node(int page_no) const {
    assert(page_no < this->index_header.num_pages);
    Page *page = PF_Manager::pager.fetch_page(this->fd, page_no);
    IndexNodeHandle node = IndexNodeHandle(&this->index_header, page);
    return node;
}

RecordID IndexHandle::get_record_id(const IndexID &index_id) const
{
    // Handle end case
    if (index_id.page_number == -1 && index_id.slot_number == -1) {
        throw IndexEntryNotFoundError();
    }
    
    IndexNodeHandle node = get_node(index_id.page_number);
    if (!node.header->is_leaf) {
        throw InternalError("Attempting to get record ID from non-leaf node");
    }
    if (index_id.slot_number >= node.header->num_key) {
        throw IndexEntryNotFoundError();
    }
    return *node.get_record_id(index_id.slot_number);
}

IndexID IndexHandle::lower_bound(const uint8_t *target) const {
    // Start from root
    IndexNodeHandle node = get_node(this->index_header.root_page);
    
    // Traverse down to leaf
    while (!node.header->is_leaf) {
        int key_idx = node.lower_bound(target);
        if (key_idx >= node.header->num_key) {
            return leaf_end();
        }
        RecordID *child = node.get_record_id(key_idx);
        node = get_node(child->page_no);
    }
    
    // Now we're at a leaf node, do lower_bound search
    int slot = node.lower_bound(target);
    
    // Handle end cases
    if (slot >= node.header->num_key) {
        // Need to go to next leaf or return end
        if (node.header->next_leaf == NDX_LEAF_HEADER_PAGE || node.header->next_leaf == NDX_NO_PAGE) {
            return leaf_end();
        } else {
            return IndexID(node.header->next_leaf, 0);
        }
    }
    
    return IndexID(node.pages->id.page_no, slot);
}

IndexID IndexHandle::upper_bound(const uint8_t *target) const {
    // Start from root
    IndexNodeHandle node = get_node(this->index_header.root_page);
    
    // Traverse down to leaf
    while (!node.header->is_leaf) {
        int key_idx = node.upper_bound(target);
        if (key_idx >= node.header->num_key) {
            return leaf_end();
        }
        RecordID *child = node.get_record_id(key_idx);
        node = get_node(child->page_no);
    }
    
    // Now we're at a leaf node, do upper_bound search
    int slot = node.upper_bound(target);
    
    // Handle end cases
    if (slot >= node.header->num_key) {
        // Need to go to next leaf or return end
        if (node.header->next_leaf == NDX_LEAF_HEADER_PAGE || node.header->next_leaf == NDX_NO_PAGE) {
            return leaf_end();
        } else {
            return IndexID(node.header->next_leaf, 0);
        }
    }
    
    return IndexID(node.pages->id.page_no, slot);
}

IndexID IndexHandle::leaf_begin() const {
    IndexID iid(this->index_header.first_leaf, 0);
    return iid;
}

IndexID IndexHandle::leaf_end() const {
    IndexNodeHandle node = get_node(this->index_header.last_leaf);
    IndexID iid(this->index_header.last_leaf, node.header->num_key);
    return iid;
}

void IndexHandle::maintain_parent(const IndexNodeHandle &node) {
    IndexNodeHandle curr = node;
    while (curr.header->parent != NDX_NO_PAGE) {
        if (curr.header->num_key == 0) {
            curr = get_node(curr.header->parent);
            continue;
        }

        IndexNodeHandle parent = get_node(curr.header->parent);
        parent.pages->mark_dirty();

        int rank = parent.find_child(curr);
        const uint8_t *child_max_key = curr.get_key(curr.header->num_key - 1);

        if (rank >= parent.header->num_key) {
            parent.insert_key(parent.header->num_key, child_max_key);
        } else {
            uint8_t *parent_key = parent.get_key(rank);
            if (memcmp(parent_key, child_max_key, this->index_header.col_len) != 0) {
                memcpy(parent_key, child_max_key, this->index_header.col_len);
            }
        }
        curr = parent;
    }
}

void IndexHandle::erase_leaf(IndexNodeHandle &leaf) {
    assert(leaf.header->is_leaf);
    IndexNodeHandle prev = get_node(leaf.header->prev_leaf);
    prev.pages->mark_dirty();
    prev.header->next_leaf = leaf.header->next_leaf;

    IndexNodeHandle next = get_node(leaf.header->next_leaf);
    next.pages->mark_dirty();
    next.header->prev_leaf = leaf.header->prev_leaf;
}

void IndexHandle::release_node(IndexNodeHandle &node) {
    node.header->next_free = this->index_header.first_free;
    this->index_header.first_free = node.pages->id.page_no;
}

void IndexHandle::maintain_child(IndexNodeHandle &node, int child_idx) {
    if (!node.header->is_leaf) {
        // Current node is inner node, load its child and set its parent to current node
        int child_page_no = node.get_record_id(child_idx)->page_no;
        IndexNodeHandle child = get_node(child_page_no);
        child.pages->mark_dirty();
        child.header->parent = node.pages->id.page_no;
    }
}

/*
Insertion of a new record into B+ tree
Ensure that all B+ properties are obeyed after the addition of new a new record.

Algorithm:
1. Find the position of insertion based on value of key
2. Check if that node has space to insert the entry
- If yes: Insert the entry
- If no: Split the node
3. Check if the node is the last leaf node
- If yes: Update the last leaf pointer in the index header
- If no: Maintain the parent's max key
4. If the node is overflowed, split it and update the parent
- If the current node is root node, allocate a new root
- Allocate a new brother node
- Split the current node at the middle position
- Transfer the keys and record IDs to the brother node
- Update the parent's key with the last key of the current node
*/

void IndexHandle::insert_entry(const u_int8_t *key, const RecordID &rid)
{
    IndexID iid = upper_bound(key);
    IndexNodeHandle node = get_node(iid.page_number);
    node.pages->mark_dirty();
    // We need to insert at iid.slot_no
    node.insert_key(iid.slot_number, key);
    node.insert_record_id(iid.slot_number, rid);
    // Maintain parent's max key
    if (iid.page_number == this->index_header.last_leaf && iid.slot_number == node.header->num_key - 1) {
        // Max key updated
        maintain_parent(node);
    }
    // Solve overflow
    while (node.header->num_child > this->index_header.btree_order) {
        // If leaf node is overflowed, we need to split it
        if (node.header->parent == NDX_NO_PAGE) {
            // If current page is root node, allocate new root
            IndexNodeHandle root = create_node();
            *root.header = IndexPageHeader(NDX_NO_PAGE, NDX_NO_PAGE, 0, 0, false, NDX_NO_PAGE, NDX_NO_PAGE);
            // Insert current node's key & rid
            RecordID curr_rid(node.pages->id.page_no, -1);
            root.insert_record_id(0, curr_rid);
            root.insert_key(0, node.get_key(node.header->num_key - 1));
            // update current node's parent
            node.header->parent = root.pages->id.page_no;
            // update global root page
            this->index_header.root_page = root.pages->id.page_no;
        }
        // Allocate brother node
        IndexNodeHandle bro = create_node();
        *bro.header = IndexPageHeader(NDX_NO_PAGE,
                             node.header->parent, // They have the same parent
                             0, 0,
                             node.header->is_leaf, // Brother node is leaf only if current node is leaf.
                             NDX_NO_PAGE, NDX_NO_PAGE);
        if (bro.header->is_leaf) {
            // maintain brother node's leaf pointer
            bro.header->next_leaf = node.header->next_leaf;
            bro.header->prev_leaf = node.pages->id.page_no;
            // Let original next node's prev = brother node
            IndexNodeHandle next = get_node(node.header->next_leaf);
            next.pages->mark_dirty();
            next.header->prev_leaf = bro.pages->id.page_no;
            // curr's next = brother node
            node.header->next_leaf = bro.pages->id.page_no;
        }
        // Split at middle position
        int split_idx = node.header->num_child / 2;
        // Keys in [0, split_idx) stay in current node, [split_idx, curr_keys) go to brother node
        int num_transfer = node.header->num_key - split_idx;
        bro.insert_keys(0, node.get_key(split_idx), num_transfer);
        bro.insert_record_ids(0, node.get_record_id(split_idx), num_transfer);
        node.header->num_key = split_idx;
        node.header->num_child = split_idx;
        // Update children's parent
        for (int child_idx = 0; child_idx < bro.header->num_child; child_idx++) {
            maintain_child(bro, child_idx);
        }
        // Copy the last key up to its parent
        u_int8_t *popup_key = node.get_key(split_idx - 1);
        // Load parent node
        IndexNodeHandle parent = get_node(node.header->parent);
        parent.pages->mark_dirty();
        // Find the rank of current node in its parent
        int child_idx = parent.find_child(node);
        // Insert popup key into parent
        parent.insert_key(child_idx, popup_key);
        RecordID bro_rid(bro.pages->id.page_no, -1);
        parent.insert_record_id(child_idx + 1, bro_rid);
        // Update global last_leaf if needed
        if (this->index_header.last_leaf == node.pages->id.page_no) {
            this->index_header.last_leaf = bro.pages->id.page_no;
        }
        // Go to its parent
        node = parent;
    }
}

/*
Deletion of a record from B+ tree
Ensure that all B+ properties are obeyed after the deletion of record.

Algorithm:
1. Find the position of deletion based on value of key
2. Check if that node has the entry to delete
- If yes: Delete the entry
- If no: Do nothing
3. Check if the node is underflowed
- If yes: Solve underflow
  - If current node is root node, underflow is permitted
  - If current node has left brother, borrow one node from it
  - If current node has right brother, borrow one node from it
  - If neither brothers are rich, merge with left brother
- If current node is root node and it is empty, delete the root and set new root to its first child
*/

void IndexHandle::delete_entry(const u_int8_t *key, const RecordID &rid)
{
    IndexID lower = lower_bound(key);
    IndexID upper = upper_bound(key);
    for (IndexIterator scan(this, lower, upper); !scan.is_end(); scan.next()) {
        // load btree node
        IndexNodeHandle node = get_node(scan.current_index_id().page_number);
        assert(node.header->is_leaf);
        RecordID *curr_rid = node.get_record_id(scan.current_index_id().slot_number);
        if (*curr_rid != rid) {
            continue;
        }
        // Found the entry with the given rid, delete it
        node.pages->mark_dirty();
        node.erase_key(scan.current_index_id().slot_number);
        node.erase_record_id(scan.current_index_id().slot_number);
        // Update its parent's key to the node's new last key
        maintain_parent(node);
        // Solve underflow
        while (node.header->num_child < (this->index_header.btree_order + 1) / 2) {
            if (node.header->parent == NDX_NO_PAGE) {
                // If current node is root node, underflow is permitted
                if (!node.header->is_leaf && node.header->num_key <= 1) {
                    // If root node is not leaf and it is empty, delete the root
                    int new_root_page = node.get_record_id(0)->page_no;
                    // Load new root and set its parent to NO_PAGE
                    IndexNodeHandle new_root = get_node(new_root_page);
                    new_root.pages->mark_dirty();
                    new_root.header->parent = NDX_NO_PAGE;
                    // Update global root
                    this->index_header.root_page = new_root_page;
                    // Free current page
                    release_node(node);
                }
                break;
            }
            // Load parent node
            IndexNodeHandle parent = get_node(node.header->parent);
            parent.pages->mark_dirty();
            // Find the rank of this child in its parent
            int child_idx = parent.find_child(node);
            if (0 < child_idx) {
                // current node has left brother, load it
                IndexNodeHandle brother_node = get_node(parent.get_record_id(child_idx - 1)->page_no);
                if (brother_node.header->num_child > (this->index_header.btree_order + 1) / 2) {
                    // If left brother is rich, borrow one node from it
                    brother_node.pages->mark_dirty();
                    node.insert_key(0, brother_node.get_key(brother_node.header->num_key - 1));
                    node.insert_record_id(0, *brother_node.get_record_id(brother_node.header->num_child - 1));
                    brother_node.erase_key(brother_node.header->num_key - 1);
                    brother_node.erase_record_id(brother_node.header->num_child - 1);
                    // Maintain parent's key as the node's max key
                    maintain_parent(brother_node);
                    // Maintain first child's parent
                    maintain_child(node, 0);
                    // underflow is solved
                    break;
                }
            }
            if (child_idx + 1 < parent.header->num_child) {
                // current node has right brother, load it
                IndexNodeHandle brother_node = get_node(parent.get_record_id(child_idx + 1)->page_no);
                if (brother_node.header->num_child > (this->index_header.btree_order + 1) / 2) {
                    // If right brother is rich, borrow one node from it
                    brother_node.pages->mark_dirty();
                    node.insert_key(node.header->num_key, brother_node.get_key(0));
                    node.insert_record_id(node.header->num_child, *brother_node.get_record_id(0));
                    brother_node.erase_key(0);
                    brother_node.erase_record_id(0);
                    // Maintain parent's key as the node's max key
                    maintain_parent(node);
                    // Maintain last child's parent
                    maintain_child(node, node.header->num_child - 1);
                    // Underflow is solved
                    break;
                }
            }
            // neither brothers is rich, need to merge
            if (0 < child_idx) {
                // merge with left brother, transfer all children of current node to left brother
                IndexNodeHandle brother_node = get_node(parent.get_record_id(child_idx - 1)->page_no);
                brother_node.pages->mark_dirty();
                brother_node.insert_keys(brother_node.header->num_key, node.get_key(0), node.header->num_key);
                brother_node.insert_record_ids(brother_node.header->num_child, node.get_record_id(0), node.header->num_child);
                // Maintain left brother's children
                for (int i = brother_node.header->num_child - node.header->num_child; i < brother_node.header->num_child; i++) {
                    maintain_child(brother_node, i);
                }
                parent.erase_key(child_idx);
                parent.erase_record_id(child_idx);
                maintain_parent(brother_node);
                // Maintain leaf list
                if (node.header->is_leaf) {
                    erase_leaf(node);
                }
                // Update global last-leaf
                if (this->index_header.last_leaf == node.pages->id.page_no) {
                    this->index_header.last_leaf = brother_node.pages->id.page_no;
                }
                // Free current page
                release_node(node);
            } else {
                assert(child_idx + 1 < parent.header->num_child);
                // merge with right brother, transfer all children of right brother to current node
                IndexNodeHandle brother_node = get_node(parent.get_record_id(child_idx + 1)->page_no);
                brother_node.pages->mark_dirty();
                // Transfer all right brother's valid rid to current node
                node.insert_record_ids(node.header->num_child, brother_node.get_record_id(0), brother_node.header->num_child);
                node.insert_keys(node.header->num_key, brother_node.get_key(0), brother_node.header->num_key);
                // Maintain current node's children
                for (int i = node.header->num_child - brother_node.header->num_child; i < node.header->num_child; i++) {
                    maintain_child(node, i);
                }
                parent.erase_record_id(child_idx + 1);
                parent.erase_key(child_idx);
                // Maintain parent's key as the node's max key
                maintain_parent(node);
                // Maintain leaf list
                if (brother_node.header->is_leaf) {
                    erase_leaf(brother_node);
                }
                // Update global last leaf
                if (this->index_header.last_leaf == brother_node.pages->id.page_no) {
                    this->index_header.last_leaf = node.pages->id.page_no;
                }
                // Free right brother page
                release_node(brother_node);
            }
            node = parent;
        }
        return;
    }
    throw IndexEntryNotFoundError();
}


