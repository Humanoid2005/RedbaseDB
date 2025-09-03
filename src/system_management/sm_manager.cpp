#include "sm_manager.h"
#include "indexing_handler/ndx.h"
#include "../record_logger.h"
#include "../db_structs.h"
#include "record_manager/rm.h"
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>

bool SM_Manager::is_dir(const std::string &db_name){
    struct stat st;
    return stat(db_name.c_str(),&st)==0 && S_ISDIR(st.st_mode); 
}

void SM_Manager::use_db(const std::string &db_name){
    if(is_dir(db_name)==false){
        throw DatabaseNotFoundError(db_name);
    }   
    int change_dir = chdir(db_name.c_str());
    if(change_dir!=0){
        throw UnixError();
    }
    SM_Manager::cwdb = db_name;
}

void SM_Manager::exit_db(){
    std::string cwdb;
    char * cwd;
    char buffer[1024];
    cwd = getcwd(buffer,sizeof(buffer));
    if(cwd!=NULL){
        cwdb.assign(cwd);
    }
    else{
        throw UnixError();
    }

    if(cwdb==SM_Manager::cwdb){
        int change_dir = chdir("..");
        if(change_dir!=0){
            throw UnixError();
        }
    }
    else{
        throw DatabaseNotFoundError(cwdb);
    }
}

void SM_Manager::create_db(const std::string &db_name){
    if(is_dir(db_name)==true){
        throw DatabaseExistsError(db_name);
    }

    int make_dir = mkdir(db_name.c_str(),S_IRWXU);
    if(make_dir!=0){
        throw UnixError();
    }

    use_db(db_name);
    DB_MetaData db_metadata;
    db_metadata.db_name = db_name;
    std::ofstream ofs(db_name+DB_METADATA_NAME);
    ofs<<db_metadata;
    exit_db();
}

void SM_Manager::drop_db(const std::string &db_name){
    if(is_dir(db_name)==false){
        throw DatabaseNotFoundError(db_name);
    }
    exit_db();
    int remover = rmdir(db_name.c_str());
    if(remover!=0){
        throw UnixError();
    }
}

void SM_Manager::open_db(const std::string &db_name){
    if(is_dir(db_name)==false){
        throw DatabaseNotFoundError(db_name);
    }

    use_db(db_name);
    SM_Manager::cwdb = db_name;
    std::ifstream ifs(db_name+DB_METADATA_NAME);
    ifs >> SM_Manager::db_metadata;

    for(auto &entry: db_metadata.tables){
        auto &table = entry.second;
        SM_Manager::file_handle_map[table.table_name] = RM_Manager::open_file(table.table_name);
        for(size_t i=0;i<table.columns.size();i++){
            auto &column = table.columns[i];
            if(column.index==true){
                auto index_name = IndexManager::get_index_name(table.table_name,i);
                assert(index_handle_map.count(index_name)==0);
                index_handle_map[index_name] = IndexManager::open_index(table.table_name,i);
            }
        }
    }
}

void SM_Manager::close_db(){
    std::ofstream ofs(SM_Manager::cwdb+DB_METADATA_NAME);
    ofs << SM_Manager::db_metadata;
    SM_Manager::db_metadata.db_name.clear();
    SM_Manager::db_metadata.tables.clear();

    for(auto &entry: file_handle_map){
        RM_Manager::close_file(entry.second.get());
    }
    file_handle_map.clear();

    for(auto &entry: index_handle_map){
        IndexManager::close_index(entry.second.get());
    }
    index_handle_map.clear();

    exit_db();
}

void SM_Manager::show_tables(){
    auto start = get_timestamp();
    RecordLogger printer(1);
    printer.print_separator();
    printer.print_record({"Tables"});
    printer.print_separator();
    for(auto &entry: SM_Manager::db_metadata.tables){
        auto &table = entry.second;
        printer.print_record({table.table_name});
    }
    printer.print_separator();
    auto end = get_timestamp();
    auto duration = std::chrono::duration<double, std::milli>(end - start);
    printer.print_query_time(duration);
}

void SM_Manager::desc_table(const std::string &table_name){
    Table_MetaData &table = SM_Manager::db_metadata.get_table(table_name);
    std::vector<std::string> captions = {"Field", "Type", "Index"};
    RecordLogger printer(captions.size());
    printer.print_separator();
    printer.print_record(captions);
    for(auto &column : table.columns){
        std::vector<std::string> field_info = {column.column_name,convertColTypeToString(column.type),column.index?"YES":"NO"};
        printer.print_record(field_info);
    }
    printer.print_separator();
}

void SM_Manager::create_table(const std::string &table_name,const std::vector<Column>&column_definations){
    if(SM_Manager::db_metadata.is_table(table_name)==true){
        throw TableExistsError(table_name);
    }

    int current_offset = 0;
    Table_MetaData table;
    table.table_name = table_name;
    for(auto &column_defination: column_definations){
        Column_MetaData column(table_name,column_defination.column_name,column_defination.type,column_defination.length,current_offset,false);
        current_offset += column_defination.length;
        table.columns.push_back(column);
    }
    int record_size = current_offset;
    RM_Manager::create_file(table_name,record_size);
    SM_Manager::db_metadata.tables[table_name] = table;
    file_handle_map[table_name] = RM_Manager::open_file(table_name);
}

void SM_Manager::drop_table(const std::string &table_name){
    Table_MetaData &table = SM_Manager::db_metadata.get_table(table_name);
    RM_Manager::close_file(file_handle_map.at(table_name).get());
    RM_Manager::destroy_file(table_name);

    for(auto &column: table.columns){
        if(column.index==true){
            SM_Manager::drop_index(table_name,column.column_name);
        }
    }
    SM_Manager::db_metadata.tables.erase(table_name);
    SM_Manager::file_handle_map.erase(table_name);
}

void SM_Manager::create_index(const std::string &table_name,const std::string &column_name){
    Table_MetaData &table = SM_Manager::db_metadata.get_table(table_name);
    auto col = table.get_column(column_name);
    if (col->index) {
        throw IndexExistsError(table_name, column_name);
    }
    // Create index file
    int col_idx = col - table.columns.begin();
    IndexManager::create_index(table_name, col_idx, col->type, col->length);
    // Open index file
    auto ih = IndexManager::open_index(table_name, col_idx);
    // Get record file handle
    auto fh = SM_Manager::file_handle_map.at(table_name).get();
    // Index all records into 
    RM_Scanner rm_scan(fh);
    for (; !rm_scan.is_end(); rm_scan.next()) {
        auto rec = fh->get_record(rm_scan.current_record_id());
        const uint8_t *key = rec->data + col->offset;
        ih->insert_entry(key, rm_scan.current_record_id());
    }
    // Store index handle
    auto index_name = IndexManager::get_index_name(table_name, col_idx);
    assert(SM_Manager::index_handle_map.count(index_name) == 0);
    SM_Manager::index_handle_map[index_name] = std::move(ih);
    // Mark column index as created
    col->index = true;
}

void SM_Manager::drop_index(const std::string &table_name, const std::string &column_name) {
    Table_MetaData &table = SM_Manager::db_metadata.tables[table_name];
    auto col = table.get_column(column_name);
    if (!col->index) {
        throw IndexNotFoundError(table_name, column_name);
    }
    int col_idx = col - table.columns.begin();
    auto index_name = IndexManager::get_index_name(table_name, col_idx);
    IndexManager::close_index(SM_Manager::index_handle_map.at(index_name).get());
    IndexManager::destroy_index(table_name, col_idx);
    SM_Manager::index_handle_map.erase(index_name);
    col->index = false;
}