# Parser Module - Complete Documentation

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Components](#components)
4. [Lexer and Parser Background](#lexer-and-parser-background)
5. [File Descriptions](#file-descriptions)
6. [Complete Processing Pipeline](#complete-processing-pipeline)
7. [How to Use](#how-to-use)
8. [Adding New Features](#adding-new-features)

---

## Overview

The **Parser Module** is responsible for converting raw SQL text into structured Abstract Syntax Trees (ASTs) that can be executed by the database engine. It uses industry-standard tools (Flex and Bison) to implement a robust SQL parser.

**Key Responsibilities:**
- Tokenize SQL text into meaningful units (keywords, identifiers, literals)
- Parse tokens according to SQL grammar rules
- Build Abstract Syntax Trees representing SQL commands
- Validate SQL syntax and report errors
- Pass structured data to the interpreter for execution

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        SQL TEXT INPUT                            │
│              "SELECT name FROM users WHERE age > 18;"            │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                    LEXER (lex.l → lex.yy.cpp)                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Tokenization: Split text into tokens                      │   │
│  │ • Keywords: SELECT, FROM, WHERE                          │   │
│  │ • Identifiers: name, users, age                          │   │
│  │ • Operators: >, =, <                                     │   │
│  │ • Literals: 18 (integer), 'text' (string)               │   │
│  └──────────────────────────────────────────────────────────┘   │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                   PARSER (yacc.y → yacc.tab.cpp)                │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Syntax Analysis: Apply grammar rules                      │   │
│  │ • stmt → SELECT selector FROM table WHERE condition       │   │
│  │ • condition → col op value                               │   │
│  │ • Build tree structure from tokens                       │   │
│  └──────────────────────────────────────────────────────────┘   │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                  AST (ast.h - Abstract Syntax Tree)             │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │               SelectStmt                                  │   │
│  │                   ├── cols: [Col("name")]               │   │
│  │                   ├── tabs: ["users"]                   │   │
│  │                   └── conds: [BinaryExpr(               │   │
│  │                         Col("age"), >, IntLit(18))]     │   │
│  └──────────────────────────────────────────────────────────┘   │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│            INTERPRETER (../interpreter.h)                        │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Execution: Walk AST and execute operations               │   │
│  │ • Extract column names, table names, conditions          │   │
│  │ • Call QL_Manager::select_from(...)                      │   │
│  │ • Return InterpreterResult with structured data          │   │
│  └──────────────────────────────────────────────────────────┘   │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                   DATABASE ENGINE EXECUTION                      │
│         (SM_Manager, QL_Manager, RM_Manager, etc.)              │
└─────────────────────────────────────────────────────────────────┘
```

---

## Components

### 1. **Lexer (Lexical Analyzer)** - `lex.l`

**Purpose:** Convert raw text into tokens.

**Tool:** Flex (Fast Lexical Analyzer Generator)

**Input:** SQL text string
```sql
SELECT name FROM users WHERE age > 18;
```

**Output:** Stream of tokens
```
SELECT IDENTIFIER("name") FROM IDENTIFIER("users") WHERE IDENTIFIER("age") > VALUE_INT(18) ;
```

**How it works:**
- Uses regular expressions to match patterns
- Each pattern maps to a token type
- Handles whitespace, comments, and special characters
- Returns tokens one at a time to the parser

### 2. **Parser (Syntax Analyzer)** - `yacc.y`

**Purpose:** Apply grammar rules to build an Abstract Syntax Tree.

**Tool:** Bison (GNU Parser Generator, successor to YACC)

**Input:** Stream of tokens from lexer

**Output:** Abstract Syntax Tree (AST)

**How it works:**
- Uses context-free grammar (CFG) rules
- Matches token sequences to grammar productions
- Executes semantic actions to build AST nodes
- Handles operator precedence and associativity
- Reports syntax errors with line/column information

### 3. **Abstract Syntax Tree (AST)** - `ast.h`

**Purpose:** Structured representation of SQL commands.

**Key Components:**
- `TreeNode` - Base class for all AST nodes
- DDL nodes: `CreateTable`, `DropTable`, `CreateIndex`, etc.
- DML nodes: `InsertStmt`, `DeleteStmt`, `UpdateStmt`, `SelectStmt`
- Expression nodes: `Value`, `Col`, `BinaryExpr`, `SetClause`
- Database nodes: `ShowDatabases`, `CreateDatabase`, `UseDatabase`, etc.

### 4. **AST Printer** - `ast_printer.h`

**Purpose:** Debug utility to visualize AST structure.

**How it works:**
- Traverses AST recursively
- Returns formatted string representation
- Useful for testing and debugging parser

### 5. **Parser Interface** - `parser.h`, `parser.cpp`

**Purpose:** High-level API for parsing SQL.

**Key Functions:**
```cpp
shared_ptr<ast::TreeNode> parse_sql(const string& sql);
```

### 6. **Interpreter** - `../interpreter.h`

**Purpose:** Execute AST by calling database operations.

**Key Functions:**
```cpp
InterpreterResult interp_sql(const shared_ptr<ast::TreeNode>& root);
```

**How it works:**
- Uses `dynamic_pointer_cast` to identify node types
- Extracts data from AST nodes
- Calls appropriate SM_Manager or QL_Manager functions
- Returns structured results (no console logging)

---

## Lexer and Parser Background

### What is a Lexer?

A **lexer** (lexical analyzer) performs **lexical analysis** - the first phase of compilation/interpretation.

**Job:** Break input text into **tokens** (meaningful units).

**Example:**
```
Input:  "SELECT * FROM users;"
Tokens: [SELECT] [*] [FROM] [IDENTIFIER("users")] [;]
```

**Why use Flex?**
- Industry-standard tool (used in GCC, Python, etc.)
- Fast pattern matching using DFAs (Deterministic Finite Automata)
- Handles complex regular expressions efficiently
- Generates C/C++ code from declarative specifications

### What is a Parser?

A **parser** (syntax analyzer) performs **syntax analysis** - the second phase of compilation/interpretation.

**Job:** Check if token sequence follows grammar rules and build a parse tree/AST.

**Example:**
```
Tokens: [SELECT] [*] [FROM] [users]
Grammar: stmt → SELECT selector FROM table
Result: SelectStmt(cols=[*], tabs=["users"], conds=[])
```

**Why use Bison?**
- Industry-standard parser generator (successor to YACC)
- Implements LR(1) parsing (powerful and efficient)
- Handles complex grammars with shift/reduce conflicts
- Generates optimized C/C++ code
- Excellent error reporting

### Flex/Bison Workflow

```
┌─────────────┐                  ┌──────────────┐
│   lex.l     │─── Flex ────────→│  lex.yy.cpp  │
│ (patterns)  │    compiler      │  (C++ code)  │
└─────────────┘                  └──────────────┘
                                        │
                                        │ #include
                                        ▼
┌─────────────┐                  ┌──────────────┐
│   yacc.y    │─── Bison ───────→│ yacc.tab.cpp │
│ (grammar)   │    compiler      │  (C++ code)  │
└─────────────┘                  └──────────────┘
                                        │
                                        │ compile
                                        ▼
                                  ┌──────────────┐
                                  │   parser.o   │
                                  │  (object)    │
                                  └──────────────┘
```

### Build Process: How Flex and Bison Generate Code

When you run `make`, CMake automatically invokes Flex and Bison to generate C++ code from your specifications. Here's exactly what happens:

#### 1. **CMake Configuration** (`src/CMakeLists.txt`)

```cmake
find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

# Generate parser from yacc.y
BISON_TARGET(Parser
        database_engine/parser/yacc.y
        ${CMAKE_CURRENT_BINARY_DIR}/yacc.tab.cpp
        DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/yacc.tab.h
)

# Generate lexer from lex.l
FLEX_TARGET(Lexer
        database_engine/parser/lex.l
        ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.cpp
)

# Link lexer and parser (lexer includes yacc.tab.h)
ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)
```

**What this does:**
- Tells CMake to use Flex and Bison tools
- Sets up custom build rules for `.l` and `.y` files
- Defines input files and output locations
- Creates dependency: lexer depends on parser (needs token definitions)

#### 2. **Bison Compilation** (`yacc.y` → `yacc.tab.cpp` + `yacc.tab.h`)

**Command executed by CMake:**
```bash
bison -d database_engine/parser/yacc.y -o build/src/yacc.tab.cpp
```

**What Bison does:**
1. Reads grammar rules from `yacc.y`
2. Analyzes grammar for conflicts (shift/reduce, reduce/reduce)
3. Builds LR(1) parsing tables (state machine)
4. Generates C++ code implementing the parser

**Generated files:**

**`yacc.tab.cpp`** (~2000-3000 lines):
```cpp
// Generated by Bison - DO NOT EDIT

// Parsing tables (state machine)
static const short yytable[] = { 45, 23, 67, 12, ... };
static const short yycheck[] = { 0, 1, 2, 3, ... };

// Parser state machine
int yyparse() {
    int yystate = 0;
    int yytoken = YYEMPTY;
    
    while (true) {
        // Read token from lexer
        if (yytoken == YYEMPTY) {
            yytoken = yylex(&yylval, &yylloc);
        }
        
        // Lookup action in parsing table
        int action = yytable[yystate];
        
        if (action > 0) {
            // SHIFT: Push token onto stack, transition to new state
            yypush(yytoken, yylval);
            yystate = action;
            yytoken = YYEMPTY;
        } else if (action < 0) {
            // REDUCE: Apply grammar rule
            switch (-action) {
                case 1: // stmt → SELECT selector FROM table WHERE cond
                    yylval.sv_node = std::make_shared<SelectStmt>(...);
                    break;
                case 2: // selector → '*'
                    yylval.sv_cols = {};
                    break;
                // ... hundreds of cases for each grammar rule
            }
            yypop(rule_length);
        } else {
            // ACCEPT or ERROR
            if (action == 0) return 0; // Success
            else yyerror("syntax error");
        }
    }
}
```

**`yacc.tab.h`** (~500 lines):
```cpp
// Generated by Bison - DO NOT EDIT

// Token type definitions (used by lexer)
#define SELECT 258
#define FROM 259
#define WHERE 260
#define IDENTIFIER 261
#define VALUE_INT 262
// ... etc

// Semantic value type
#define YYSTYPE ast::SemValue

// Location type
typedef struct YYLTYPE {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} YYLTYPE;

// Parser function
int yyparse();
```

#### 3. **Flex Compilation** (`lex.l` → `lex.yy.cpp`)

**Command executed by CMake:**
```bash
flex -o build/src/lex.yy.cpp database_engine/parser/lex.l
```

**What Flex does:**
1. Reads pattern rules from `lex.l`
2. Converts regular expressions to DFA (Deterministic Finite Automaton)
3. Optimizes DFA for speed
4. Generates C++ code implementing the lexer

**Generated file:**

**`lex.yy.cpp`** (~4000-5000 lines):
```cpp
// Generated by Flex - DO NOT EDIT

#include "yacc.tab.h"  // Get token definitions

// DFA transition table (optimized state machine)
static const short yy_base[] = { 0, 1, 2, 45, 67, ... };
static const short yy_nxt[] = { 3, 4, 5, 6, 7, ... };

// Main lexer function
int yylex(YYSTYPE* yylval, YYLTYPE* yylloc) {
    int yy_state = 0;
    char* yy_cp = yytext;
    
    while (true) {
        // Get next character
        int c = getc(yyin);
        
        // Transition to next state based on character
        yy_state = yy_nxt[yy_base[yy_state] + c];
        
        // Check if we're in an accepting state
        if (yy_accept[yy_state]) {
            // Match found! Execute action
            switch (yy_state) {
                case 12: // Matched "SELECT"
                    return SELECT;
                
                case 15: // Matched "FROM"
                    return FROM;
                
                case 23: // Matched {identifier}
                    yylval->sv_str = strdup(yytext);
                    return IDENTIFIER;
                
                case 27: // Matched {digit}+
                    yylval->sv_int = atoi(yytext);
                    return VALUE_INT;
                
                case 31: // Matched whitespace
                    // Skip - don't return anything
                    break;
                
                // ... hundreds of cases for each pattern
            }
        }
        
        // No match yet, keep scanning
        *yy_cp++ = c;
    }
}
```

#### 4. **C++ Compilation**

**Command executed by CMake:**
```bash
g++ -c build/src/yacc.tab.cpp -o build/src/yacc.tab.o
g++ -c build/src/lex.yy.cpp -o build/src/lex.yy.o
g++ -c src/database_engine/parser/ast.cpp -o build/src/ast.o
# ... other files
```

**What happens:**
- Generated C++ files are compiled like any other source files
- `lex.yy.cpp` includes `yacc.tab.h` to get token definitions
- Object files are linked into the library

#### 5. **Linking**

```bash
ar rcs lib/libAUTODB-cpp.a yacc.tab.o lex.yy.o ast.o ... (all other .o files)
```

**Result:** Static library containing all database engine code, including the parser.

#### Build Sequence Summary

```
1. CMake detects yacc.y and lex.l need processing
   ↓
2. Bison generates yacc.tab.cpp and yacc.tab.h
   ↓
3. Flex generates lex.yy.cpp (includes yacc.tab.h)
   ↓
4. G++ compiles yacc.tab.cpp → yacc.tab.o
   ↓
5. G++ compiles lex.yy.cpp → lex.yy.o
   ↓
6. G++ compiles other source files → .o files
   ↓
7. ar links all .o files → libAUTODB-cpp.a
   ↓
8. G++ links library with executables → db_server, db_client, tests
```

#### When Are Files Regenerated?

**Files are regenerated when:**
- `yacc.y` is modified → regenerate `yacc.tab.cpp` and `yacc.tab.h`
- `lex.l` is modified → regenerate `lex.yy.cpp`
- `make clean` is run → remove all generated files

**Files are NOT regenerated when:**
- Other source files change (e.g., `interpreter.h`, `ql_manager.cpp`)
- Build already up-to-date

**Forced regeneration:**
```bash
cd build
rm -f src/lex.yy.cpp src/yacc.tab.cpp src/yacc.tab.h
make
```

#### Where Are Generated Files?

**Location:** `build/src/` (not in source tree!)

```
build/
└── src/
    ├── lex.yy.cpp      (Generated by Flex)
    ├── yacc.tab.cpp    (Generated by Bison)
    └── yacc.tab.h      (Generated by Bison)
```

**Never commit these to Git!** They're automatically generated during build.

#### How Parser Invocation Works at Runtime

When your application calls `parse_sql("SELECT * FROM users;")`:

```cpp
// 1. Application code
auto ast = parse_sql("SELECT * FROM users;");

// 2. parse_sql() implementation (parser_defs.h)
inline shared_ptr<TreeNode> parse_sql(const string& sql) {
    // a) Create lexer input buffer from string
    YY_BUFFER_STATE buf = yy_scan_string(sql.c_str());
    
    // b) Start parsing (generated yacc.tab.cpp)
    yyparse();  
    //   ↓
    //   Parser calls yylex() to get tokens
    //   ↓
    //   Lexer scans input buffer, returns tokens
    //   ↓
    //   Parser applies grammar rules, builds AST
    //   ↓
    //   AST stored in ast::parse_tree global variable
    
    // c) Clean up buffer
    yy_delete_buffer(buf);
    
    // d) Return the built AST
    return ast::parse_tree;
}

// 3. Now you have the AST!
if (auto select = dynamic_pointer_cast<SelectStmt>(ast)) {
    // Extract data from AST...
}
```

**Key points:**
1. `yy_scan_string()` - Flex function, sets up input for lexer
2. `yyparse()` - Bison function, runs the parser state machine
3. `yylex()` - Flex function, called by parser to get next token
4. `ast::parse_tree` - Global variable where parser stores result
5. `yy_delete_buffer()` - Flex function, cleans up memory

#### Understanding the Generated Code

You don't need to understand the generated code (it's machine-generated and optimized), but here's what's happening inside:

**Lexer (lex.yy.cpp):**
- Implements a DFA (state machine)
- Each state represents "partial matches so far"
- Transitions on input characters
- When accepting state reached → execute action, return token

**Parser (yacc.tab.cpp):**
- Implements LR(1) parser (state machine)
- Maintains two stacks: state stack and value stack
- Two operations: SHIFT (push token) and REDUCE (apply rule)
- Uses lookahead token to decide shift vs reduce
- Executes semantic actions to build AST

**Example walkthrough:**

Input: `SELECT * FROM users;`

```
Lexer:
State 0 → 'S' → State 5
State 5 → 'E' → State 12
State 12 → 'L' → State 23
State 23 → 'E' → State 34
State 34 → 'C' → State 45
State 45 → 'T' → State 56 (ACCEPT: return SELECT)

Parser:
[State 0] SHIFT SELECT → [State 0, State 3]
[State 3] SHIFT * → [State 0, State 3, State 7]
[State 7] REDUCE selector → * → [State 0, State 4]
[State 4] SHIFT FROM → [State 0, State 4, State 9]
...
Eventually REDUCE all the way to stmt
ACCEPT!
```

---

## File Descriptions

### `lex.l` - Lexer Specification

**Structure:**
```
%{
    // C++ code (headers, includes)
%}

/* Definitions section - regular expression patterns */
digit       [0-9]
letter      [a-zA-Z]
identifier  {letter}({letter}|{digit}|_)*

%%
/* Rules section - pattern → action */

"SELECT"    { return SELECT; }
"FROM"      { return FROM; }
{identifier} { yylval->sv_str = yytext; return IDENTIFIER; }
{digit}+     { yylval->sv_int = atoi(yytext); return VALUE_INT; }
[ \t\n]+     { /* skip whitespace */ }

%%
/* User code section - additional C++ functions */
```

**Key Sections:**

1. **Definitions (`%{ %}`)**: C++ includes and helper code
2. **Pattern Definitions**: Named regular expressions for reuse
3. **Rules (`%% ... %%`)**: Pattern → Action mappings
4. **User Code**: Additional C++ functions

**Pattern Syntax:**
- `.` - Any character except newline
- `*` - Zero or more repetitions
- `+` - One or more repetitions
- `?` - Zero or one occurrence
- `[abc]` - Character class (a, b, or c)
- `[a-z]` - Character range
- `[^abc]` - Negation (anything except a, b, c)
- `|` - Alternation (or)
- `()` - Grouping
- `{name}` - Reference to named pattern

**Actions:**
- Return token type: `return SELECT;`
- Store value: `yylval->sv_int = atoi(yytext);`
- Skip: `{ /* nothing */ }`

### `yacc.y` - Parser Specification

**Structure:**
```
%{
    // C++ code (headers, error handler)
%}

/* Declarations section */
%define api.value.type {ast::SemValue}  // Semantic value type
%token SELECT FROM WHERE                 // Terminal symbols
%type <sv_node> stmt                     // Non-terminal types

%%
/* Grammar rules section */

stmt: SELECT selector FROM table optWhereClause
    {
        $$ = std::make_shared<SelectStmt>($2, $4, $5);
    }
    ;

selector: '*' { $$ = {}; }
        | colList { $$ = $1; }
        ;

%%
/* User code section */
```

**Key Sections:**

1. **Prologue (`%{ %}`)**: C++ includes, error handler
2. **Declarations**: Tokens, types, precedence
3. **Grammar Rules (`%% ... %%`)**: Production rules with actions
4. **Epilogue**: Additional C++ code

**Grammar Rule Syntax:**
```
non_terminal: pattern1 { action1 }
            | pattern2 { action2 }
            | pattern3 { action3 }
            ;
```

**Semantic Actions:**
- `$$` - Value of left-hand side (result)
- `$1, $2, $3, ...` - Values of right-hand side symbols
- Execute C++ code to build AST nodes

**Example:**
```yacc
stmt: SELECT selector FROM table WHERE condition
    {
        // $1=SELECT, $2=selector, $3=FROM, $4=table, $5=WHERE, $6=condition
        $$ = std::make_shared<SelectStmt>($2, $4, $6);
    }
    ;
```

**About Red Squiggles in VSCode:**

You may see red error squiggles in `yacc.y` like "Type was not declared in the %union". These are **false positives** from VSCode's C++ linter, which doesn't understand Bison's advanced features.

**Why they appear:**
- Bison supports custom semantic value types via `%define api.value.type {ast::SemValue}`
- VSCode's linter expects the old-style `%union` declaration
- The linter doesn't understand that `ast::SemValue` is defined in `ast.h`

**The truth:**
- ✅ **The code compiles perfectly** - Bison understands the syntax
- ✅ **All tests pass** - No actual errors exist
- ❌ **VSCode is confused** - It's a limitation of the IDE's parser

**Solution:** Ignore the red squiggles. They don't affect compilation. Alternatively, disable C++ error squiggles for `.y` files in VSCode settings.

### `ast.h` - AST Node Definitions

**Structure:**
```cpp
namespace ast {

// Enums
enum SvType { SV_TYPE_INT, SV_TYPE_FLOAT, SV_TYPE_STRING, SV_TYPE_DATETIME };
enum SvCompOp { SV_OP_EQ, SV_OP_NE, SV_OP_LT, SV_OP_GT, SV_OP_LE, SV_OP_GE };

// Base class
struct TreeNode {
    virtual ~TreeNode() = default;
};

// DDL nodes
struct CreateTable : public TreeNode {
    std::string tab_name;
    std::vector<std::shared_ptr<Field>> fields;
    
    CreateTable(string name, vector<shared_ptr<Field>> flds)
        : tab_name(move(name)), fields(move(flds)) {}
};

// DML nodes
struct SelectStmt : public TreeNode {
    std::vector<std::shared_ptr<Col>> cols;
    std::vector<std::string> tabs;
    std::vector<std::shared_ptr<BinaryExpr>> conds;
    
    SelectStmt(vector<shared_ptr<Col>> c, vector<string> t, vector<shared_ptr<BinaryExpr>> w)
        : cols(move(c)), tabs(move(t)), conds(move(w)) {}
};

// Expression nodes
struct Value : public Expr {};
struct IntLit : public Value { int val; };
struct Col : public Expr { string tab_name; string col_name; };
struct BinaryExpr : public TreeNode { 
    shared_ptr<Col> lhs; 
    SvCompOp op; 
    shared_ptr<Expr> rhs; 
};

// Semantic value (union-like struct for Bison)
struct SemValue {
    int sv_int;
    float sv_float;
    string sv_str;
    shared_ptr<TreeNode> sv_node;
    shared_ptr<Col> sv_col;
    // ... other fields
};

extern shared_ptr<TreeNode> parse_tree;  // Global parse result

} // namespace ast

#define YYSTYPE ast::SemValue  // Tell Bison to use SemValue
```

**Node Hierarchy:**
```
TreeNode (base)
├── Help
├── ShowTables / ShowDatabases
├── CreateDatabase / DropDatabase / UseDatabase
├── DescTable
├── CreateTable / DropTable
├── CreateIndex / DropIndex
├── InsertStmt / DeleteStmt / UpdateStmt / SelectStmt
└── Expr
    ├── Value (IntLit, FloatLit, StringLit, DateTimeLit)
    ├── Col
    ├── BinaryExpr
    └── SetClause
```

### `ast_printer.h` - Debug Visualization

**Purpose:** Convert AST to readable string format.

**Usage:**
```cpp
auto ast = parse_sql("SELECT * FROM users;");
string debug_output = TreePrinter::print(ast);
std::cout << debug_output;
// Output:
// SELECT
//   LIST
//   LIST
//     users
//   LIST
```

**How it works:**
- Implements visitor pattern
- Recursively traverses AST
- Returns indented string representation
- No console logging (returns string)

### `parser.h` / `parser_defs.h` - Parser Interface

**High-level API:**

```cpp
#include "parser/parser.h"

// Parse SQL string → AST
shared_ptr<ast::TreeNode> parse_sql(const string& sql);
```

**Implementation:** (in `parser_defs.h`)
```cpp
inline shared_ptr<ast::TreeNode> parse_sql(const string& sql) {
    // 1. Create lexer buffer from string
    YY_BUFFER_STATE buf = yy_scan_string(sql.c_str());
    
    // 2. Parse (yyparse calls lexer, builds AST in ast::parse_tree)
    yyparse();
    
    // 3. Clean up buffer
    yy_delete_buffer(buf);
    
    // 4. Return the global parse_tree
    return ast::parse_tree;
}
```

**Usage Example:**
```cpp
#include "parser/parser.h"

auto ast = parse_sql("SELECT * FROM users;");
if (ast) {
    // Use the AST
    std::string debug = ast::TreePrinter::print(ast);
    std::cout << debug;
}
```

### `../interpreter.h` - AST Executor

**Purpose:** Execute parsed SQL by calling database operations.

**Key Function:**
```cpp
InterpreterResult Interpreter::interp_sql(const shared_ptr<ast::TreeNode>& root) {
    if (auto x = dynamic_pointer_cast<ast::SelectStmt>(root)) {
        // Extract data from AST
        vector<TabCol> cols = ...;
        vector<string> tables = x->tabs;
        vector<Condition> conds = ...;
        
        // Execute query
        SelectResult result = QL_Manager::select_from(cols, tables, conds);
        
        // Return structured result
        return InterpreterResult(CMD_SELECT, true, result);
    }
    else if (auto x = dynamic_pointer_cast<ast::CreateTable>(root)) {
        // Handle CREATE TABLE
        vector<ColumnInfo> col_defs = ...;
        SM_Manager::create_table(x->tab_name, col_defs);
        return InterpreterResult(CMD_CREATE_TABLE, true, DDLResult(...));
    }
    // ... handle other command types
}
```

**Conversion Functions:**
```cpp
// Convert AST types to execution types
static ColumnType interp_sv_type(ast::SvType sv_type);
static CompOp interp_sv_comp_op(ast::SvCompOp op);
static Value interp_sv_value(const shared_ptr<ast::Value>& sv_val);
static vector<Condition> interp_where_clause(const vector<shared_ptr<ast::BinaryExpr>>& conds);
```

**Important:** Interpreter returns `InterpreterResult` with structured data - it does NOT log to console.

---

## Complete Processing Pipeline

Let's trace a complete SQL query through the entire system:

### Example SQL:
```sql
SELECT name, age FROM users WHERE age > 18;
```

### Step 1: Lexical Analysis (lex.l)

**Input:** Raw text string

**Process:**
```
"SELECT" → Matches pattern "SELECT" → Returns token SELECT
" "      → Matches whitespace → Skip
"name"   → Matches {identifier} → Returns IDENTIFIER, stores "name" in yylval->sv_str
","      → Matches literal → Returns ','
"age"    → Matches {identifier} → Returns IDENTIFIER, stores "age"
" "      → Skip
"FROM"   → Returns FROM
"users"  → Returns IDENTIFIER, stores "users"
"WHERE"  → Returns WHERE
"age"    → Returns IDENTIFIER, stores "age"
">"      → Returns '>'
"18"     → Matches {digit}+ → Returns VALUE_INT, stores 18 in yylval->sv_int
";"      → Returns ';'
```

**Output:** Token stream for parser

### Step 2: Syntax Analysis (yacc.y)

**Input:** Token stream

**Process:** Apply grammar rules bottom-up (shift-reduce parsing)

```
1. Shift: name → IDENTIFIER("name")
2. Reduce: IDENTIFIER → colName
3. Reduce: colName → col  (creates Col("", "name"))
4. Shift: , age
5. Reduce: Similar for "age" → Col("", "age")
6. Reduce: col, col → colList  (creates vector<Col>)
7. Reduce: colList → selector
8. Shift: FROM
9. Reduce: IDENTIFIER("users") → tbName → tableList
10. Shift: WHERE
11. Reduce: col (age) → Col("", "age")
12. Shift: >
13. Reduce: VALUE_INT(18) → IntLit(18)
14. Reduce: col > value → BinaryExpr(Col("age"), SV_OP_GT, IntLit(18))
15. Reduce: BinaryExpr → condition → whereClause
16. Reduce: SELECT selector FROM tableList WHERE whereClause → stmt
17. Reduce: stmt ; → start
```

**Output:** AST tree structure

```
SelectStmt
├── cols: [Col("", "name"), Col("", "age")]
├── tabs: ["users"]
└── conds: [BinaryExpr(
              lhs: Col("", "age"),
              op: SV_OP_GT,
              rhs: IntLit(18)
           )]
```

### Step 3: Interpretation (interpreter.h)

**Input:** AST root node

**Process:**
```cpp
1. dynamic_pointer_cast<SelectStmt>(root) → Success, it's a SELECT

2. Extract columns:
   for (auto& sv_col : x->cols) {
       TabCol col("", sv_col->col_name);  // ("", "name"), ("", "age")
   }

3. Extract tables:
   vector<string> tables = x->tabs;  // ["users"]

4. Extract conditions:
   for (auto& expr : x->conds) {
       Condition cond;
       cond.lhs_col = TabCol("", expr->lhs->col_name);  // ("", "age")
       cond.op = interp_sv_comp_op(expr->op);           // OP_GT
       cond.is_rhs_val = true;
       cond.rhs_val.set_int(18);                        // Value(18)
   }

5. Execute:
   SelectResult result = QL_Manager::select_from(cols, tables, conds);

6. Return:
   return InterpreterResult(CMD_SELECT, true, result);
```

**Output:** `InterpreterResult` containing:
```cpp
InterpreterResult {
    command_type: CMD_SELECT,
    success: true,
    data: SelectResult {
        column_names: ["name", "age"],
        rows: [
            SelectRow { values: [RowValue("Alice"), RowValue("20")] },
            SelectRow { values: [RowValue("Bob"), RowValue("22")] },
            SelectRow { values: [RowValue("Carol"), RowValue("19")] }
        ]
    }
}
```

### Step 4: Application Display

**Input:** `InterpreterResult`

**Process:** Application code formats and displays results
```cpp
if (result.command_type == CMD_SELECT) {
    auto& sel = std::get<SelectResult>(result.data);
    
    // Print headers
    for (const auto& col : sel.column_names) {
        std::cout << col << "\t";
    }
    std::cout << "\n";
    
    // Print rows
    for (const auto& row : sel.rows) {
        for (const auto& val : row.values) {
            std::cout << val.value << "\t";
        }
        std::cout << "\n";
    }
}
```

**Output:** Formatted display
```
name    age
Alice   20
Bob     22
Carol   19

3 rows
```

---

## How to Use

### Basic Usage

```cpp
#include "parser/parser.h"
#include "interpreter.h"
#include <iostream>

int main() {
    std::string sql = "SELECT * FROM users WHERE age > 18;";
    
    try {
        // Step 1: Parse SQL → AST
        auto ast = parse_sql(sql);
        
        // Step 2: Execute AST → Result
        InterpreterResult result = Interpreter::interp_sql(ast);
        
        // Step 3: Check success
        if (!result.success) {
            std::cerr << "Error: " << result.error_message << "\n";
            return 1;
        }
        
        // Step 4: Process result based on command type
        if (result.command_type == CMD_SELECT) {
            auto& sel = std::get<SelectResult>(result.data);
            // Display results...
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
```

### Debugging Parser

```cpp
#include "parser/parser.h"
#include "parser/ast_printer.h"

// Parse and print AST for debugging
auto ast = parse_sql("CREATE TABLE users (id INT, name CHAR(50));");
std::string debug_str = TreePrinter::print(ast);
std::cout << debug_str;

// Output:
// CREATE_TABLE
//   users
//   LIST
//     COL_DEF
//       id
//       TYPE_LEN
//         INT
//         4
//     COL_DEF
//       name
//       TYPE_LEN
//         STRING
//         50
```

---

## Adding New Features

### Example: Adding a LIMIT clause to SELECT

#### Step 1: Modify `ast.h`
```cpp
struct SelectStmt : public TreeNode {
    std::vector<std::shared_ptr<Col>> cols;
    std::vector<std::string> tabs;
    std::vector<std::shared_ptr<BinaryExpr>> conds;
    int limit;  // ADD THIS
    
    SelectStmt(vector<shared_ptr<Col>> c, vector<string> t, 
               vector<shared_ptr<BinaryExpr>> w, int lim = -1)
        : cols(move(c)), tabs(move(t)), conds(move(w)), limit(lim) {}
};
```

#### Step 2: Modify `lex.l`
```lex
"LIMIT"     { return LIMIT; }
```

#### Step 3: Modify `yacc.y`

Add token:
```yacc
%token LIMIT
```

Modify grammar:
```yacc
dml:
        SELECT selector FROM tableList optWhereClause optLimitClause
    {
        $$ = std::make_shared<SelectStmt>($2, $4, $5, $6);
    }
    ;

optLimitClause:
        /* epsilon */ { $$ = -1; }
    |   LIMIT VALUE_INT { $$ = $2; }
    ;
```

#### Step 4: Modify `interpreter.h`
```cpp
else if (auto x = std::dynamic_pointer_cast<ast::SelectStmt>(root)) {
    // ... existing code ...
    
    // Pass limit to QL_Manager
    SelectResult result = QL_Manager::select_from(cols, tables, conds, x->limit);
    return InterpreterResult(CMD_SELECT, true, result);
}
```

#### Step 5: Rebuild
```bash
cd build
make clean
make
```

---

## Supported SQL Syntax

### Database Operations
```sql
SHOW DATABASES;
CREATE DATABASE dbname;
DROP DATABASE dbname;
USE dbname;
```

### Table Operations
```sql
SHOW TABLES;
DESC tablename;
CREATE TABLE tablename (col1 INT, col2 CHAR(n), col3 FLOAT, col4 DATETIME);
DROP TABLE tablename;
```

### Index Operations
```sql
CREATE INDEX tablename (colname);
DROP INDEX tablename (colname);
```

### Data Manipulation
```sql
INSERT INTO tablename VALUES (val1, val2, ...);
DELETE FROM tablename WHERE condition;
UPDATE tablename SET col1 = val1, col2 = val2 WHERE condition;
SELECT col1, col2 FROM tablename WHERE condition;
SELECT * FROM table1, table2 WHERE table1.col = table2.col;
```

### Supported Types
- `INT` - 4-byte integer
- `FLOAT` - 4-byte floating point
- `CHAR(n)` - Fixed-length string
- `DATETIME` - 19-character datetime ("YYYY-MM-DD HH:MM:SS")

### Supported Operators
- `=` - Equal
- `<>` - Not equal
- `<` - Less than
- `>` - Greater than
- `<=` - Less than or equal
- `>=` - Greater than or equal
- `AND` - Logical and (in WHERE clauses)

---

## Troubleshooting

### Red Squiggles in yacc.y

**Issue:** VSCode shows errors like "Type was not declared in the %union"

**Cause:** VSCode's C++ linter doesn't understand Bison's advanced features (custom semantic types defined via `%define api.value.type`)

**Solution:** These are **false positives**. The code compiles correctly. You can:
1. Ignore them (they don't affect compilation)
2. Add to `.vscode/settings.json`:
```json
{
    "C_Cpp.errorSquiggles": "Disabled"
}
```
3. Use Bison-specific IDE plugins

### Build Errors

If you get build errors after modifying parser files:

```bash
cd build
rm -f src/lex.yy.cpp src/yacc.tab.cpp src/yacc.tab.h
make clean
make
```

This forces regeneration of lexer/parser files.

### Parser Debugging

Enable Bison debug mode in `yacc.y`:
```yacc
%define parse.trace
```

Then run with debug output:
```bash
./your_program --debug
```

---

## File Summary

| File | Type | Purpose | Generated? |
|------|------|---------|------------|
| `lex.l` | Flex | Lexer specification | Source |
| `yacc.y` | Bison | Parser specification | Source |
| `ast.h` | C++ | AST node definitions | Source |
| `ast.cpp` | C++ | AST implementation | Source |
| `ast_printer.h` | C++ | AST debug printer | Source |
| `parser.h` | C++ | Parser API header (includes all parser components) | Source |
| `parser_defs.h` | C++ | Parser function declarations and parse_sql() wrapper | Source |
| `lex.yy.cpp` | C++ | Generated lexer code | Generated |
| `yacc.tab.cpp` | C++ | Generated parser code | Generated |
| `yacc.tab.h` | C++ | Generated parser header | Generated |

**Note:** Generated files are automatically created by CMake during build. Never edit them manually.

---

## References

- **Flex Manual:** https://westes.github.io/flex/manual/
- **Bison Manual:** https://www.gnu.org/software/bison/manual/
- **Dragon Book:** "Compilers: Principles, Techniques, and Tools" by Aho, Sethi, Ullman
- **Crafting Interpreters:** https://craftinginterpreters.com/

---

## Summary

The Parser Module is a critical component that bridges raw SQL text and executable database operations. By leveraging Flex and Bison, it provides:

✅ **Robust tokenization** - Handles all SQL keywords, operators, and literals
✅ **Flexible grammar** - Easy to extend with new SQL features  
✅ **Clear structure** - Separation of lexing, parsing, AST, and interpretation
✅ **Type safety** - Strong typing throughout the pipeline
✅ **Error reporting** - Detailed syntax error messages with line/column info
✅ **Debuggability** - AST printer for visualization
✅ **Production ready** - Industry-standard tools used by major compilers

The interpreter completes the pipeline by executing parsed ASTs and returning structured results without console logging, making the entire system suitable for embedding in applications, APIs, and GUIs.
