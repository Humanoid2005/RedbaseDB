# RedBase - A Relational Database Management System

A full-featured relational database management system written from scratch in C++17. RedBase implements a complete database engine with support for SQL queries, B+ tree indexing, transaction management, and a custom storage layer.

## Features

### Core Database Functionality
- **Complete SQL Support**: DDL and DML operations with standard SQL syntax
- **B+ Tree Indexing**: Efficient indexing with automatic index management
- **ACID Transactions**: Full transaction support with rollback capabilities
- **Multi-table Queries**: Support for joins and complex query operations
- **Custom Storage Engine**: Page-based storage with efficient caching

### Supported SQL Operations
```sql
-- Database Operations
CREATE DATABASE database_name;
USE DATABASE database_name;
DROP DATABASE database_name;

-- Table Operations
CREATE TABLE table_name (column_name type [, ...]);
DROP TABLE table_name;
DESC table_name;
SHOW TABLES;

-- Index Operations
CREATE INDEX table_name (column_name);
DROP INDEX table_name (column_name);

-- Data Manipulation
INSERT INTO table_name VALUES (value [, ...]);
DELETE FROM table_name [WHERE conditions];
UPDATE table_name SET column=value [, ...] [WHERE conditions];
SELECT columns FROM table_name [WHERE conditions];
```

### Supported Data Types
- `INT` - 32-bit integers
- `FLOAT` - Single precision floating point
- `CHAR(n)` - Fixed-length character strings
- `DATETIME` - Date and time values

## Architecture

RedBase follows a layered architecture with clear separation of concerns:

```
┌─────────────────────────────────────────┐
│              SQL Interface              │
│        (Parser + Interpreter)          │
├─────────────────────────────────────────┤
│            Query Language               │
│         (Query Processor)              │
├─────────────────────────────────────────┤
│          System Management             │
│        (Schema & Metadata)             │
├─────────────────────────────────────────┤
│            Index Handler               │
│          (B+ Tree Index)               │
├─────────────────────────────────────────┤
│           Record Manager               │
│        (Record Operations)             │
├─────────────────────────────────────────┤
│          Page File Handler             │
│      (Storage & Buffer Manager)        │
└─────────────────────────────────────────┘
```

### System Components

#### 1. Page File Handler (`src/page_file_handler/`)
- **PF_Manager**: File operations and lifecycle management
- **PF_Pager**: Buffer pool management with LRU eviction
- **PF_Structs**: Core data structures for page management
- Implements a page-based storage system with efficient caching

#### 2. Record Manager (`src/record_manager/`)
- **RM_Manager**: Record file management and operations
- **RM_Handler**: Record-level operations (CRUD)
- **RM_Scanner**: Sequential and conditional record scanning
- **Bitmap**: Efficient slot management for variable-length records

#### 3. Index Handler (`src/indexing_handler/`)
- **NDX_Manager**: Index file management and B+ tree operations
- **NDX_Handler**: Node-level B+ tree operations
- **NDX_Scanner**: Index-based record retrieval
- Implements B+ tree with automatic balancing and splitting

#### 4. System Management (`src/system_management/`)
- **SM_Manager**: Database and table schema management
- **SM_Meta**: Metadata structures and persistence
- Handles database catalogs and system metadata

#### 5. Query Language (`src/query_language/`)
- **QL_Manager**: Query execution and optimization
- **QL_Node**: Query execution tree nodes
- Implements join algorithms and query processing

#### 6. Parser (`src/parser/`)
- **Lexical Analyzer**: Tokenizes SQL input using Flex
- **Parser**: Builds AST from tokens using Bison (LALR(1))
- **AST**: Abstract syntax tree representation of SQL queries

##  Building the Project

### Prerequisites
- G++ compiler with C++17 support
- GNU Make
- Flex (lexical analyzer generator)
- Bison (parser generator)

### Build Commands
```bash
# Build the database system
make

# Build with debug symbols
make debug

# Build optimized release version
make release

# Clean build artifacts
make clean

# Install system-wide (optional)
sudo make install

# Run the database
make run
```

### Build Targets
- `all` - Build the main executable (default)
- `debug` - Build with debugging symbols and assertions
- `release` - Build with optimizations
- `clean` - Remove all build artifacts
- `install` - Install to `/usr/local/bin/`
- `test` - Run basic tests
- `info` - Show build configuration

