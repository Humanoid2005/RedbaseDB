#ifndef RM_SCANNER_H
#define RM_SCANNER_H

#include "rm_structs.h"
#include "../db_structs.h"

class RM_FileHandle;

class RM_Scanner: public RecordScanner{
private:
    const RM_FileHandle * filehandle;
    RecordID record_id;
public:
    RM_Scanner(const RM_FileHandle * file_handle);
    void next() override;
    bool is_end() const override {
        return record_id.page_number == RM_NO_PAGE;
    }

    RecordID current_record_id() const override{
        return record_id;
    }
};

#endif