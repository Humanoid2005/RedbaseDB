#ifndef NDX_MANAGER_H
#define NDX_MANAGER_H

#include "ndx_defs.h"
#include "ndx_handler.h"
#include <memory>
#include <string>

class IndexManager{
public:
    static std::string get_index_name(const std::string &filename,int index_number){
        return filename + "."+std::to_string(index_number)+".ndx";
    }

    static bool exists(const std::string &filename,int index_number);

    static void create_index(const std::string &filename,int index_number, ColumnType col_type,int col_len);

    static void destroy_index(const std::string &filename,int index_number);

    static std::unique_ptr<IndexHandle> open_index(const std::string &filename,int index_number);

    static void close_index(const IndexHandle * index_handle);
};

#endif