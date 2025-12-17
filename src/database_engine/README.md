# Database Engine Architecture

## Overview

This is a **relational database management system (RDBMS)** implemented in C++ that provides SQL query processing, storage management, and indexing capabilities. The architecture follows a **layered/modular design pattern** where each layer has well-defined responsibilities and interfaces.

## Software Architecture

### Architecture Pattern: **Layered Architecture**

The database engine follows a **bottom-up layered architecture** where higher layers depend on lower layers, but not vice versa. This promotes:
- **Separation of concerns** - Each layer handles a specific aspect of database functionality
- **Modularity** - Components can be developed, tested, and maintained independently
- **Reusability** - Lower layers provide reusable services to higher layers
- **Abstraction** - Each layer hides implementation details from layers above

---

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        CLIENT LAYER                              │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │              SQL Interface / Client Applications           │ │
│  └────────────────────────────────────────────────────────────┘ │
└───────────────────────────────────┬─────────────────────────────┘
                                    │
┌───────────────────────────────────▼─────────────────────────────┐
│                     INTERPRETER LAYER                            │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Interpreter (interpreter.h, interpreter_defs.h)           │ │
│  │  - Executes AST nodes                                      │ │
│  │  - Returns structured results (InterpreterResult)          │ │
│  └────────────────────────────────────────────────────────────┘ │
└───────────────────────────────────┬─────────────────────────────┘
                                    │
┌───────────────────────────────────▼─────────────────────────────┐
│                      PARSER LAYER                                │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Parser Module (parser/)                                   │ │
│  │  ├─ Lexer (lex.l) - Tokenization                          │ │
│  │  ├─ Parser (yacc.y) - Syntax Analysis                     │ │
│  │  ├─ AST (ast.h, ast.cpp) - Abstract Syntax Tree           │ │
│  │  └─ AST Printer (ast_printer.h) - Visualization           │ │
│  └────────────────────────────────────────────────────────────┘ │
└───────────────────────────────────┬─────────────────────────────┘
                                    │
┌───────────────────────────────────▼─────────────────────────────┐
│                   QUERY PROCESSING LAYER                         │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  Query Language Module (query_language/)                   │ │
│  │  ├─ QL Manager (ql_manager.h/cpp)                         │ │
│  │  │   - SELECT, INSERT, UPDATE, DELETE operations          │ │
│  │  ├─ QL Node (ql_node.h/cpp)                              │ │
│  │  │   - Query execution nodes                              │ │
│  │  │   - Table scan, Join, Projection                       │ │
│  │  └─ QL Defs (ql_defs.h) - Type definitions               │ │
│  └────────────────────────────────────────────────────────────┘ │
└───────────────────────────────────┬─────────────────────────────┘
                                    │
