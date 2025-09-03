#ifndef RM_DEFS_H
#define RM_DEFS_H

#include "../db_defs.h"
#include "../page_file_handler/pf.h"

constexpr int RM_NO_PAGE = -1;
constexpr int RM_FILE_HDR_PAGE = 0;
constexpr int RM_FIRST_RECORD_PAGE = 1;
constexpr int RM_MAX_RECORD_SIZE = 512;

struct RM_FileHeader {
    int record_size;
    int num_pages;
    int num_records_per_page;
    int first_free;
    int bitmap_size;
};

struct RM_PageHeader {
    int next_free;
    int num_records;
};

struct RM_Record {
    uint8_t *data;
    int size;

    RM_Record(const RM_Record &other) = delete;

    RM_Record &operator=(const RM_Record &other) = delete;

    RM_Record(int size_) {
        size = size_;
        data = new uint8_t[size_];
    }

    ~RM_Record() { delete[] data; }
};

#endif