#include "ndx_scanner.h"
#include "ndx_handler.h"
#include <cassert>

void IndexScanner::next(){
    assert(!is_end());
    IndexNodeHandle node = this->index_handle->get_node(this->current_index.page_number);
    assert(node.header->is_leaf);
    assert(this->current_index.slot_number < node.header->num_key);
    // increment slot no
    this->current_index.slot_number++;
    if (this->current_index.page_number != this->index_handle->index_header.last_leaf && this->current_index.slot_number == node.header->num_key) {
        // go to next leaf
        this->current_index.slot_number = 0;
        this->current_index.page_number = node.header->next_leaf;
    }    
}

RecordID IndexScanner::current_record_id() const { 
    return this->index_handle->get_record_id(this->current_index); 
}