#include "ndx_iterator.h"
#include "ndx_handler.h"
#include <cassert>

void IndexIterator::next(){
    assert(!is_end());
    IndexNodeHandle node = this->index_handle->get_node(this->current_index.page_number);
    assert(node.header->is_leaf);
    assert(this->current_index.slot_number < node.header->num_key);
    // increment slot no
    this->current_index.slot_number++;
    if (this->current_index.slot_number >= node.header->num_key) {
        if (node.header->next_leaf == NDX_LEAF_HEADER_PAGE || node.header->next_leaf == NDX_NO_PAGE) {
            this->current_index = this->index_handle->leaf_end();
        } else {
            // go to next leaf
            this->current_index.slot_number = 0;
            this->current_index.page_number = node.header->next_leaf;
        }
    }
}

RecordID IndexIterator::get_RecordID() const { 
    return this->index_handle->get_record_id(this->current_index); 
}