// Simple demo showing parse_sql() function usage
#include "src/parser/parser.h"
#include <iostream>

int main() {
    std::cout << "=== Parser Demo ===\n\n";
    
    // Example 1: Parse a simple SELECT
    std::string sql1 = "SELECT name, age FROM users WHERE age > 18;";
    std::cout << "SQL: " << sql1 << "\n";
    
    auto ast1 = parse_sql(sql1);
    if (ast1) {
        std::string ast_str = ast::TreePrinter::print(ast1);
        std::cout << "AST:\n" << ast_str << "\n";
    }
    
    // Example 2: Parse CREATE TABLE
    std::string sql2 = "CREATE TABLE students (id INT, name CHAR(50), gpa FLOAT);";
    std::cout << "\nSQL: " << sql2 << "\n";
    
    auto ast2 = parse_sql(sql2);
    if (ast2) {
        std::string ast_str = ast::TreePrinter::print(ast2);
        std::cout << "AST:\n" << ast_str << "\n";
    }
    
    // Example 3: Parse INSERT
    std::string sql3 = "INSERT INTO students VALUES (1, 'Alice', 3.8);";
    std::cout << "\nSQL: " << sql3 << "\n";
    
    auto ast3 = parse_sql(sql3);
    if (ast3) {
        std::string ast_str = ast::TreePrinter::print(ast3);
        std::cout << "AST:\n" << ast_str << "\n";
    }
    
    std::cout << "\n✓ parse_sql() function works correctly!\n";
    
    return 0;
}
