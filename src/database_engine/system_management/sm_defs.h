#ifndef SM_DEFS_H
#define SM_DEFS_H
#include <string>
#include <vector>

static const std::string DB_META_NAME = "db.meta";
static const std::string DB_LIST_FILE = "databases.list";

// Output structures for show_tables and desc_table
struct TableInfo {
    std::string table_name;
    
    TableInfo() = default;
    TableInfo(std::string name) : table_name(std::move(name)) {}
};

struct ShowTablesResult {
    std::vector<TableInfo> tables;
    
    void add_table(const std::string& name) {
        tables.emplace_back(name);
    }
    
    size_t count() const { return tables.size(); }
};

struct ColumnDescription {
    std::string field_name;
    std::string type_name;
    std::string has_index;
    
    ColumnDescription() = default;
    ColumnDescription(std::string field, std::string type, std::string index)
        : field_name(std::move(field)), type_name(std::move(type)), has_index(std::move(index)) {}
};

struct DescTableResult {
    std::string table_name;
    std::vector<ColumnDescription> columns;
    
    void add_column(const std::string& field, const std::string& type, const std::string& index) {
        columns.emplace_back(field, type, index);
    }
    
    size_t count() const { return columns.size(); }
};

#endif
