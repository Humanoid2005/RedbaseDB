#ifndef NDX_STRUCTS_H
#define NDX_STRUCTS_H

#include "../db_defs.h"
#include "../page_file_handler/pf.h"

constexpr int NDX_NO_PAGE = -1;            // Represents an invalid or non-existent page.
constexpr int NDX_FILE_HDR_PAGE = 0;       // Page number reserved for the file header.
constexpr int NDX_LEAF_HEADER_PAGE = 1;    // Likely a reserved page for leaf metadata.
constexpr int NDX_INIT_ROOT_PAGE = 2;      // Initial root page of the B+ tree.
constexpr int NDX_INIT_NUM_PAGES = 3;      // Initial number of pages when the index is created.
constexpr int NDX_MAX_COL_LEN = 512;       // Maximum length allowed for a key column.

/*
Here we are implementing a B+ Tree index structure.
The indexID class is used to identify a specific key in the index using its page number and slot number.
The index file header structure stores metadata about the entire index file.
The index page header structure stores metadata about a specific page in the index file i.e. a node in the B+ tree.
Each page in the B+ tree (leaf or non-leaf) has a header describing its contents.
*/

/*
Structure of a B+ Tree:
- p = order of B+ tree
- Non-leaf node: p block pointers ; p-1 keys
- Leaf Node: p-1 keys; p-1 record pointers; 2 block pointers (prev and next leaf node)

Properties:
B+ tree properties: Order p => max_children = p
-Keys are sorted.
-All leaf nodes are at the same level. ( p-1 keys, p-1 record ptrs, 2 block ptrs)
-Non-leaf nodes act as routing/index nodes. ( p-1 keys, p block ptrs)
-Leaf nodes store actual (key, rid) pairs.
-Each node has a maximum number of children (defined by btree_order), and overflows are resolved by splitting.
*/

class IndexID {
public:
    int page_number;   // Page number within the file
    int slot_number;   // Slot number within that page (position of the key)

    IndexID() = default;

    IndexID(int page_number, int slot_number){
        this->page_number = page_number;
        this->slot_number = slot_number;
    }

    friend bool operator==(const IndexID &x, const IndexID &y) {
        return x.page_number == y.page_number && x.slot_number == y.slot_number;
    }

    friend bool operator!=(const IndexID &x, const IndexID &y) {
        return !(x == y);
    }
};

class IndexFileHeader {
public:
    int first_free;     // First free page for reuse
    int num_pages;      // Total number of pages in the index file
    int root_page;      // Page number of the root node in the B+ tree

    ColumnType column_type;   // Data type of the indexed column (e.g., int, float, string, datetime)
    int col_len;        // Length of the key column in bytes

    int btree_order;    // Maximum number of children a B+ tree node can have
    int key_offset;     // Byte offset where key array begins in each page
    int rid_offset;     // Byte offset where rid (or child pointers) begin

    int first_leaf;     // Page number of the first leaf in the B+ tree
    int last_leaf;      // Page number of the last leaf in the B+ tree

    IndexFileHeader() = default;

    IndexFileHeader(int first_free, int num_pages, int root_page, ColumnType column_type,int col_len, int btree_order, int key_offset, int rid_offset,int first_leaf, int last_leaf){
        this->first_free = first_free;
        this->num_pages = num_pages;
        this->root_page = root_page;
        this->column_type = column_type;
        this->col_len = col_len;
        this->btree_order = btree_order;
        this->key_offset = key_offset;
        this->rid_offset = rid_offset;
        this->first_leaf = first_leaf;
        this->last_leaf = last_leaf;
    }
};

class IndexPageHeader {
public:
    int next_free;   // Next free page (used for managing free space)
    int parent;      // Page number of the parent node
    int num_key;     // Number of keys stored in this page
    int num_child;   // Number of children pointers (maximum is order of the B+ tree)

    bool is_leaf;    // Flag indicating if the page is a leaf node

    // These are only used if the page is a leaf node:
    int prev_leaf;   // Page number of the previous leaf node
    int next_leaf;   // Page number of the next leaf node

    IndexPageHeader() = default;

    IndexPageHeader(int next_free, int parent, int num_key, int num_child,bool is_leaf, int prev_leaf, int next_leaf){
        this->next_free = next_free;
        this->parent = parent;
        this->num_key = num_key;
        this->num_child = num_child;
        this->is_leaf = is_leaf;
        this->prev_leaf = prev_leaf;
        this->next_leaf = next_leaf;
    }

};


#endif