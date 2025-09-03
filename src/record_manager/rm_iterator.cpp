#include "rm_iterator.h"
#include "rm_file_handle.h"
#include <cassert>

RM_Iterator::RM_Iterator(const RM_FileHandle *fh) : _fh(fh) {
    _rid = RecordID(RM_FIRST_RECORD_PAGE, -1);
    next();
}

void RM_Iterator::next() {
    assert(!is_end());
    while (_rid.page_no < _fh->hdr.num_pages) {
        RM_PageHandle ph = _fh->fetch_page(_rid.page_no);
        _rid.slot_no = Bitmap::next_bit(true, ph.bitmap, _fh->hdr.num_records_per_page, _rid.slot_no);
        if (_rid.slot_no < _fh->hdr.num_records_per_page) {
            return;
        }
        _rid.slot_no = -1;
        _rid.page_no++;
    }
    // next record not found
    _rid.page_no = RM_NO_PAGE;
}