┌───────────────────────────────────▼─────────────────────────────┐
│                SYSTEM MANAGEMENT LAYER                           │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │  System Management Module (system_management/)             │ │
│  │  ├─ SM Manager (sm_manager.h/cpp)                         │ │
│  │  │   - Database creation/deletion                          │ │
│  │  │   - Table creation/deletion                             │ │
│  │  │   - Index management                                    │ │
│  │  ├─ SM Meta (sm_meta.h) - Metadata structures             │ │
│  │  │   - Table metadata, Column metadata                     │ │
│  │  └─ SM Defs (sm_defs.h) - Type definitions               │ │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────┬──────────────────────┬───────────────────┘
                      │                      │
        ┌─────────────▼──────────┐  ┌────────▼──────────────┐
        │  INDEX HANDLER LAYER   │  │  RECORD MANAGER LAYER │
        │ ┌────────────────────┐ │  │ ┌──────────────────┐ │
        │ │ Index Handler      │ │  │ │ Record Manager   │ │
        │ │ (index_handler/)   │ │  │ │ (record_manager/)│ │
        │ │                    │ │  │ │                  │ │
        │ │ - B+ Tree Index    │ │  │ │ - Record storage │ │
        │ │ - NDX Manager      │ │  │ │ - RM Manager     │ │
        │ │ - NDX Handler      │ │  │ │ - RM File Handle │ │
        │ │ - NDX Iterator     │ │  │ │ - RM Iterator    │ │
        │ │ - Key management   │ │  │ │ - Bitmap (slots) │ │
        │ └────────────────────┘ │  │ └──────────────────┘ │
        └────────────┬───────────┘  └──────────┬───────────┘
                     │                         │
                     └─────────┬───────────────┘
                               │
              ┌────────────────▼────────────────┐
              │    PAGE FILE HANDLER LAYER      │
              │  ┌────────────────────────────┐ │
              │  │ Page File Handler          │ │
              │  │ (page_file_handler/)       │ │
              │  │                            │ │
              │  │ - PF Manager (file ops)    │ │
              │  │ - PF Pager (cache/buffer)  │ │
              │  │ - Page management          │ │
              │  │ - LRU cache (65536 pages)  │ │
              │  └────────────────────────────┘ │
              └─────────────────────────────────┘
                               │
              ┌────────────────▼────────────────┐
              │       DISK STORAGE              │
              │  - Database files               │
              │  - Table data files             │
              │  - Index files                  │
              └─────────────────────────────────┘
```

---

## Layer Descriptions

### 1. **Page File Handler** (page_file_handler/)
**Responsibility**: Low-level page and file management
- Manages disk I/O operations
- Implements buffer pool with LRU cache (65,536 pages)
- Page size: 4096 bytes
- Handles file creation, deletion, and page read/write operations
- **Key Files**: `pf_manager.cpp`, `pf_pager.cpp`, `pf_defs.h`

### 2. **Record Manager** (record_manager/)
**Responsibility**: Record-level storage and retrieval
- Manages fixed-length records within pages
- Implements bitmap-based slot management for free space tracking
- Supports record insertion, deletion, and updates
- Provides iterators for sequential record scanning
- Maximum record size: 512 bytes
- **Key Files**: `rm_manager.cpp`, `rm_file_handle.cpp`, `rm_iterator.cpp`, `bitmap.h`

### 3. **Index Handler** (index_handler/)
**Responsibility**: B+ Tree index management
- Implements B+ Tree indexing for fast lookups
- Supports indexes on INT, FLOAT, STRING, and DATETIME columns
- Provides efficient range queries and point lookups
- Handles index creation, insertion, deletion, and searching
- Maximum key column length: 512 bytes
- **Key Files**: `ndx_manager.cpp`, `ndx_handler.cpp`, `ndx_iterator.cpp`, `ndx_defs.h`

### 4. **System Management** (system_management/)
**Responsibility**: Database and schema management
- Manages multiple databases
- Handles table creation and deletion with metadata storage
- Manages column definitions (name, type, length)
- Coordinates between Record Manager and Index Handler
- Stores metadata in system catalogs
- **Key Files**: `sm_manager.cpp`, `sm_meta.h`, `sm_defs.h`

### 5. **Query Language** (query_language/)
**Responsibility**: Query execution
- Implements relational operators: SELECT, INSERT, UPDATE, DELETE
- Query execution plans with operator nodes:
  - **QlNodeTable**: Table scan with filtering
  - **QlNodeJoin**: Nested loop join
  - **QlNodeProj**: Projection
- Condition evaluation and type checking
- **Key Files**: `ql_manager.cpp`, `ql_node.cpp`, `ql_defs.h`

### 6. **Parser** (parser/)
**Responsibility**: SQL parsing and AST generation
- Lexical analysis using Flex
- Syntax analysis using Bison
- Generates Abstract Syntax Tree (AST)
- Supports SQL DDL (CREATE, DROP, DESC, SHOW) and DML (SELECT, INSERT, UPDATE, DELETE)
- Multi-database support (CREATE DATABASE, USE DATABASE, etc.)
- **Key Files**: `lex.l`, `yacc.y`, `ast.h`, `ast.cpp`

### 7. **Interpreter** (interpreter.h, interpreter_defs.h)
**Responsibility**: AST execution and result formatting
- Traverses AST and invokes appropriate operations
- Returns structured results (InterpreterResult) instead of console output
- Maps AST types to execution layer types
- Handles error propagation for proper exception handling

---

## Key Features

### ✅ **Multi-Database Support**
- Create, drop, and switch between multiple databases
- Each database has its own set of tables and indexes
- SQL Commands: `CREATE DATABASE`, `DROP DATABASE`, `USE DATABASE`, `SHOW DATABASES`

### ✅ **SQL Support**
**DDL (Data Definition Language):**
- `CREATE TABLE` - Define tables with columns
- `DROP TABLE` - Delete tables
- `CREATE INDEX` - Build B+ Tree indexes
- `DROP INDEX` - Remove indexes
- `DESC TABLE` - Show table schema
- `SHOW TABLES` - List all tables

**DML (Data Manipulation Language):**
- `SELECT` - Query data with joins, projections, and WHERE conditions
- `INSERT` - Add records
- `UPDATE` - Modify records
- `DELETE` - Remove records

### ✅ **Data Types**
- `INT` - 32-bit integers
- `FLOAT` - 32-bit floating point
- `CHAR(n)` / `STRING` - Fixed/variable length strings
- `DATETIME` - Date and time (stored as DateTime object with 6 integer fields)

### ✅ **Indexing**
- **B+ Tree indexes** for efficient lookups
- Supports all data types
- Automatic index maintenance on INSERT/UPDATE/DELETE
- Range queries and point lookups

### ✅ **Query Optimization**
- Condition pushdown to reduce intermediate results
- Index utilization for faster access
- Nested loop joins

### ✅ **Buffer Management**
- LRU (Least Recently Used) page replacement
- 65,536-page buffer pool
- Efficient disk I/O minimization

---

## Design Principles

### 1. **Type Safety**
- Strong typing with ColumnType enum (TYPE_INT, TYPE_FLOAT, TYPE_STRING, TYPE_DATETIME)
- Type checking during query execution
- Compile-time constants using `constexpr` instead of `#define`

