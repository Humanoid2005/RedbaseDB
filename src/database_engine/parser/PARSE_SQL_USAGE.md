# Using parse_sql() Function

## Quick Start

The `parse_sql()` function is a high-level wrapper that makes it easy to parse SQL strings into AST nodes.

### Basic Usage

```cpp
#include "parser/parser.h"
#include <iostream>

int main() {
    // Parse SQL string
    std::string sql = "SELECT * FROM users WHERE age > 18;";
    auto ast = parse_sql(sql);
    
    // Check if parsing succeeded
    if (ast) {
        // Use the AST
        std::cout << "Parsing successful!\n";
        
        // Debug: Print AST structure
        std::string debug_output = ast::TreePrinter::print(ast);
        std::cout << debug_output;
    } else {
        std::cout << "Parsing failed or empty statement\n";
    }
    
    return 0;
}
```

## Function Signature

```cpp
std::shared_ptr<ast::TreeNode> parse_sql(const std::string& sql);
```

**Parameters:**
- `sql` - SQL statement as a string (must end with semicolon)

**Returns:**
- `shared_ptr<ast::TreeNode>` - Pointer to root of AST
- `nullptr` - If statement is empty (e.g., just whitespace or "EXIT;")

## What It Does

1. Creates a lexer buffer from the SQL string
2. Calls Bison parser to tokenize and build AST
3. Cleans up lexer buffer
4. Returns the parsed AST tree

## Complete Example with Interpreter

```cpp
#include "parser/parser.h"
#include "interpreter.h"
#include <iostream>

int main() {
    std::string sql = "CREATE TABLE users (id INT, name CHAR(50), age INT);";
    
    try {
        // Step 1: Parse SQL → AST
        auto ast = parse_sql(sql);
        
        if (!ast) {
            std::cerr << "Empty or invalid SQL\n";
            return 1;
        }
        
        // Step 2: Execute AST → Result
        InterpreterResult result = Interpreter::interp_sql(ast);
        
        // Step 3: Check result
        if (result.success) {
            if (result.command_type == CMD_CREATE_TABLE) {
                auto& ddl = std::get<DDLResult>(result.data);
                std::cout << "✓ " << ddl.message << ": " << ddl.object_name << "\n";
            }
        } else {
            std::cerr << "❌ Error: " << result.error_message << "\n";
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
```

## Parsing Multiple Statements

```cpp
#include "parser/parser.h"
#include <vector>
#include <string>

std::vector<std::string> sqls = {
    "CREATE TABLE users (id INT, name CHAR(50));",
    "INSERT INTO users VALUES (1, 'Alice');",
    "SELECT * FROM users;",
    "DROP TABLE users;"
};

for (const auto& sql : sqls) {
    std::cout << "Parsing: " << sql << "\n";
    
    auto ast = parse_sql(sql);
    if (ast) {
        // Process AST...
        std::cout << "  ✓ Parsed successfully\n";
    }
}
```

## Error Handling

```cpp
#include "parser/parser.h"

try {
    std::string sql = "INVALID SQL SYNTAX!!!";
    auto ast = parse_sql(sql);
    
    // Parser will print error to stderr automatically
    // and return nullptr or throw exception
    
    if (!ast) {
        std::cerr << "Failed to parse SQL\n";
    }
    
} catch (const std::exception& e) {
    std::cerr << "Parse error: " << e.what() << "\n";
}
```

**Note:** The parser prints syntax errors to stderr with line and column information automatically via Bison's error handler.

## Debugging - View AST Structure

```cpp
#include "parser/parser.h"

std::string sql = "SELECT name, age FROM users WHERE age > 18;";
auto ast = parse_sql(sql);

if (ast) {
    // Print pretty AST structure
    std::string ast_str = ast::TreePrinter::print(ast);
    std::cout << "AST Structure:\n" << ast_str << "\n";
}

// Output:
// AST Structure:
// SELECT
//   LIST
//     COL
//       
//       name
//     COL
//       
//       age
//   LIST
//     users
//   LIST
//     BINARY_EXPR
//       COL
//         
//         age
//       >
//       INT_LIT
//         18
```

## SQL Requirements

The SQL string **must**:
- End with a semicolon (`;`)
- Follow supported SQL syntax (see parser README)
- Be valid according to grammar rules in `yacc.y`

Examples:
```cpp
// ✓ Correct - ends with semicolon
parse_sql("SELECT * FROM users;");

// ✗ Wrong - no semicolon
parse_sql("SELECT * FROM users");  // Will fail

// ✓ Correct - with conditions
parse_sql("SELECT * FROM users WHERE age > 18;");

// ✓ Correct - CREATE TABLE
parse_sql("CREATE TABLE t (id INT, name CHAR(50));");
```

## Supported SQL Commands

See the main [Parser README](README.md#supported-sql-syntax) for complete list of supported SQL syntax.

Quick summary:
- **Database:** SHOW/CREATE/DROP/USE DATABASE
- **Tables:** SHOW TABLES, DESC, CREATE/DROP TABLE
- **Indexes:** CREATE/DROP INDEX
- **DML:** INSERT, DELETE, UPDATE, SELECT
- **Types:** INT, FLOAT, CHAR(n), DATETIME
- **Operators:** =, <>, <, >, <=, >=, AND

## Implementation Details

The `parse_sql()` function is an inline function defined in `src/parser/parser_defs.h`:

```cpp
inline std::shared_ptr<ast::TreeNode> parse_sql(const std::string& sql) {
    YY_BUFFER_STATE buf = yy_scan_string(sql.c_str());
    yyparse();
    yy_delete_buffer(buf);
    return ast::parse_tree;
}
```

It wraps the low-level Flex/Bison API:
- `yy_scan_string()` - Creates lexer buffer
- `yyparse()` - Runs parser (builds `ast::parse_tree`)
- `yy_delete_buffer()` - Cleans up buffer
- Returns `ast::parse_tree` (global variable)

## Examples Directory

See working examples in `/examples/`:
- `simple_parser_demo.cpp` - Basic parse_sql() usage
- `interpreter_example.cpp` - Complete parser + interpreter demo

## See Also

- [Parser README](README.md) - Complete parser documentation
- [Interpreter API](../INTERPRETER_API.md) - How to execute parsed ASTs
- [Interpreter Quick Reference](../../documentation/INTERPRETER_QUICKREF.md) - Quick reference guide
