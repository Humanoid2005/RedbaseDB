#ifndef RM_FILE_HANDLE_H
#define RM_FILE_HANDLE_H

#include "./bitmap.h"
#include "./rm_defs.h"
#include <memory>

struct RM_PageHandle {
    RM_PageHeader *hdr;
    uint8_t *bitmap;
    uint8_t *slots;
    Page *page;
    const RM_FileHeader *fhdr;

    RM_PageHandle(const RM_FileHeader *fhdr_, Page *page_) : page(page_), fhdr(fhdr_) {
        hdr = (RM_PageHeader *)page->buf;
        bitmap = page->buf + sizeof(RM_PageHeader);
        slots = bitmap + fhdr->bitmap_size;
    }

    uint8_t *get_slot(int slot_no) const { return slots + slot_no * fhdr->record_size; }
};

class RM_FileHandle {

  friend class RM_Iterator;

  private:
    RM_PageHandle fetch_page(int page_no) const;

    RM_PageHandle create_page();

    void release_page(RM_PageHandle &ph);

  public:
    RM_FileHeader hdr;
    int fd;

    RM_FileHandle(int fd_) {
        fd = fd_;
        PF_Pager::read_page(fd, RM_FILE_HDR_PAGE, (uint8_t *)&hdr, sizeof(hdr));
    }

    RM_FileHandle(const RM_FileHandle &other) = delete;

    RM_FileHandle &operator=(const RM_FileHandle &other) = delete;

    bool is_record(const RecordID &rid) const;

    std::unique_ptr<RM_Record> get_record(const RecordID &rid) const;

    RecordID insert_record(uint8_t *buf);

    void delete_record(const RecordID &rid);

    void update_record(const RecordID &rid, uint8_t *buf);
};

#endif