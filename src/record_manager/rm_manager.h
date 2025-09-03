#ifndef RM_MANAGER_H
#define RM_MANAGER_h

#include "bitmap.h"
#include "rm_structs.h"
#include "rm_handler.h"

class RM_Manager{
public:
    static void create_file(const std::string &filename,int record_size);

    static void destroy_file(const std::string &filename);

    static std::unique_ptr<RM_FileHandle> open_file(const std::string &filename);

    static void close_file(const RM_FileHandle * file_handle);
};

#endif