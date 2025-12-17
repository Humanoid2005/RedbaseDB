#ifndef INTERPRETER_DEFS_H
#define INTERPRETER_DEFS_H

#include "database_engine/query_language/ql_defs.h"
#include "database_engine/system_management/sm_defs.h"
#include <string>
#include <variant>
#include <vector>

// Enum for command types
enum CommandType {
    CMD_HELP,
    CMD_SHOW_TABLES,
    CMD_SHOW_DATABASES,
    CMD_CREATE_DATABASE,
    CMD_DROP_DATABASE,
    CMD_USE_DATABASE,
    CMD_DESC_TABLE,
    CMD_CREATE_TABLE,
    CMD_DROP_TABLE,
    CMD_CREATE_INDEX,
    CMD_DROP_INDEX,
    CMD_INSERT,
    CMD_DELETE,
    CMD_UPDATE,
    CMD_SELECT
};

// Help result
struct HelpResult {
    std::string help_text;
    
    HelpResult() = default;
    HelpResult(std::string text) : help_text(std::move(text)) {}
};

// Database operation results
struct ShowDatabasesResult {
    std::vector<std::string> databases;
    
    void add_database(const std::string& db_name) {
        databases.push_back(db_name);
    }
    
    size_t count() const { return databases.size(); }
};

struct DatabaseOperationResult {
    std::string db_name;
    bool success;
    std::string message;
    
    DatabaseOperationResult() : success(false) {}
    DatabaseOperationResult(std::string name, bool succ, std::string msg = "")
        : db_name(std::move(name)), success(succ), message(std::move(msg)) {}
};

// DDL operation results
struct DDLResult {
    std::string object_type;  // "table" or "index"
    std::string object_name;
    bool success;
    std::string message;
    
    DDLResult() : success(false) {}
    DDLResult(std::string type, std::string name, bool succ, std::string msg = "")
        : object_type(std::move(type)), object_name(std::move(name)), success(succ), message(std::move(msg)) {}
};

// Unified interpreter result
struct InterpreterResult {
    CommandType command_type;
    bool success;
    std::string error_message;
    
    // Use variant to hold different result types
    std::variant<
        std::monostate,           // Empty/no data
        HelpResult,
        ShowTablesResult,
        ShowDatabasesResult,
        DatabaseOperationResult,
        DescTableResult,
        DDLResult,
        InsertResult,
        DeleteResult,
        UpdateResult,
        SelectResult
    > data;
    
    InterpreterResult() : command_type(CMD_HELP), success(true) {}
    
    template<typename T>
    InterpreterResult(CommandType type, bool succ, T result_data)
        : command_type(type), success(succ), data(std::move(result_data)) {}
    
    InterpreterResult(CommandType type, bool succ, std::string error = "")
        : command_type(type), success(succ), error_message(std::move(error)) {}
};

#endif
