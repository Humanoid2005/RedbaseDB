#include "rm_file_handle.h"
#include <cassert>

bool RM_FileHandle::is_record(const RecordID &rid) const {
    RM_PageHandle ph = fetch_page(rid.page_no);
    return Bitmap::test(ph.bitmap, rid.slot_no);
}

std::unique_ptr<RM_Record> RM_FileHandle::get_record(const RecordID &rid) const {
    auto record = std::make_unique<RM_Record>(hdr.record_size);
    RM_PageHandle ph = fetch_page(rid.page_no);
    if (!Bitmap::test(ph.bitmap, rid.slot_no)) {
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }
    uint8_t *slot = ph.get_slot(rid.slot_no);
    memcpy(record->data, slot, hdr.record_size);
    record->size = hdr.record_size;
    return record;
}

RecordID RM_FileHandle::insert_record(uint8_t *buf) {
    RM_PageHandle ph = create_page();
    // get slot number
    int slot_no = Bitmap::first_bit(false, ph.bitmap, hdr.num_records_per_page);
    assert(slot_no < hdr.num_records_per_page);
    // update bitmap
    Bitmap::set(ph.bitmap, slot_no);
    // update page header
    ph.page->mark_dirty();
    ph.hdr->num_records++;
    if (ph.hdr->num_records == hdr.num_records_per_page) {
        // page is full
        hdr.first_free = ph.hdr->next_free;
    }
    // copy record data into slot
    uint8_t *slot = ph.get_slot(slot_no);
    memcpy(slot, buf, hdr.record_size);
    RecordID rid(ph.page->id.page_no, slot_no);
    return rid;
}

void RM_FileHandle::delete_record(const RecordID &rid) {
    RM_PageHandle ph = fetch_page(rid.page_no);
    if (!Bitmap::test(ph.bitmap, rid.slot_no)) {
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }
    ph.page->mark_dirty();
    if (ph.hdr->num_records == hdr.num_records_per_page) {
        // originally full, now available for new record
        release_page(ph);
    }
    Bitmap::reset(ph.bitmap, rid.slot_no);
    ph.hdr->num_records--;
}

void RM_FileHandle::update_record(const RecordID &rid, uint8_t *buf) {
    RM_PageHandle ph = fetch_page(rid.page_no);
    if (!Bitmap::test(ph.bitmap, rid.slot_no)) {
        throw RecordNotFoundError(rid.page_no, rid.slot_no);
    }
    ph.page->mark_dirty();
    uint8_t *slot = ph.get_slot(rid.slot_no);
    memcpy(slot, buf, hdr.record_size);
}

RM_PageHandle RM_FileHandle::fetch_page(int page_no) const {
    assert(page_no < hdr.num_pages);
    Page *page = PF_Manager::pager.fetch_page(fd, page_no);
    RM_PageHandle ph(&hdr, page);
    return ph;
}

RM_PageHandle RM_FileHandle::create_page() {
    if (hdr.first_free == RM_NO_PAGE) {
        // No free pages. Need to allocate a new page.
        Page *page = PF_Manager::pager.create_page(fd, hdr.num_pages);
        // Init page handle
        RM_PageHandle ph = RM_PageHandle(&hdr, page);
        ph.hdr->num_records = 0;
        ph.hdr->next_free = RM_NO_PAGE;
        Bitmap::init(ph.bitmap, hdr.bitmap_size);
        // Update file header
        hdr.num_pages++;
        hdr.first_free = page->id.page_no;
        return ph;
    } else {
        // Fetch the first free page.
        RM_PageHandle ph = fetch_page(hdr.first_free);
        return ph;
    }
}

void RM_FileHandle::release_page(RM_PageHandle &ph) {
    ph.hdr->next_free = hdr.first_free;
    hdr.first_free = ph.page->id.page_no;
}