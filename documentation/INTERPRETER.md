# Interpreter Documentation

## Table of Contents
1. [Overview](#overview)
2. [Key Concepts](#key-concepts)
3. [API Reference](#api-reference)
4. [Usage Examples](#usage-examples)
5. [Result Types](#result-types)
6. [Migration Guide](#migration-guide)

---

## Overview

The **Interpreter** executes parsed SQL by converting Abstract Syntax Tree (AST) nodes into database operations and returning structured results.

**Key Features:**
- **No Console Output** - Returns structured data instead of logging
- **Type Safe** - Uses `std::variant` for compile-time checking
- **Flexible** - Application controls formatting and display
- **API Ready** - Perfect for web services, GUIs, automation

**Architecture:**
```
SQL Text → Parser → AST → Interpreter → InterpreterResult → Your Application
                              ↓
                    SM_Manager / QL_Manager
                    (Database Operations)
```

---

## Key Concepts

### What Changed?

**Before (Old Approach):**
```cpp
// Logs directly to console
Interpreter::interp_sql(ast_root);
// Output printed to stdout automatically
```

**After (New Approach):**
```cpp
// Returns structured data
InterpreterResult result = Interpreter::interp_sql(ast_root);

// Your application decides how to display
if (result.success) {
    auto& data = std::get<SelectResult>(result.data);
    // Format as needed: JSON, XML, pretty table, etc.
}
```

### Why Structured Results?

1. **Separation of Concerns** - Interpreter focuses on execution, not presentation
2. **Testability** - Easy to verify results in unit tests
3. **Flexibility** - Output can be JSON, XML, GUI widgets, etc.
4. **Embeddable** - Use in web servers, APIs, embedded systems
5. **Performance** - No string formatting overhead if not displayed

---

## API Reference

### Main Function

```cpp
InterpreterResult Interpreter::interp_sql(const std::shared_ptr<ast::TreeNode>& root);
```

**Parameters:**
- `root` - AST root node from parser

**Returns:**
- `InterpreterResult` - Structured result with command type, success status, and data

### InterpreterResult Structure

```cpp
struct InterpreterResult {
    CommandType command_type;    // What SQL command was executed
    bool success;                // Did it succeed?
    std::string error_message;   // Error details (if failed)
    std::variant<...> data;      // Actual result data (type varies by command)
};
```

### Command Types

```cpp
enum CommandType {
    CMD_HELP, CMD_SHOW_TABLES, CMD_SHOW_DATABASES,
    CMD_CREATE_DATABASE, CMD_DROP_DATABASE, CMD_USE_DATABASE,
    CMD_DESC_TABLE, CMD_CREATE_TABLE, CMD_DROP_TABLE,
    CMD_CREATE_INDEX, CMD_DROP_INDEX,
    CMD_INSERT, CMD_DELETE, CMD_UPDATE, CMD_SELECT
};
```

---

## Usage Examples

### Basic Usage

```cpp
#include "parser/parser.h"
#include "interpreter.h"
#include <iostream>

int main() {
    std::string sql = "SELECT * FROM users WHERE age > 18;";
    
    // Parse SQL
    auto ast = parse_sql(sql);
    
    // Execute
    InterpreterResult result = Interpreter::interp_sql(ast);
    
    // Check success
    if (!result.success) {
        std::cerr << "Error: " << result.error_message << "\n";
        return 1;
    }
    
    // Process based on command type
    if (result.command_type == CMD_SELECT) {
        auto& sel = std::get<SelectResult>(result.data);
        
        // Display results
        for (const auto& col : sel.column_names) {
            std::cout << col << "\t";
        }
        std::cout << "\n";
        
        for (const auto& row : sel.rows) {
            for (const auto& val : row.values) {
                std::cout << val.value << "\t";
            }
            std::cout << "\n";
        }
    }
    
    return 0;
}
```

### Pattern: Switch on Command Type

```cpp
InterpreterResult result = Interpreter::interp_sql(ast);

switch (result.command_type) {
    case CMD_SELECT: {
        auto& sel = std::get<SelectResult>(result.data);
        // Display SELECT results
        break;
    }
    
    case CMD_INSERT: {
        auto& ins = std::get<InsertResult>(result.data);
        std::cout << "✓ " << ins.message << "\n";
        break;
    }
    
    case CMD_CREATE_TABLE: {
        auto& ddl = std::get<DDLResult>(result.data);
        std::cout << "✓ Table '" << ddl.object_name << "' created\n";
        break;
    }
    
    // ... other cases
}
```

### Error Handling

```cpp
try {
    auto ast = parse_sql(sql);
    InterpreterResult result = Interpreter::interp_sql(ast);
    
    if (!result.success) {
        std::cerr << "SQL Error: " << result.error_message << "\n";
        return;
    }
    
    // Process results...
    
} catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
}
```

---

## Result Types

### Quick Reference Table

| SQL Command | CommandType | Result Type | Key Fields |
|-------------|-------------|-------------|------------|
| `HELP;` | `CMD_HELP` | `HelpResult` | `help_text` |
| `SHOW DATABASES;` | `CMD_SHOW_DATABASES` | `ShowDatabasesResult` | `databases[]`, `count()` |
| `CREATE DATABASE db;` | `CMD_CREATE_DATABASE` | `DatabaseOperationResult` | `db_name`, `success`, `message` |
| `DROP DATABASE db;` | `CMD_DROP_DATABASE` | `DatabaseOperationResult` | `db_name`, `success`, `message` |
| `USE db;` | `CMD_USE_DATABASE` | `DatabaseOperationResult` | `db_name`, `success`, `message` |
| `SHOW TABLES;` | `CMD_SHOW_TABLES` | `ShowTablesResult` | `tables[]`, `count()` |
| `DESC table;` | `CMD_DESC_TABLE` | `DescTableResult` | `table_name`, `columns[]` |
| `CREATE TABLE ...;` | `CMD_CREATE_TABLE` | `DDLResult` | `object_type`, `object_name`, `message` |
| `DROP TABLE t;` | `CMD_DROP_TABLE` | `DDLResult` | `object_type`, `object_name`, `message` |
| `CREATE INDEX ...;` | `CMD_CREATE_INDEX` | `DDLResult` | `object_type`, `object_name`, `message` |
| `DROP INDEX ...;` | `CMD_DROP_INDEX` | `DDLResult` | `object_type`, `object_name`, `message` |
| `INSERT INTO ...;` | `CMD_INSERT` | `InsertResult` | `table_name`, `success`, `message` |
| `DELETE FROM ...;` | `CMD_DELETE` | `DeleteResult` | `table_name`, `affected_rows`, `message` |
| `UPDATE ... SET ...;` | `CMD_UPDATE` | `UpdateResult` | `table_name`, `affected_rows`, `message` |
| `SELECT ... FROM ...;` | `CMD_SELECT` | `SelectResult` | `column_names[]`, `rows[]` |

### Detailed Result Structures

#### SelectResult
```cpp
struct SelectResult {
    std::vector<std::string> column_names;
    std::vector<SelectRow> rows;
    
    size_t row_count() const;
    size_t column_count() const;
};

struct SelectRow {
    std::vector<RowValue> values;
};

struct RowValue {
    std::string value;
};
```

**Usage:**
```cpp
auto& sel = std::get<SelectResult>(result.data);

// Headers
for (const auto& col : sel.column_names) {
    std::cout << col << "\t";
}
std::cout << "\n";

// Data rows
for (const auto& row : sel.rows) {
    for (const auto& val : row.values) {
        std::cout << val.value << "\t";
    }
    std::cout << "\n";
}

std::cout << sel.row_count() << " rows\n";
```

#### InsertResult / DeleteResult / UpdateResult
```cpp
struct InsertResult {
    std::string table_name;
    bool success;
    std::string message;
};

struct DeleteResult {
    std::string table_name;
    size_t affected_rows;
    bool success;
    std::string message;
};

struct UpdateResult {
    std::string table_name;
    size_t affected_rows;
    bool success;
    std::string message;
};
```

**Usage:**
```cpp
auto& ins = std::get<InsertResult>(result.data);
std::cout << ins.message << " in table " << ins.table_name << "\n";

auto& del = std::get<DeleteResult>(result.data);
std::cout << del.affected_rows << " rows deleted\n";

auto& upd = std::get<UpdateResult>(result.data);
std::cout << upd.affected_rows << " rows updated\n";
```

#### ShowTablesResult
```cpp
struct ShowTablesResult {
    std::vector<TableInfo> tables;
    size_t count() const;
};

struct TableInfo {
    std::string table_name;
};
```

**Usage:**
```cpp
auto& tables = std::get<ShowTablesResult>(result.data);
for (const auto& table : tables.tables) {
    std::cout << "  • " << table.table_name << "\n";
}
std::cout << "Total: " << tables.count() << " table(s)\n";
```

#### ShowDatabasesResult
```cpp
struct ShowDatabasesResult {
    std::vector<std::string> databases;
    size_t count() const;
};
```

**Usage:**
```cpp
auto& dbs = std::get<ShowDatabasesResult>(result.data);
for (const auto& db : dbs.databases) {
    std::cout << "  • " << db << "\n";
}
```

#### DescTableResult
```cpp
struct DescTableResult {
    std::string table_name;
    std::vector<ColumnDescription> columns;
    size_t count() const;
};

struct ColumnDescription {
    std::string field_name;
    std::string type_name;
    std::string has_index;  // "YES" or "NO"
};
```

**Usage:**
```cpp
auto& desc = std::get<DescTableResult>(result.data);
std::cout << "Table: " << desc.table_name << "\n";
for (const auto& col : desc.columns) {
    std::cout << "  " << col.field_name << " " << col.type_name;
    if (col.has_index == "YES") {
        std::cout << " [INDEXED]";
    }
    std::cout << "\n";
}
```

#### DDLResult (CREATE/DROP TABLE/INDEX)
```cpp
struct DDLResult {
    std::string object_type;  // "table" or "index"
    std::string object_name;
    bool success;
    std::string message;
};
```

**Usage:**
```cpp
auto& ddl = std::get<DDLResult>(result.data);
std::cout << "✓ " << ddl.message << ": " << ddl.object_name << "\n";
```

#### DatabaseOperationResult (CREATE/DROP/USE DATABASE)
```cpp
struct DatabaseOperationResult {
    std::string db_name;
    bool success;
    std::string message;
};
```

**Usage:**
```cpp
auto& db_op = std::get<DatabaseOperationResult>(result.data);
std::cout << "✓ " << db_op.message << ": " << db_op.db_name << "\n";
```

---

## Migration Guide

### Old Code (Console Logging)

```cpp
void execute_sql(const std::string& sql) {
    auto ast = parse_sql(sql);
    Interpreter::interp_sql(ast);  // void return, logs to console
    // Output automatically printed
}
```

### New Code (Structured Results)

```cpp
void execute_sql(const std::string& sql) {
    auto ast = parse_sql(sql);
    InterpreterResult result = Interpreter::interp_sql(ast);  // returns result
    
    if (!result.success) {
        std::cerr << "Error: " << result.error_message << "\n";
        return;
    }
    
    // Your application formats and displays the result
    switch (result.command_type) {
        case CMD_SELECT:
            display_select_result(std::get<SelectResult>(result.data));
            break;
        case CMD_INSERT:
            display_insert_result(std::get<InsertResult>(result.data));
            break;
        // ... handle other types
    }
}
```

### Converting to JSON (Example)

```cpp
#include <nlohmann/json.hpp>  // Example JSON library

json result_to_json(const InterpreterResult& result) {
    json j;
    j["success"] = result.success;
    j["command_type"] = command_type_to_string(result.command_type);
    
    if (!result.success) {
        j["error"] = result.error_message;
        return j;
    }
    
    if (result.command_type == CMD_SELECT) {
        auto& sel = std::get<SelectResult>(result.data);
        j["columns"] = sel.column_names;
        
        json rows_json = json::array();
        for (const auto& row : sel.rows) {
            json row_json = json::array();
            for (const auto& val : row.values) {
                row_json.push_back(val.value);
            }
            rows_json.push_back(row_json);
        }
        j["rows"] = rows_json;
        j["row_count"] = sel.row_count();
    }
    // ... handle other command types
    
    return j;
}
```

---

## Common Patterns

### Pattern 1: Simple Success Message

```cpp
InterpreterResult result = Interpreter::interp_sql(ast);
if (result.success) {
    std::cout << "✓ Command executed successfully\n";
}
```

### Pattern 2: Extract and Display SELECT Results

```cpp
if (result.command_type == CMD_SELECT) {
    auto& sel = std::get<SelectResult>(result.data);
    
    // Pretty print table
    std::cout << "+" << std::string(60, '-') << "+\n";
    for (const auto& col : sel.column_names) {
        std::cout << "| " << std::setw(15) << col;
    }
    std::cout << " |\n";
    std::cout << "+" << std::string(60, '-') << "+\n";
    
    for (const auto& row : sel.rows) {
        for (const auto& val : row.values) {
            std::cout << "| " << std::setw(15) << val.value;
        }
        std::cout << " |\n";
    }
    std::cout << "+" << std::string(60, '-') << "+\n";
    std::cout << sel.row_count() << " row(s)\n";
}
```

### Pattern 3: Unified Error Handler

```cpp
void execute_with_error_handling(const std::string& sql) {
    try {
        auto ast = parse_sql(sql);
        InterpreterResult result = Interpreter::interp_sql(ast);
        
        if (!result.success) {
            log_error("SQL execution failed", result.error_message);
            return;
        }
        
        process_result(result);
        
    } catch (const RedBaseError& e) {
        log_error("Database error", e.what());
    } catch (const std::exception& e) {
        log_error("System error", e.what());
    }
}
```

---

## Files

- `src/interpreter.h` - Main interpreter implementation
- `src/interpreter_defs.h` - Result structure definitions
- `src/system_management/sm_defs.h` - System management results (ShowTablesResult, DescTableResult)
- `src/query_language/ql_defs.h` - Query language results (SelectResult, InsertResult, etc.)

---

## Benefits Summary

| Feature | Old Approach | New Approach |
|---------|-------------|--------------|
| Output | Logs to console | Returns structured data |
| Flexibility | Fixed format | Application controls formatting |
| Testability | Parse console output | Direct access to data |
| API Usage | Not suitable | Perfect for APIs/web services |
| GUI Integration | Difficult | Easy integration |
| Performance | Always formats strings | Only format when needed |
| Type Safety | String parsing | Compile-time checking with variant |

---

## Troubleshooting

### Issue: Can't extract result data

**Problem:**
```cpp
auto& sel = std::get<SelectResult>(result.data);  // Throws exception!
```

**Solution:** Check command type first
```cpp
if (result.command_type == CMD_SELECT) {
    auto& sel = std::get<SelectResult>(result.data);  // Safe
}
```

### Issue: Not sure what type to extract

**Solution:** Use switch on command_type
```cpp
switch (result.command_type) {
    case CMD_SELECT: /* use SelectResult */ break;
    case CMD_INSERT: /* use InsertResult */ break;
    // ... etc
}
```

---

## See Also

- **Parser Documentation**: `src/parser/README.md`
- **Example Code**: `examples/interpreter_example.cpp`
- **Simple Demo**: `examples/simple_parser_demo.cpp`

---

## Summary

The Interpreter provides a clean, type-safe API for executing SQL commands:

**Returns structured data** - No console logging  
**Type safe** - Uses std::variant for compile-time checking  
**Flexible** - Application controls formatting  
**Complete** - Handles all SQL commands (DDL, DML, database ops)  
**Error handling** - Clear success/failure status  

Use `InterpreterResult result = Interpreter::interp_sql(ast)` to execute parsed SQL and get structured results you can process however you need!