##  Usage

### Starting the Database
```bash
# Start with a database name
./bin/dbms mydatabase

# If the database doesn't exist, it will be created automatically
```

### Interactive Shell
RedBase provides an interactive SQL shell with command history and line editing:

```
  ██████╗ ███████╗██████╗ ██████╗  █████╗ ███████╗███████╗
  ██╔══██╗██╔════╝██╔══██╗██╔══██╗██╔══██╗██╔════╝██╔════╝
  ██████╔╝█████╗  ██║  ██║██████╔╝███████║███████╗█████╗  
  ██╔══██╗██╔══╝  ██║  ██║██╔══██╗██╔══██║╚════██║██╔══╝  
  ██║  ██║███████╗██████╔╝██████╔╝██║  ██║███████║███████╗
  ╚═╝  ╚═╝╚══════╝╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝╚══════╝

Type 'help;' for help.

redbase> 
```

### Example Session
```sql
redbase> CREATE TABLE students (id INT, name CHAR(50), gpa FLOAT);
redbase> CREATE INDEX students (id);
redbase> INSERT INTO students VALUES (1, 'Alice', 3.8);
redbase> INSERT INTO students VALUES (2, 'Bob', 3.2);
redbase> SELECT * FROM students WHERE gpa > 3.5;
redbase> UPDATE students SET gpa = 3.9 WHERE name = 'Alice';
redbase> DELETE FROM students WHERE id = 2;
redbase> DESC students;
redbase> DROP TABLE students;
```

##  Project Structure

```
RedBase/
├── src/                          # Source code
│   ├── main.cpp                  # Application entry point
│   ├── interpreter.h             # SQL command interpreter
│   ├── db_structs.h             # Core database structures
│   ├── db_error.h               # Error handling
│   ├── datetime.h               # Date/time utilities
│   ├── record_logger.h          # Output formatting
│   ├── parser/                  # SQL parsing
│   │   ├── lex.l               # Lexical analyzer (Flex)
│   │   ├── yacc.y              # Grammar specification (Bison)
│   │   ├── abstract_syntax_tree.* # AST implementation
│   │   └── parser_structs.h    # Parser data structures
│   ├── page_file_handler/       # Storage layer
│   │   ├── pf_manager.*        # File management
│   │   ├── pf_pager.*          # Buffer management
│   │   └── pf_structs.h        # Page structures
│   ├── record_manager/          # Record operations
│   │   ├── rm_manager.*        # Record file management
│   │   ├── rm_handler.*        # Record operations
│   │   ├── rm_scanner.*        # Record scanning
│   │   └── bitmap.h            # Slot management
│   ├── indexing_handler/        # B+ tree indexing
│   │   ├── ndx_manager.*       # Index management
│   │   ├── ndx_handler.*       # B+ tree operations
│   │   └── ndx_scanner.*       # Index scanning
│   ├── query_language/          # Query processing
│   │   ├── ql_manager.*        # Query execution
│   │   └── ql_node.*           # Query tree nodes
│   ├── system_management/       # Schema management
│   │   ├── sm_manager.*        # System operations
│   │   └── sm_meta.h           # Metadata structures
│   └── linenoise/              # Command line interface
├── References/                  # Reference implementations
│   └── redis_src/              # Redis-inspired components
├── Makefile                    # Build configuration
├── LICENSE                     # MIT License
└── README.md                   # This file
```

##  Advanced Features

### Index Management
- Automatic B+ tree balancing
- Support for range queries and point lookups

### Buffer Management
- LRU page replacement policy
- Configurable buffer pool size
- Efficient page I/O with minimal disk access

### Error Handling
- Comprehensive error reporting
- Type-safe error classes
- Graceful error recovery


##  Technical Details

### Storage Format
- **Page Size**: 4KB (configurable)
- **Record Format**: Slotted page with bitmap allocation
- **Index Format**: B+ tree with configurable order
- **File Format**: Platform-independent binary format

### Performance Characteristics
- **Index Operations**: O(log n) search, insert, delete
- **Table Scans**: Sequential with efficient buffering
- **Memory Usage**: Configurable buffer pool with LRU eviction


## References

[RedBase Stanford Course](https://web.stanford.edu/class/cs346/2015/redbase.html)
