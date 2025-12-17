#include "sm_manager.h"
#include "database_engine/index_handler/ndx.h"
#include "database_engine/record_manager/rm.h"
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include "database_engine/error.h"
#include <assert.h>
#include <algorithm>

std::string SM_Manager::current_db_name;
DB_Metadata SM_Manager::db;
std::map<std::string, std::unique_ptr<RM_FileHandle>> SM_Manager::fhs;
std::map<std::string, std::unique_ptr<IndexHandle>> SM_Manager::ihs;
std::string SM_Manager::base_dir;

void SM_Manager::initialize_base_dir() {
    if (base_dir.empty()) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            base_dir = std::string(cwd);
        }
    }
}

bool SM_Manager::is_dir(const std::string &db_name) {
    initialize_base_dir();
    struct stat st;
    std::string db_path = base_dir + "/databases/" + db_name;
    return stat(db_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

std::string SM_Manager::get_db_path(const std::string &db_name) {
    initialize_base_dir();
    return base_dir + "/databases/" + db_name;
}

void SM_Manager::register_database(const std::string &db_name) {
    initialize_base_dir();
    // Ensure databases directory exists
    std::string db_dir = base_dir + "/databases";
    mkdir(db_dir.c_str(), 0755);
    
    // Read existing database list
    std::vector<std::string> db_list;
    std::string list_file = base_dir + "/" + DB_LIST_FILE;
    std::ifstream ifs(list_file);
    if (ifs.is_open()) {
        std::string name;
        while (std::getline(ifs, name)) {
            if (!name.empty()) {
                db_list.push_back(name);
            }
        }
        ifs.close();
    }
    
    // Add new database if not already present
    if (std::find(db_list.begin(), db_list.end(), db_name) == db_list.end()) {
        db_list.push_back(db_name);
    }
    
    // Write back
    std::ofstream ofs(list_file);
    for (const auto& name : db_list) {
        ofs << name << '\n';
    }
}

void SM_Manager::unregister_database(const std::string &db_name) {
    initialize_base_dir();
    std::vector<std::string> db_list;
    std::string list_file = base_dir + "/" + DB_LIST_FILE;
    std::ifstream ifs(list_file);
    if (ifs.is_open()) {
        std::string name;
        while (std::getline(ifs, name)) {
            if (!name.empty() && name != db_name) {
                db_list.push_back(name);
            }
        }
        ifs.close();
    }
    
    std::ofstream ofs(list_file);
    for (const auto& name : db_list) {
        ofs << name << '\n';
    }
}

std::vector<std::string> SM_Manager::list_databases() {
    initialize_base_dir();
    std::vector<std::string> db_list;
    std::string list_file = base_dir + "/" + DB_LIST_FILE;
    std::ifstream ifs(list_file);
    if (ifs.is_open()) {
        std::string name;
        while (std::getline(ifs, name)) {
            if (!name.empty()) {
                db_list.push_back(name);
            }
        }
    }
    return db_list;
}

std::string SM_Manager::get_current_db() {
    if (current_db_name.empty()) {
        throw InternalError("No database is currently open");
    }
    return current_db_name;
}

void SM_Manager::create_db(const std::string &db_name) {
    initialize_base_dir();
    if (is_dir(db_name)) {
        throw DatabaseExistsError(db_name);
    }
    
    // Ensure databases directory exists
    std::string db_dir = base_dir + "/databases";
    mkdir(db_dir.c_str(), 0755);
    
    // Create database subdirectory
    std::string db_path = get_db_path(db_name);
    if (mkdir(db_path.c_str(), 0755) < 0) {
        throw UnixError();
    }
    
    // Create the system catalogs
    DB_Metadata new_db;
    new_db.name = db_name;
    std::ofstream ofs(db_path + "/" + DB_META_NAME);
    ofs << new_db;
    ofs.close();
    
    // Register the database
    register_database(db_name);
}

void SM_Manager::drop_db(const std::string &db_name) {
    initialize_base_dir();
    if (!is_dir(db_name)) {
        throw DatabaseNotFoundError(db_name);
    }
    
    // Cannot drop currently open database
    if (db_name == current_db_name) {
        throw InternalError("Cannot drop currently open database '" + db_name + "'. Close it first.");
    }
    
    std::string db_path = get_db_path(db_name);
    std::string cmd = "rm -rf \"" + db_path + "\"";
    if (system(cmd.c_str()) < 0) {
        throw UnixError();
    }
    
    // Unregister the database
    unregister_database(db_name);
}

void SM_Manager::open_db(const std::string &db_name) {
    initialize_base_dir();
    if (!is_dir(db_name)) {
        throw DatabaseNotFoundError(db_name);
    }
    
    // Close current database if any is open
    if (!current_db_name.empty()) {
        close_db();
    }
    
    std::string db_path = get_db_path(db_name);
    
    // cd to database dir
    if (chdir(db_path.c_str()) < 0) {
        throw UnixError();
    }
    
    // Load meta
    std::ifstream ifs(DB_META_NAME);
    ifs >> db;
    ifs.close();
    
    current_db_name = db_name;
    
    // Open all record files & index files
    for (auto &entry : db.tabs) {
        auto &tab = entry.second;
        fhs[tab.name] = RM_Manager::open_file(tab.name);
        for (size_t i = 0; i < tab.cols.size(); i++) {
            auto &col = tab.cols[i];
            if (col.index) {
                auto index_name = IndexManager::get_index_name(tab.name, i);
                assert(ihs.count(index_name) == 0);
                ihs[index_name] = IndexManager::open_index(tab.name, i);
            }
        }
    }
}

void SM_Manager::close_db() {
    if (current_db_name.empty()) {
        throw InternalError("No database is currently open");
    }
    
    // Dump meta
    std::ofstream ofs(DB_META_NAME);
    ofs << db;
    ofs.close();
    
    db.name.clear();
    db.tabs.clear();
    
    // Close all record files
    for (auto &entry : fhs) {
        RM_Manager::close_file(entry.second.get());
    }
    fhs.clear();
    
    // Close all index files
    for (auto &entry : ihs) {
        IndexManager::close_index(entry.second.get());
    }
    ihs.clear();
    
    // Navigate back to root
    if (chdir("../..") < 0) {
        throw UnixError();
    }
    
    current_db_name.clear();
}

ShowTablesResult SM_Manager::show_tables() {
    ShowTablesResult result;
    
    for (auto &entry : db.tabs) {
        auto &tab = entry.second;
        result.add_table(tab.name);
    }
    
    return result;
}

DescTableResult SM_Manager::desc_table(const std::string &tab_name) {
    Table_Metadata &tab = db.get_table(tab_name);
    
    DescTableResult result;
    result.table_name = tab_name;
    
    // Collect column information
    for (auto &col : tab.cols) {
        result.add_column(col.name, ColumnType2str(col.type), col.index ? "YES" : "NO");
    }
    
    return result;
}

void SM_Manager::create_table(const std::string &tab_name, const std::vector<ColumnInfo> &col_defs) {
    if (db.is_table(tab_name)) {
        throw TableExistsError(tab_name);
    }
    // Create table meta
    int curr_offset = 0;
    Table_Metadata tab;
    tab.name = tab_name;
    for (auto &col_def : col_defs) {
        Column_Metadata col(tab_name, col_def.name, col_def.type, col_def.len, curr_offset, false);
        curr_offset += col_def.len;
        tab.cols.push_back(col);
    }
    // Create & open record file
    int record_size = curr_offset;
    RM_Manager::create_file(tab_name, record_size);
    db.tabs[tab_name] = tab;
    fhs[tab_name] = RM_Manager::open_file(tab_name);
}

void SM_Manager::drop_table(const std::string &tab_name) {
    // Find table index in db meta
    Table_Metadata &tab = db.get_table(tab_name);
    // Close & destroy record file
    RM_Manager::close_file(fhs.at(tab_name).get());
    RM_Manager::destroy_file(tab_name);
    // Close & destroy index file
    for (auto &col : tab.cols) {
        if (col.index) {
            SM_Manager::drop_index(tab_name, col.name);
        }
    }
    db.tabs.erase(tab_name);
    fhs.erase(tab_name);
}

void SM_Manager::create_index(const std::string &tab_name, const std::string &col_name) {
    Table_Metadata &tab = db.get_table(tab_name);
    auto col = tab.get_col(col_name);
    if (col->index) {
        throw IndexExistsError(tab_name, col_name);
    }
    // Create index file
    int col_idx = col - tab.cols.begin();
    IndexManager::create_index(tab_name, col_idx, col->type, col->len);
    // Open index file
    auto ih = IndexManager::open_index(tab_name, col_idx);
    // Get record file handle
    auto fh = fhs.at(tab_name).get();
    // Index all records into index
    for (RM_Iterator rm_scan(fh); !rm_scan.is_end(); rm_scan.next()) {
        auto rec = fh->get_record(rm_scan.get_RecordID());
        const uint8_t *key = rec->data + col->offset;
        ih->insert_entry(key,rm_scan.get_RecordID());
    }
    // Store index handle
    auto index_name = IndexManager::get_index_name(tab_name, col_idx);
    assert(ihs.count(index_name) == 0);
    ihs[index_name] = std::move(ih);
    // Mark column index as created
    col->index = true;
}

void SM_Manager::drop_index(const std::string &tab_name, const std::string &col_name) {
    Table_Metadata &tab = db.tabs[tab_name];
    auto col = tab.get_col(col_name);
    if (!col->index) {
        throw IndexNotFoundError(tab_name, col_name);
    }
    int col_idx = col - tab.cols.begin();
    auto index_name = IndexManager::get_index_name(tab_name, col_idx);
    IndexManager::close_index(ihs.at(index_name).get());
    IndexManager::destroy_index(tab_name, col_idx);
    ihs.erase(index_name);
    col->index = false;
}
