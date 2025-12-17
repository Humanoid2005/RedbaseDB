#ifndef QL_DEFS_H
#define QL_DEFS_H

#include "database_engine/db_defs.h"
#include <string>
#include <vector>

// Output structures for query results (similar to SM_Manager output structures)

struct RowValue {
    std::string value;
    
    RowValue() = default;
    RowValue(std::string val) : value(std::move(val)) {}
};

struct SelectRow {
    std::vector<RowValue> values;
    
    void add_value(const std::string& val) {
        values.emplace_back(val);
    }
    
    size_t count() const { return values.size(); }
};

struct SelectResult {
    std::vector<std::string> column_names;
    std::vector<SelectRow> rows;
    
    void add_column(const std::string& col_name) {
        column_names.push_back(col_name);
    }
    
    void add_row(const SelectRow& row) {
        rows.push_back(row);
    }
    
    size_t row_count() const { return rows.size(); }
    size_t column_count() const { return column_names.size(); }
};

struct InsertResult {
    std::string table_name;
    bool success;
    std::string message;
    
    InsertResult() : success(false) {}
    InsertResult(std::string table, bool succ, std::string msg = "")
        : table_name(std::move(table)), success(succ), message(std::move(msg)) {}
};

struct DeleteResult {
    std::string table_name;
    size_t affected_rows;
    bool success;
    std::string message;
    
    DeleteResult() : affected_rows(0), success(false) {}
    DeleteResult(std::string table, size_t rows, bool succ, std::string msg = "")
        : table_name(std::move(table)), affected_rows(rows), success(succ), message(std::move(msg)) {}
};

struct UpdateResult {
    std::string table_name;
    size_t affected_rows;
    bool success;
    std::string message;
    
    UpdateResult() : affected_rows(0), success(false) {}
    UpdateResult(std::string table, size_t rows, bool succ, std::string msg = "")
        : table_name(std::move(table)), affected_rows(rows), success(succ), message(std::move(msg)) {}
};

#endif
