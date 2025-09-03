#ifndef NDX_HANDLER_H
#define NDX_HANDLER_H

#include "ndx_defs.h"

static const bool binary_search = true; //SET_AFTER_TESTING

int compare_keys(const u_int8_t * a,const u_int8_t * b,ColumnType type,int column_length);

/*
Class to handle operations in a B+ tree node
*/
class IndexNodeHandle{
public:
    IndexPageHeader *header;
    uint8_t * keys;
    RecordID* record_pointers;
    Page* pages;
    const IndexFileHeader * index_header;


    IndexNodeHandle() = default;

    IndexNodeHandle(const IndexFileHeader * index_header,Page * pages){
        this->index_header = index_header;
        this->pages = pages;
        this->header = (IndexPageHeader*)pages->buf;
        this->keys = pages->buf + index_header->key_offset;
        this->record_pointers = (RecordID*)(pages->buf + index_header->rid_offset);
    }

    u_int8_t * get_key(int key_id)const {
        return keys + key_id*(index_header->col_len);
    }

    RecordID * get_record_id(int record_id){
        return &record_pointers[record_id];
    }

    int lower_bound(const uint8_t* target) const;
    int upper_bound(const u_int8_t* target) const;

    void insert_keys(int pos,const uint8_t * keys,int n);
    void insert_key(int pos,const uint8_t * key);
    void erase_key(int pos);

    void insert_record_ids(int pos,const RecordID * recordIDs,int n);
    void insert_record_id(int pos,const RecordID &recordID);
    void erase_record_id(int pos);

    int find_child(IndexNodeHandle &child);
};

/*
Class to handle operations pertaining to the entire B+ tree
*/

class IndexHandle{
    friend class IndexIterator;
private:
    IndexNodeHandle create_node();

    void maintain_parent(const IndexNodeHandle &node);
    void maintain_child(IndexNodeHandle &node,int child_index);

    void erase_leaf(IndexNodeHandle &leaf);
    void release_node(IndexNodeHandle & node);
public:
    int fd;
    IndexFileHeader index_header;

    IndexHandle(int fd){
        this->fd = fd;
        PF_Pager::read_page(fd,NDX_FILE_HDR_PAGE,(u_int8_t*)&(this->index_header),sizeof(this->index_header));
    }

    IndexNodeHandle get_node(int page_number) const;

    void insert_entry(const u_int8_t *key,const RecordID &rid);
    void delete_entry(const u_int8_t *key, const RecordID &rid);
    RecordID get_record_id(const IndexID &index_id) const;

    IndexID lower_bound(const uint8_t * target) const;
    IndexID upper_bound(const uint8_t* target) const;

    IndexID leaf_end() const;
    IndexID leaf_begin() const;
};

#endif