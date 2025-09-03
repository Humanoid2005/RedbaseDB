#include "rm_manager.h"
#include "../db_error.h"

void RM_Manager::create_file(const std::string &filename,int record_size){
    if(record_size < 1 || record_size > RM_MAX_RECORD_SIZE){
        throw InvalidRecordSizeError(record_size);
    }

    PF_Manager::create_file(filename);
    int fd = PF_Manager::open_file(filename);

    RM_FileHeader file_header;
    file_header.record_size = record_size;
    file_header.num_pages = 1;
    file_header.first_free = RM_NO_PAGE;

    file_header.num_records_per_page = (Bitmap::WIDTH * (PAGE_SIZE-1-(int)sizeof(RM_PageHeader))+1)/(1+record_size*Bitmap::WIDTH);
    file_header.bitmap_size = (file_header.num_records_per_page + Bitmap::WIDTH - 1)/Bitmap::WIDTH;
    PF_Pager::write_page(fd,RM_FILE_HDR_PAGE,(uint8_t*)&file_header,sizeof(file_header));
    PF_Manager::close_file(fd);
}

void RM_Manager::destroy_file(const std::string &filename){
    PF_Manager::destroy_file(filename);
}

std::unique_ptr<RM_FileHandle> RM_Manager::open_file(const std::string &filename){
    int fd = PF_Manager::open_file(filename);
    return std::make_unique<RM_FileHandle>(fd);
}

void RM_Manager::close_file(const RM_FileHandle* file_handle){
    PF_Pager::write_page(file_handle->fd,RM_FILE_HDR_PAGE,(uint8_t*)&file_handle->file_header,sizeof(file_handle->file_header));
    PF_Manager::close_file(file_handle->fd);
}