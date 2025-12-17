# Page File Manager System Documentation

## Overview

This module implements a basic page file management system, including:

* **`PF_Pager`**: Handles in-memory page caching and I/O operations to/from disk.
* **`PF_Manager`**: Manages file-level operations like creating, opening, closing, and deleting files, and interfaces with the pager.

This system is designed to optimize page-based access to files using a limited-size memory cache (LRU policy).

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      PF_Manager                                  │
│              (Static Page File Manager)                          │
├─────────────────────────────────────────────────────────────────┤
│  • create_file(filename)                                         │
│  • destroy_file(filename)                                        │
│  • open_file(filename) → PF_Pager*                               │
│  • close_file(PF_Pager*)                                         │
└──────────────────────────┬──────────────────────────────────────┘
                           │ creates/manages
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│                      PF_Pager                                    │
│              (Buffer Pool Manager with LRU)                      │
├─────────────────────────────────────────────────────────────────┤
│  Public API:                                                     │
│  • fetch_page(page_no) → Page*                                   │
│  • create_page(page_no) → Page*                                  │
│  • allocate_page() → page_no                                     │
│  • dispose_page(page_no)                                         │
│  • mark_dirty(page_no)                                           │
│  • unpin_page(page_no)                                           │
│  • force_pages()  - Flush all dirty pages                        │
│                                                                   │
│  Internal State:                                                 │
│  • int fd                    - File descriptor                   │
│  • FileHeader file_header    - File metadata                     │
│  • Page _pages[NUM_CACHE]    - Buffer pool (in-memory)           │
│  • list<Page*> _busy_pages   - LRU list (active pages)           │
│  • list<Page*> _free_pages   - Free page pool                    │
│  • map<PageID, iter> _busy_map - Fast lookup in LRU list         │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│              Buffer Pool Architecture                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  Memory Layout:                                                  │
│  ┌────────────────────────────────────────────────┐             │
│  │ uint8_t _cache[NUM_CACHE * PAGE_SIZE]          │             │
│  │                                                 │             │
│  │  ┌──────────────┐ ┌──────────────┐             │             │
│  │  │   Page 0     │ │   Page 1     │  ...        │             │
│  │  │  (4096 bytes)│ │  (4096 bytes)│             │             │
│  │  └──────────────┘ └──────────────┘             │             │
│  └────────────────────────────────────────────────┘             │
│                                                                   │
│  Page Metadata:                                                  │
│  ┌────────────────────────────────────────────────┐             │
│  │ struct Page {                                   │             │
│  │   int fd;           // File descriptor          │             │
│  │   int page_no;      // Page number on disk      │             │
│  │   int pin_count;    // Reference count          │             │
│  │   bool is_dirty;    // Modified flag            │             │
│  │   uint8_t* data;    // Pointer to _cache        │             │
│  │ }                                               │             │
│  └────────────────────────────────────────────────┘             │
│                                                                   │
│  LRU Doubly-Linked List (_busy_pages):                           │
│  ┌─────────────────────────────────────────────┐                │
│  │  MRU → [Page*] ⇄ [Page*] ⇄ [Page*] → LRU   │                │
│  │        (most       (middle)     (least      │                │
│  │         recent)                  recent)    │                │
│  └─────────────────────────────────────────────┘                │
│  ↑                                                                │
│  └─ Evict from tail when cache full                              │
│                                                                   │
│  Free List (_free_pages):                                        │
│  ┌─────────────────────────────────────────────┐                │
│  │  [Page*] → [Page*] → [Page*] → NULL         │                │
│  │  (unused buffer pool slots)                  │                │
│  └─────────────────────────────────────────────┘                │
│                                                                   │
│  Fast Lookup (_busy_map):                                        │
│  ┌─────────────────────────────────────────────┐                │
│  │  PageID(fd, page_no) → iterator in _busy    │                │
│  │  hash_map for O(1) lookup                    │                │
│  └─────────────────────────────────────────────┘                │
└──────────────────────────┬──────────────────────────────────────┘
                           │ performs I/O
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│                   Disk File Structure                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                   │
│  Page 0: File Header                                             │
│  ┌──────────────────────────────────────┐                       │
│  │ FileHeader                            │                       │
│  │ • int first_free    - Free list head  │                       │
│  │ • int num_pages     - Total pages     │                       │
│  └──────────────────────────────────────┘                       │
│                                                                   │
│  Page 1..N: Data Pages                                           │
│  ┌──────────────────────────────────────┐                       │
│  │ Page Content (4096 bytes)             │                       │
│  │                                       │                       │
│  │ [Application-specific data]           │                       │
│  │ (e.g., RM_PageHeader + records)       │                       │
│  │ (e.g., IndexNodeHeader + keys)        │                       │
│  └──────────────────────────────────────┘                       │
│                                                                   │
│  Free Page List:                                                 │
│  first_free → Page 5 → Page 12 → Page 23 → -1                   │
│  (Each free page stores next_free in its data)                   │
└──────────────────────────┬──────────────────────────────────────┘
                           │
                           ↓
┌─────────────────────────────────────────────────────────────────┐
│                   Operating System                               │
│              (File I/O - read/write/lseek)                       │
└─────────────────────────────────────────────────────────────────┘
```

**Page Lifecycle:**
```
1. Request: fetch_page(page_no)
              ↓
2. Check Cache: _busy_map.find(PageID(fd, page_no))
              ↓
   ┌──── Hit ────┐          ┌──── Miss ────┐
   │             │          │              │
   ↓             ↓          ↓              ↓
3a. Move to MRU  3b. Evict LRU page    Allocate from _free_pages
   in _busy_pages    (if cache full)         or evict LRU
              ↓          │              │
              │          ↓              ↓
              │    Flush if dirty   Read from disk
              │          │         (lseek + read)
              │          ↓              │
              └──────→ Return Page* ←───┘
