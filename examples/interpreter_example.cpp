// Example: Using the Interpreter with structured results
// This demonstrates how to use the interpreter without any console logging

#include "src/interpreter.h"
#include "src/parser/parser.h"
#include <iostream>
#include <iomanip>
#include <variant>

void print_select_result(const SelectResult& sel) {
    // Print column headers
    for (const auto& col : sel.column_names) {
        std::cout << col << "\t";
    }
    std::cout << "\n";
    
    // Print separator
    for (size_t i = 0; i < sel.column_count(); i++) {
        std::cout << "--------\t";
    }
    std::cout << "\n";
    
    // Print rows
    for (const auto& row : sel.rows) {
        for (const auto& val : row.values) {
            std::cout << val.value << "\t";
        }
        std::cout << "\n";
    }
    
    std::cout << "\n(" << sel.row_count() << " row(s) returned)\n";
}

void print_table_list(const ShowTablesResult& tables) {
    std::cout << "Tables in current database:\n";
    std::cout << "----------------------------\n";
    for (const auto& table : tables.tables) {
        std::cout << "  • " << table.table_name << "\n";
    }
    std::cout << "\nTotal: " << tables.count() << " table(s)\n";
}

void print_database_list(const ShowDatabasesResult& databases) {
    std::cout << "Available databases:\n";
    std::cout << "--------------------\n";
    for (const auto& db : databases.databases) {
        std::cout << "  • " << db << "\n";
    }
    std::cout << "\nTotal: " << databases.count() << " database(s)\n";
}

void print_table_description(const DescTableResult& desc) {
    std::cout << "Table: " << desc.table_name << "\n";
    std::cout << "+" << std::string(60, '-') << "+\n";
    std::cout << "| Field            | Type         | Indexed |\n";
    std::cout << "+" << std::string(60, '-') << "+\n";
    
    for (const auto& col : desc.columns) {
        std::cout << "| " << std::left << std::setw(16) << col.field_name 
                  << " | " << std::setw(12) << col.type_name 
                  << " | " << std::setw(7) << col.has_index << " |\n";
    }
    
    std::cout << "+" << std::string(60, '-') << "+\n";
    std::cout << desc.count() << " column(s)\n";
}

void execute_and_display(const std::string& sql) {
    std::cout << "\n> " << sql << "\n";
    std::cout << std::string(70, '=') << "\n";
    
    try {
        // Parse SQL
        auto ast_root = parse_sql(sql);
        
        // Interpret (returns structured data, NO console logging)
        InterpreterResult result = Interpreter::interp_sql(ast_root);
        
        if (!result.success) {
            std::cerr << "❌ Error: " << result.error_message << "\n";
            return;
        }
        
        // Process results based on command type
        switch (result.command_type) {
            case CMD_HELP: {
                auto& help = std::get<HelpResult>(result.data);
                std::cout << help.help_text;
                break;
            }
            
            case CMD_SHOW_TABLES: {
                auto& tables = std::get<ShowTablesResult>(result.data);
                print_table_list(tables);
                break;
            }
            
            case CMD_SHOW_DATABASES: {
                auto& databases = std::get<ShowDatabasesResult>(result.data);
                print_database_list(databases);
                break;
            }
            
            case CMD_CREATE_DATABASE:
            case CMD_DROP_DATABASE:
            case CMD_USE_DATABASE: {
                auto& db_op = std::get<DatabaseOperationResult>(result.data);
                std::cout << "✓ " << db_op.message << ": " << db_op.db_name << "\n";
                break;
            }
            
            case CMD_DESC_TABLE: {
                auto& desc = std::get<DescTableResult>(result.data);
                print_table_description(desc);
                break;
            }
            
            case CMD_CREATE_TABLE:
            case CMD_DROP_TABLE:
            case CMD_CREATE_INDEX:
            case CMD_DROP_INDEX: {
                auto& ddl = std::get<DDLResult>(result.data);
                std::cout << "✓ " << ddl.message << ": " << ddl.object_name << "\n";
                break;
            }
            
            case CMD_INSERT: {
                auto& ins = std::get<InsertResult>(result.data);
                std::cout << "✓ " << ins.message << "\n";
                std::cout << "  Table: " << ins.table_name << "\n";
                break;
            }
            
            case CMD_DELETE: {
                auto& del = std::get<DeleteResult>(result.data);
                std::cout << "✓ " << del.message << "\n";
                std::cout << "  Table: " << del.table_name << "\n";
                std::cout << "  Affected rows: " << del.affected_rows << "\n";
                break;
            }
            
            case CMD_UPDATE: {
                auto& upd = std::get<UpdateResult>(result.data);
                std::cout << "✓ " << upd.message << "\n";
                std::cout << "  Table: " << upd.table_name << "\n";
                std::cout << "  Affected rows: " << upd.affected_rows << "\n";
                break;
            }
            
            case CMD_SELECT: {
                auto& sel = std::get<SelectResult>(result.data);
                print_select_result(sel);
                break;
            }
            
            default:
                std::cout << "✓ Command executed successfully\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Exception: " << e.what() << "\n";
    }
}

int main() {
    std::cout << "=== Interpreter Structured Results Demo ===\n";
    std::cout << "No console logging from interpreter!\n";
    std::cout << "All output is formatted by the application.\n";
    
    // Example queries
    execute_and_display("SHOW DATABASES;");
    execute_and_display("CREATE DATABASE university;");
    execute_and_display("USE university;");
    execute_and_display("CREATE TABLE students (id INT, name CHAR(50), age INT, enrolled DATETIME);");
    execute_and_display("DESC students;");
    execute_and_display("SHOW TABLES;");
    execute_and_display("INSERT INTO students VALUES (1, 'Alice', 20, '2024-09-01 09:00:00');");
    execute_and_display("SELECT * FROM students;");
    execute_and_display("UPDATE students SET age = 21 WHERE id = 1;");
    execute_and_display("DELETE FROM students WHERE age > 100;");
    execute_and_display("HELP;");
    
    return 0;
}
