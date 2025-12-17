#ifndef NDX_ITERATOR_H
#define NDX_ITERATOR_H

#include "../db_defs.h"
#include "ndx_defs.h"

class IndexHandle;

/*
Class : Iterator for B+ tree enabling range scans
*/
class IndexIterator: public RecordIterator{
private:
    IndexHandle * index_handle;
    IndexID current_index;
    IndexID end_index;
public:
    IndexIterator(IndexHandle* index_handle,const IndexID &lower,const IndexID &upper){
        this->index_handle = index_handle;
        this->current_index = lower;
        this->end_index = upper;
    }

    const IndexID& current_index_id(){
        return current_index;
    }

    RecordID get_RecordID() const override;
    void next() override;
    bool is_end() const override{
        return current_index==end_index;
    }
};

#endif