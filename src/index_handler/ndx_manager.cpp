#include "ndx_manager.h"
#include <cassert>

bool IndexManager::exists(const std::string &filename,int index_number){
    auto index_name = get_index_name(filename,index_number);
    return PF_Manager::is_file(index_name);
}

void IndexManager::create_index(const std::string &filename,int index_number,ColumnType col_type,int col_len){
    std::string index_name = get_index_name(filename,index_number);
    assert(index_number>=0);
    //Create the index file
    PF_Manager::create_file(index_name);
    //Open the index file
    int fd = PF_Manager::open_file(index_name);
    //Create the file header and write to the file
    // index file = page_header + (attribute + record_id)*(n+1) Since we reserver one slot for convienient insertion and deletion 
    // |index_file| <= PAGE_SIZE

    if(col_len>NDX_MAX_COL_LEN){
        throw InvalidColLengthError(col_len);
    }
    /*
    Total usable space = PAGE_SIZE - Header size
    Space per entry = col_len (d_type size) + sizeof(RecordID) -1 (since for n keys we have n+1 block pointers in B+ trees)
    Order = (Total usable space)/(space per entry)
    */
    int btree_order = (int)((PAGE_SIZE - sizeof(IndexPageHeader)) / (col_len + sizeof(RecordID)) - 1);
    assert(btree_order>2);

    int key_offset = sizeof(IndexPageHeader);//since keys will be inserted after header
    int record_offset = key_offset + (btree_order+1)*col_len;//records will be inserted after keys

    //Create the file header and write to the file
    IndexFileHeader file_header(NDX_NO_PAGE,NDX_INIT_NUM_PAGES,NDX_INIT_ROOT_PAGE,col_type,col_len,btree_order,key_offset,record_offset,NDX_INIT_ROOT_PAGE,NDX_INIT_ROOT_PAGE);
    PF_Pager::write_page(fd,NDX_FILE_HDR_PAGE,(const u_int8_t*)&file_header,sizeof(file_header));

    //Create a new leaf list header page and write to the file
    static u_int8_t leaf_page_buffer[PAGE_SIZE];
    IndexPageHeader*  leaf_page_header = (IndexPageHeader*)leaf_page_buffer;
    *leaf_page_header = IndexPageHeader(NDX_NO_PAGE,NDX_NO_PAGE,0,0,true,NDX_INIT_ROOT_PAGE,NDX_INIT_ROOT_PAGE);
    PF_Pager::write_page(fd,NDX_LEAF_HEADER_PAGE,leaf_page_buffer,PAGE_SIZE);

    //Create root node and write to the file
    static u_int8_t root_page_buffer[PAGE_SIZE];
    IndexPageHeader * root_page_header = (IndexPageHeader*)root_page_buffer;
    *root_page_header = IndexPageHeader(NDX_NO_PAGE,NDX_NO_PAGE,0,0,true,NDX_LEAF_HEADER_PAGE,NDX_LEAF_HEADER_PAGE);
    PF_Pager::write_page(fd,NDX_INIT_ROOT_PAGE,root_page_buffer,PAGE_SIZE);

    //Closing the index file
    PF_Manager::close_file(fd);
}

void IndexManager::destroy_index(const std::string &filename,int index_number){
    std::string index_name = get_index_name(filename,index_number);
    PF_Manager::destroy_file(index_name);
}

std::unique_ptr<IndexHandle> IndexManager::open_index(const std::string &filename,int index_number){
    std::string index_name = get_index_name(filename,index_number);
    int fd = PF_Manager::open_file(index_name);
    return std::make_unique<IndexHandle>(fd);
}

void IndexManager::close_index(const IndexHandle * index_handle){
    PF_Pager::write_page(index_handle->fd,NDX_FILE_HDR_PAGE,(const u_int8_t *)&index_handle->index_header,sizeof(index_handle->index_header));
    PF_Manager::close_file(index_handle->fd);
}