```

**Data Flow (Write Operation):**
```
Higher-level module (RM/Index)
    ↓ request page
PF_Pager::fetch_page()
    ↓ get page buffer
Modify page->data
    ↓ notify change
PF_Pager::mark_dirty()
    ↓ later...
PF_Pager::force_pages() or eviction
    ↓ write to disk
lseek(fd, page_no * PAGE_SIZE, SEEK_SET)
write(fd, page->data, PAGE_SIZE)
```

---

## `PF_Pager` Class

### Description

`PF_Pager` manages a fixed-size in-memory cache of pages read from or written to disk. It implements an **LRU (Least Recently Used)** replacement strategy when the cache is full.

### Data Members

* `uint8_t _cache[NUM_CACHE_PAGES * PAGE_SIZE]`: Raw memory buffer to hold all page data.
* `Page _pages[NUM_CACHE_PAGES]`: Array of Page objects representing metadata and buffers.
* `std::list<Page *> _busy_pages`: Pages currently in use (i.e., in cache).
* `std::unordered_map<PageID, std::list<Page *>::iterator> _busy_map`: Map of page ID to its position in `_busy_pages`.
* `std::list<Page *> _free_pages`: Pages available for reuse (not in cache).

---

### Public Functions

#### `PF_Pager()`

Initializes the cache. Links the `_pages` to their respective buffer offsets and adds all pages to `_free_pages`.

#### `~PF_Pager()`

Flushes all dirty pages to disk. Asserts that all pages are freed back to `_free_pages`.

#### `Page *create_page(int fd, int page_no)`

* Allocates and returns a new page in cache.
* Marks the page as dirty.
* Does **not** load content from disk (assumes a new page).

#### `Page *fetch_page(int fd, int page_no)`

* Retrieves an existing page from disk.
* If the page is in cache, it is moved to the front (most recently used).
* If not in cache, a page is evicted and the requested page is read into memory.

#### `void flush_file(int fd)`

* Iterates over all busy pages and flushes those associated with the given file descriptor.

#### `void flush_page(Page *page)`

* Forces a dirty page to be written to disk.
* Removes it from `_busy_pages` and adds it to `_free_pages`.

#### `void flush_all()`

* Flushes all dirty pages to disk and clears the cache (`_busy_pages`).

#### `bool in_cache(const PageID &page_id) const`

* Returns true if a page is in cache (`_busy_map`).

#### Accessor Functions

* `get_busy_pages()`: Returns list of busy pages.
* `get_busy_map()`: Returns the map of cached page positions.
* `get_free_pages()`: Returns list of free pages.

---

### Static Helper Functions

#### `static void read_page(int fd, int page_no, uint8_t *buf, int num_bytes)`

* Reads `num_bytes` from the given file descriptor and page number into `buf`.

#### `static void write_page(int fd, int page_no, const uint8_t *buf, int num_bytes)`

* Writes `num_bytes` from `buf` to the file at the specified page location.

---

### Private Functions

#### `template <bool EXISTS> Page *get_page(int fd, int page_no)`

* Core page access function.
* If `EXISTS == true`, assumes the page exists on disk and loads it.
* If `EXISTS == false`, assumes it’s a new page.
* Handles LRU eviction if cache is full:

  * Removes least recently used page from back of `_busy_pages`.
  * Writes dirty page back to disk.
* Moves accessed page to the front of `_busy_pages`.

#### `void force_page(Page *page)`

* If the page is dirty, writes it to disk and marks it clean.

#### `void access(Page *page)`

* Moves the page to the front of `_busy_pages` (marks it as most recently used).

---

## `PF_Manager` Class

### Description

`PF_Manager` is responsible for file-level operations and tracking open files.

### Static Members

* `static PF_Pager pager`: Global pager instance used for page caching.
* `static std::unordered_map<std::string, int> _path2fd`: Maps file path to file descriptor.
* `static std::unordered_map<int, std::string> _fd2path`: Maps file descriptor back to file path.

---

### Functions

#### `static bool is_file(const std::string &path)`

* Checks if the file exists and is a regular file using `stat`.

#### `static void create_file(const std::string &path)`

* Creates a new file with user read/write permissions.
* Throws `FileExistsError` if the file already exists.

#### `static void destroy_file(const std::string &path)`

* Deletes a file.
* Throws `FileNotFoundError` if the file doesn’t exist.
* Throws `FileNotClosedError` if file is still open.

#### `static int open_file(const std::string &path)`

* Opens an existing file in read/write mode.
* Throws:

  * `FileNotFoundError` if the file doesn’t exist.
  * `FileNotClosedError` if already open.
* Registers the open file in `_path2fd` and `_fd2path`.

#### `static void close_file(int fd)`

* Closes the file descriptor.
* Flushes associated pages from cache via `pager.flush_file`.
* Unregisters the file descriptor from internal maps.

---

## Page Eviction Algorithm: LRU

1. Pages in `_busy_pages` are maintained in usage order.
2. Most recently accessed pages are at the **front**, least recently used at the **back**.
3. When cache is full:

   * Evict the page at the back of `_busy_pages`.
   * Flush it to disk if dirty.
   * Remove it from `_busy_map`.
   * Reuse it for the new page.

---

## Error Handling

The module throws appropriate exceptions (assumed to be defined in `error.h`) like:

* `FileNotFoundError`
* `FileExistsError`
* `FileNotClosedError`
* `FileNotOpenError`
* `UnixError`

These errors ensure safe and predictable behavior for file and memory management.

---

![PF UML Diagram](../../documentation/uml_diagrams/pf.png)
