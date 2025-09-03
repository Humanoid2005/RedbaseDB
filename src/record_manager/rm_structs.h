#ifndef RM_STRUCTS_H
#define RM_STRUCTS_H

#include "page_file_handler/pf.h"

constexpr int RM_NO_PAGE = -1;// No page in the file
constexpr int RM_NO_SLOT = -1; // No slot in the page
constexpr int RM_FILE_HDR_PAGE = 0;// File header page
constexpr int RM_FIRST_RECORD_PAGE = 1;// First page for records
constexpr int RM_MAX_RECORD_SIZE = 512;// Maximum size of a record

class RM_FileHeader{
public:
    int record_size;
    int num_pages;
    int num_records_per_page;
    int first_free;
    int bitmap_size;
};

class RM_PageHeader{
public:
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