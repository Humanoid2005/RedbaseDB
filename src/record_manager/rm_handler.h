#ifndef RM_HANDLER_H
#define RM_HANDLER_H

#include "bitmap.h"
#include "rm_structs.h"
#include "../db_structs.h"
#include <memory>

class RM_PageHandle{
public:
    RM_PageHeader * page_header;
    uint8_t * bitmap;
    uint8_t * slots;
    Page * page;
    const RM_FileHeader * file_header;

    RM_PageHandle(const RM_FileHeader * file_header,Page * page){
        this->file_header = file_header;
        this->page = page;
        this->page_header = (RM_PageHeader*)page->buf;
        this->bitmap = page->buf + sizeof(RM_PageHeader);
        this->slots = bitmap + this->file_header->bitmap_size;
    }

    uint8_t* get_slot(int slot_number) const{
        return this->slots + slot_number*(this->file_header->record_size);
    }
};

class RM_FileHandle{
    friend class RM_Scanner;

private:
    RM_PageHandle fetch_page(int page_number) const;

    RM_PageHandle create_page();

    void release_page(RM_PageHandle &page_handle);

public:
    RM_FileHeader file_header;
    int fd;

    RM_FileHandle(int fd){
        this->fd = fd;
        PF_Pager::read_page(fd,RM_FILE_HDR_PAGE,(uint8_t*)&file_header,sizeof(file_header));
    }

    RM_FileHandle(const RM_FileHandle &other) = delete;

    RM_FileHandle &operator=(const RM_FileHandle &other) = delete;

    bool is_record(const RecordID record_id) const;

    std::unique_ptr<RM_Record> get_record(const RecordID &record_id) const;

    RecordID insert_record(uint8_t *buf);

    void delete_record(const RecordID &record_id);

    void update_record(const RecordID &record_id,uint8_t * buf);
};


#endif