#include "rm_handler.h"
#include "db_structs.h"
#include <cassert>

bool RM_FileHandle::is_record(const RecordID record_id) const
{
    RM_PageHandle page_handle = fetch_page(record_id.page_number);
    return Bitmap::test(page_handle.bitmap,record_id.slot_number);
}

std::unique_ptr<RM_Record> RM_FileHandle::get_record(const RecordID &rid)const{
    auto record = std::make_unique<RM_Record>(this->file_header.record_size);
    RM_PageHandle page_handle = fetch_page(rid.page_number);
    if(Bitmap::test(page_handle.bitmap,rid.slot_number)==false){
        throw RecordNotFoundError(rid.page_number,rid.slot_number);
    }
    
}

RecordID RM_FileHandle::insert_record(uint8_t *buf) {
    RM_PageHandle page_handle = create_page();
    // get slot number
    int slot_no = Bitmap::first_bit(false, page_handle.bitmap, this->file_header.num_records_per_page);
    assert(slot_no < this->file_header.num_records_per_page);
    // update bitmap
    Bitmap::set(page_handle.bitmap, slot_no);
    // update page header
    page_handle.page->mark_dirty();
    page_handle.page_header->num_records++;
    if (page_handle.page_header->num_records == this->file_header.num_records_per_page) {
        // page is full
        this->file_header.first_free = page_handle.page_header->next_free;
    }
    // copy record data into slot
    uint8_t *slot = page_handle.get_slot(slot_no);
    memcpy(slot, buf, this->file_header.record_size);
    RecordID rid(page_handle.page->id.page_number, slot_no);
    return rid;
}

void RM_FileHandle::delete_record(const RecordID &rid){
    RM_PageHandle page_handle = fetch_page(rid.page_number);
    if(Bitmap::test(page_handle.bitmap,rid.slot_number)==false){
        throw RecordNotFoundError(rid.page_number,rid.slot_number);
    }
    Bitmap::reset(page_handle.bitmap,rid.slot_number);
    page_handle.page_header->num_records--;
}

void RM_FileHandle::update_record(const RecordID &rid,uint8_t * buf){
    RM_PageHandle page_handle = fetch_page(rid.page_number);
    if(Bitmap::test(page_handle.bitmap,rid.slot_number)==false){
        throw RecordNotFoundError(rid.page_number,rid.slot_number);
    }
    page_handle.page->mark_dirty();
    uint8_t * slot = page_handle.get_slot(rid.slot_number);
    memcpy(slot,buf,this->file_header.record_size);
}

RM_PageHandle RM_FileHandle::fetch_page(int page_number) const{
    assert(page_number<this->file_header.num_pages);
    Page * page = PF_Manager::pager.fetch_page(this->fd,page_number);
    RM_PageHandle page_handle(&(this->file_header),page);
    return page_handle;
}

RM_PageHandle RM_FileHandle::create_page(){
    if(this->file_header.first_free==RM_NO_PAGE){
        // There are no free pages so we will allocate a new page
        Page * page = PF_Manager::pager.create_page(this->fd,this->file_header.num_pages);
        //Initialise the page handle
        RM_PageHandle page_handle = RM_PageHandle(&(this->file_header),page);
        page_handle.page_header->num_records = 0;
        page_handle.page_header->next_free = RM_NO_PAGE;
        Bitmap::init(page_handle.bitmap,this->file_header.bitmap_size);
        //Update the file header
        this->file_header.num_pages++;
        this->file_header.first_free = page->id.page_number;
        return page_handle;
    }
    else{
        //Fetch the first free page
        RM_PageHandle page_handle = fetch_page(this->file_header.first_free);
        return page_handle;
    }
}

void RM_FileHandle::release_page(RM_PageHandle &page_handle){
    page_handle.page_header->next_free = this->file_header.first_free;
    this->file_header.first_free = page_handle.page->id.page_number;
}