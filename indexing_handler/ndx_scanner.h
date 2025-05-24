#ifndef NDX_SCANNER_H
#define NDX_SCANNER_H

#include "../db_structs.h"
#include "ndx_structs.h"

class IndexHandle;

/*
Class : Iterator for B+ tree enabling range scans
*/
class IndexScanner: public RecordScanner{
private:
    IndexHandle * index_handle;
    IndexID current_index;
    IndexID end_index;
public:
    IndexScanner(IndexHandle* index_handle,const IndexID &lower,const IndexID &upper){
        this->index_handle = index_handle;
        this->current_index = lower;
        this->end_index = upper;
    }

    const IndexID& current_index_id(){
        return current_index;
    }

    RecordID current_record_id() const override;
    void next() override;
    bool is_end() const override{
        return current_index==end_index;
    }
};

#endif