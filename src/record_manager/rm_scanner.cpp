#include "rm_scanner.h"
#include "rm_handler.h"
#include <cassert>

RM_Scanner::RM_Scanner(const RM_FileHandle * file_handle)
{
    this->filehandle = file_handle;
    this->record_id = RecordID(RM_FIRST_RECORD_PAGE,-1);
    next();
}

void RM_Scanner::next(){
    assert(!is_end());
    while(this->record_id.page_number < this->filehandle->file_header.num_pages){
        RM_PageHandle page_handle = this->filehandle->fetch_page(this->record_id.page_number);
        this->record_id.slot_number = Bitmap::next_bit(true,page_handle.bitmap,this->filehandle->file_header.num_records_per_page,this->record_id.slot_number);
        if(this->record_id.slot_number < this->filehandle->file_header.num_records_per_page){
            return ;
        }
        this->record_id.slot_number = -1;
        this->record_id.page_number++;
    }
    //next record not found
    this->record_id.page_number = RM_NO_PAGE;
}