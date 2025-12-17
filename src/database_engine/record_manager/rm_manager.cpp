#include "rm_manager.h"

void RM_Manager::create_file(const std::string &filename, int record_size) {
    if (record_size < 1 || record_size > RM_MAX_RECORD_SIZE) {
        throw InvalidRecordSizeError(record_size);
    }
    PF_Manager::create_file(filename);
    int fd = PF_Manager::open_file(filename);

    RM_FileHeader hdr{};
    hdr.record_size = record_size;
    hdr.num_pages = 1;
    hdr.first_free = RM_NO_PAGE;
    // We have: sizeof(RM_PageHeader) + (n + 7) / 8 + n * record_size <= PAGE_SIZE
    hdr.num_records_per_page = (Bitmap::WIDTH * (PAGE_SIZE - 1 - (int)sizeof(RM_PageHeader)) + 1) / (1 + record_size * Bitmap::WIDTH);
    hdr.bitmap_size = (hdr.num_records_per_page + Bitmap::WIDTH - 1) / Bitmap::WIDTH;
    PF_Pager::write_page(fd, RM_FILE_HDR_PAGE, (uint8_t *)&hdr, sizeof(hdr));
    PF_Manager::close_file(fd);
}

void RM_Manager::destroy_file(const std::string &filename) { PF_Manager::destroy_file(filename); }

std::unique_ptr<RM_FileHandle> RM_Manager::open_file(const std::string &filename) {
    int fd = PF_Manager::open_file(filename);
    return std::make_unique<RM_FileHandle>(fd);
}

void RM_Manager::close_file(const RM_FileHandle *fh) {
    PF_Pager::write_page(fh->fd, RM_FILE_HDR_PAGE, (uint8_t *)&fh->hdr, sizeof(fh->hdr));
    PF_Manager::close_file(fh->fd);
}