### 2. **Error Handling**
- Custom exception hierarchy (RedBaseError base class)
- Specific exceptions: `TableNotFoundError`, `ColumnNotFoundError`, `IncompatibleTypeError`, etc.
- Exceptions propagate to client layer for proper error reporting

### 3. **Memory Management**
- RAII (Resource Acquisition Is Initialization) pattern
- Smart pointers (`std::shared_ptr`, `std::unique_ptr`) for automatic cleanup
- No manual memory management in most cases

### 4. **Modularity**
- Each module has clear interfaces (`.h` files)
- Implementation details hidden in `.cpp` files
- Minimal coupling between layers

### 5. **Testability**
- Each layer has corresponding unit tests (`*_test.cpp`)
- Google Test framework integration
- Tests cover: Page File Handler, Record Manager, Index Handler, Parser, Query Language, System Management

---

## Configuration Constants

Key system parameters (defined in `*_defs.h` files):

| Constant | Value | Description |
|----------|-------|-------------|
| `PAGE_SIZE` | 4096 | Size of each page in bytes |
| `NUM_CACHE_PAGES` | 65536 | Number of pages in buffer cache |
| `RM_MAX_RECORD_SIZE` | 512 | Maximum record size in bytes |
| `NDX_MAX_COL_LEN` | 512 | Maximum index key length in bytes |
| `NDX_INIT_ROOT_PAGE` | 2 | Initial root page of B+ tree |

---

## Dataflow Example: SELECT Query

