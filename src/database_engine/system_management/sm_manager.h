#pragma once

#include "database_engine/index_handler/ndx.h"
#include "database_engine/record_manager/rm.h"
#include "database_engine/system_management/sm_defs.h"
#include "database_engine/system_management/sm_meta.h"

struct ColumnInfo {
    std::string name; // Column name
    ColumnType type;     // Type of column
    int len;          // Length of column

    ColumnInfo() = default;
    ColumnInfo(std::string name_, ColumnType type_, int len_) : name(std::move(name_)), type(type_), len(len_) {}
};

class SM_Manager {
  public:
    static std::string current_db_name;  // Track which database is currently open
    static std::string base_dir;  // Base directory for all databases
    static DB_Metadata db;
    static std::map<std::string, std::unique_ptr<RM_FileHandle>> fhs;
    static std::map<std::string, std::unique_ptr<IndexHandle>> ihs;

    // Database management
    static void initialize_base_dir();
    
    static bool is_dir(const std::string &db_name);
    
    static std::string get_db_path(const std::string &db_name);

    static void create_db(const std::string &db_name);

    static void drop_db(const std::string &db_name);

    static void open_db(const std::string &db_name);

    static void close_db();
    
    static std::string get_current_db();
    
    static std::vector<std::string> list_databases();

    // Table management
    static ShowTablesResult show_tables();

    static DescTableResult desc_table(const std::string &tab_name);

    static void create_table(const std::string &tab_name, const std::vector<ColumnInfo> &col_defs);

    static void drop_table(const std::string &tab_name);

    // Index management
    static void create_index(const std::string &tab_name, const std::string &col_name);

    static void drop_index(const std::string &tab_name, const std::string &col_name);

  private:
    static void register_database(const std::string &db_name);
    static void unregister_database(const std::string &db_name);
};
