#ifndef SM_MANAGER_H
#define SM_MANAGER_H

#include "../indexing_handler/ndx.h"
#include "../record_manager/rm.h"
#include "sm_structs.h"
#include "sm_meta.h"

class Column{
public:
    std::string column_name;
    ColumnType type;
    int length;

    Column() = default;
    Column(std::string column_name,ColumnType type,int length){
        this->column_name = column_name;
        this->type = type;
        this->length = length;
    }
};

class SM_Manager{
public:
    static DB_MetaData db_metadata;
    static std::string cwdb;
    static std::map<std::string,std::unique_ptr<RM_FileHandle>>file_handle_map;
    static std::map<std::string,std::unique_ptr<IndexHandle>>index_handle_map;

    static bool is_dir(const std::string &db_name);

    static void create_db(const std::string &db_name);

    static void use_db(const std::string &db_name);

    static void exit_db();

    static void drop_db(const std::string &db_name);

    static void open_db(const std::string &db_name);

    static void close_db();

    static void show_tables();

    static void desc_table(const std::string &table_name);

    static void create_table(const std::string &table_name,const std::vector<Column>&columns);

    static void drop_table(const std::string &table_name);

    static void create_index(const std::string &table_name,const std::string &column_name);

    static void drop_index(const std::string &table_name,const std::string &column_name);
};

#endif