```
1. Client submits SQL: "SELECT * FROM users WHERE age > 25"
                  ↓
2. Parser (lex.l + yacc.y) tokenizes and parses SQL
                  ↓
3. AST generated (SelectStmt node with conditions)
                  ↓
4. Interpreter traverses AST and calls QL_Manager::select_from()
                  ↓
5. Query Language creates execution plan:
   - QlNodeTable(users, age > 25) → filters records
   - QlNodeProj(*) → projects all columns
                  ↓
6. QlNodeTable requests records from System Management
                  ↓
7. SM_Manager coordinates between Record Manager (data) and Index Handler (optional)
                  ↓
8. Record Manager reads pages using Page File Handler
                  ↓
9. Pages loaded from disk into buffer cache (LRU managed by PF_Pager)
                  ↓
10. Records filtered by condition, results sent back up the stack
                  ↓
11. Interpreter formats results as SelectResult
                  ↓
12. Client receives structured result (rows, columns, metadata)
```

---

## File Organization

```
database_engine/
├── datetime.h              # DateTime class for DATETIME type
├── db_defs.h              # Core type definitions (ColumnType, RecordID)
├── error.h                # Exception hierarchy
├── interpreter.h          # AST interpreter
├── interpreter_defs.h     # Result structures
│
├── parser/                # SQL Parsing Layer
│   ├── ast.h, ast.cpp     # Abstract Syntax Tree nodes
│   ├── lex.l              # Flex lexer (tokenization)
│   ├── yacc.y             # Bison parser (syntax analysis)
│   ├── ast_printer.h      # AST visualization
│   └── parser_defs.h      # Parser utilities
│
├── query_language/        # Query Execution Layer
│   ├── ql_manager.h/cpp   # Query operations (SELECT, INSERT, etc.)
│   ├── ql_node.h/cpp      # Execution operators (scan, join, project)
│   └── ql_defs.h          # Value, Condition structures
│
├── system_management/     # Schema Management Layer
│   ├── sm_manager.h/cpp   # Database/table/index management
│   ├── sm_meta.h          # Metadata structures
│   └── sm_defs.h          # Type definitions
│
├── index_handler/         # B+ Tree Index Layer
│   ├── ndx_manager.h/cpp  # Index file management
│   ├── ndx_handler.h/cpp  # B+ tree operations
│   ├── ndx_iterator.h/cpp # Index scanning
│   └── ndx_defs.h         # Index structures
│
├── record_manager/        # Record Storage Layer
│   ├── rm_manager.h/cpp   # Record file management
│   ├── rm_file_handle.h/cpp # Record operations
│   ├── rm_iterator.h/cpp  # Record scanning
│   ├── bitmap.h           # Free space management
│   └── rm_defs.h          # Record structures
│
└── page_file_handler/     # Page & Buffer Management Layer
    ├── pf_manager.h/cpp   # File operations
    ├── pf_pager.h/cpp     # Buffer pool & LRU cache
    └── pf_defs.h          # Page structures
```

---

## Building and Testing

```bash
# Configure
cd build
cmake ..

# Build
make

# Run all tests
ctest

# Individual test categories
./bin/pf_test       # Page file handler tests
./bin/rm_test       # Record manager tests
./bin/ndx_test      # Index handler tests
./bin/parser_test   # Parser tests
./bin/ql_test       # Query language tests
./bin/sm_test       # System management tests
```

---

## Future Enhancements

- **Query optimization**: Cost-based optimizer, join ordering
- **Concurrency control**: Locking, MVCC (Multi-Version Concurrency Control)
- **Transaction management**: ACID properties, logging, recovery
- **Advanced indexing**: Hash indexes, bitmap indexes
- **Storage optimization**: Compression, variable-length records
- **Distributed execution**: Sharding, replication

---

## References

- **Architecture Pattern**: Layered Architecture (similar to PostgreSQL, MySQL architecture)
- **B+ Tree Implementation**: Based on classic database textbooks
- **Buffer Management**: LRU replacement policy
- **Parser Generation**: Flex & Bison tools for lexical and syntax analysis
