# System Management (Catalog Manager)

## Overview
The System Management module acts as the **catalog manager** for the database system. It handles database lifecycle (create, open, close, drop), table schema management, and maintains system catalogs that store metadata about databases, tables, and indexes.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        SM_Manager                                │
│              (Static Catalog Manager Interface)                  │
│                                                                   │
│  Static Members:                                                 │
│  • DB_Metadata db          - Current database metadata           │
│  • map<string, unique_ptr<RM_FileHandle>> fhs - Table files     │
│  • map<string, unique_ptr<IndexHandle>> ihs - Index files       │
│  • string current_db_name  - Active database                     │
│  • string base_dir         - Workspace absolute path             │
└────────────┬────────────────┬────────────────┬──────────────────┘
             │                │                │
             │                │                │
    ┌────────▼────────┐  ┌───▼────────┐  ┌───▼──────────┐
    │   Database Ops  │  │  Table Ops │  │   Index Ops  │
    ├─────────────────┤  ├────────────┤  ├──────────────┤
    │• create_db()    │  │• create_   │  │• create_     │
    │• open_db()      │  │  table()   │  │  index()     │
    │• close_db()     │  │• drop_     │  │• drop_       │
    │• drop_db()      │  │  table()   │  │  index()     │
    │• list_dbs()     │  │• show_     │  └──────────────┘
    │                 │  │  tables()  │
    │                 │  │• desc_     │
    │                 │  │  table()   │
    └─────────────────┘  └────────────┘
             │                │                │
             │                │                │
             ↓                ↓                ↓
    ┌──────────────────────────────────────────────────┐
    │           Metadata Structures                     │
    ├──────────────────────────────────────────────────┤
    │                                                   │
    │  DB_Metadata                                      │
    │  ├── string name                                  │
    │  └── vector<Table_Metadata> tables                │
    │                                                   │
    │  Table_Metadata                                   │
    │  ├── string name                                  │
    │  └── vector<Column_Metadata> cols                 │
    │                                                   │
    │  Column_Metadata                                  │
    │  ├── string tab_name, name                        │
    │  ├── ColumnType type                              │
    │  ├── int len, offset                              │
    │  └── bool index                                   │
    └──────────────────────────────────────────────────┘
             │
             ↓
    ┌──────────────────────────────────────────────────┐
    │          Integration Layer                        │
    ├──────────────────────────────────────────────────┤
    │                                                   │
    │  Record Manager (RM_FileHandle)                   │
    │  • Manages .rdb files (table data)                │
    │  • CRUD operations on records                     │
    │                                                   │
    │  Index Handler (IndexHandle)                      │
    │  • Manages .ndx files (B+ tree indexes)           │
    │  • Fast lookups and range scans                   │
    │                                                   │
    │  Page File Handler (PF_Manager/PF_Pager)          │
    │  • Low-level file I/O and buffering               │
    │  • Manages disk pages                             │
    └──────────────────────────────────────────────────┘
```

**Component Hierarchy:**
```
SM_Manager (Catalog Manager)
    ↓ creates/manages
RM_FileHandle (Record Manager)
    ↓ uses
PF_Pager (Page File Handler)
    ↓ manages
Buffer Pool & Disk I/O

SM_Manager (Catalog Manager)
    ↓ creates/manages
IndexHandle (Index Manager)
    ↓ uses
PF_Pager (Page File Handler)
```

## File Structure

### On-Disk Organization

```
workspace/
├── databases.list              # Registry of all databases
└── databases/
    ├── database1/
    │   ├── db.meta            # Database metadata
    │   ├── table1_0.rdb       # Record file for table1
    │   ├── table1_0.ndx       # Index file for table1, column 0
    │   ├── table2_0.rdb       # Record file for table2
    │   └── table2_1.ndx       # Index file for table2, column 1
    ├── database2/
    │   └── db.meta
    └── database3/
        └── db.meta
```

### Key Files

- **`databases.list`**: Central registry tracking all databases (one per line)
- **`db.meta`**: Per-database metadata (table schemas, column definitions, indexes)
- **`*.rdb`**: Record data files (managed by Record Manager)
- **`*.ndx`**: Index files (managed by Index Handler)

## Core Components

### 1. SM_Manager (sm_manager.h/cpp)

**Static class** providing the main interface for system operations.

#### State Management
```cpp
static std::string current_db_name;  // Currently open database
static std::string base_dir;         // Absolute path to workspace
static DB_Metadata db;               // In-memory catalog for current DB
static map<string, RM_FileHandle*> fhs;  // Open record files
static map<string, IndexHandle*> ihs;    // Open index files
```

#### Database Lifecycle

**Create Database:**
```cpp
create_db("mydb")
    ├── Check if database exists
    ├── Create databases/mydb/ directory
    ├── Initialize empty db.meta file
    └── Register in databases.list
```

**Open Database:**
```cpp
open_db("mydb")
    ├── Close any currently open database
    ├── Change directory to databases/mydb/
    ├── Load db.meta into memory (DB_Metadata)
    ├── Open all record files (.rdb)
    └── Open all index files (.ndx)
```

**Close Database:**
```cpp
close_db()
    ├── Write db.meta back to disk
    ├── Close all record file handles
    ├── Close all index file handles
    └── Clear current_db_name
```

**Drop Database:**
```cpp
drop_db("mydb")
    ├── Check database is not currently open
    ├── Remove databases/mydb/ directory (rm -rf)
    └── Unregister from databases.list
```

### 2. Metadata Structures (sm_meta.h)

#### Column_Metadata
```cpp
struct Column_Metadata {
    string tab_name;   // Parent table
    string name;       // Column name
    ColumnType type;   // INT, FLOAT, STRING, DATETIME
    int len;           // Byte length
    int offset;        // Offset in record
    bool index;        // Has index?
}
```

**Example:**
```
Table: users
Column 0: id (INT, 4 bytes, offset 0, indexed)
Column 1: name (STRING, 50 bytes, offset 4, not indexed)
Column 2: age (INT, 4 bytes, offset 54, indexed)
```

#### Table_Metadata
```cpp
struct Table_Metadata {
    string name;
    vector<Column_Metadata> cols;
    
    bool is_col(string col_name)  // Check if column exists
    Column_Metadata& get_col(string col_name)  // Get column info
}
```

Stores complete schema for a single table.

#### DB_Metadata
```cpp
struct DB_Metadata {
    string name;
    map<string, Table_Metadata> tabs;
    
    bool is_table(string tab_name)  // Check if table exists
    Table_Metadata& get_table(string tab_name)  // Get table schema
}
```

Stores all tables in the database. This is the in-memory catalog.

### 3. Output Structures (sm_defs.h)

For returning query results as structured data instead of printing to console:

#### ShowTablesResult
```cpp
struct ShowTablesResult {
    vector<TableInfo> tables;
    
    add_table(name)
    count()
}
```

**Usage:**
```cpp
auto result = SM_Manager::show_tables();
for (const auto& table : result.tables) {
    log_service.send(table.table_name);
}
```

#### DescTableResult
```cpp
struct DescTableResult {
    string table_name;
    vector<ColumnDescription> columns;
    
    add_column(field, type, has_index)
    count()
}
```

**Example:**
```cpp
auto result = SM_Manager::desc_table("users");
// result.columns[0] = {field: "id", type: "INT", has_index: "YES"}
// result.columns[1] = {field: "name", type: "STRING", has_index: "NO"}
```

## Key Operations

### 1. Create Table

```
create_table("users", columns)
    1. Validate no duplicate table name
    2. Calculate record layout:
       - offset[0] = 0
       - offset[i] = offset[i-1] + len[i-1]
       - record_size = sum of all lengths
    
    3. Create Table_Metadata:
       - Store column definitions
       - Set all index flags to false initially
    
    4. Create record file:
       - RM_Manager::create_file("users", record_size)
    
    5. Update db.meta:
       - Add table to DB_Metadata.tabs
       - Write to disk
    
    6. Open file handle:
       - fhs["users"] = RM_Manager::open_file("users")
```

**Column Offset Calculation:**
```
Table: products (id INT, name STRING(50), price FLOAT)

offset[id]    = 0
offset[name]  = 0 + 4 = 4
offset[price] = 4 + 50 = 54
record_size   = 54 + 4 = 58 bytes
```

### 2. Create Index

```
create_index("users", "id")
    1. Get table metadata
    2. Find column index for "id" (e.g., column 0)
    3. Check if index already exists (col.index == true)
    
    4. Create index file:
       - IndexManager::create_index("users", 0, type, len)
    
    5. Open index handle:
       - ihs["users_0.ndx"] = IndexManager::open_index("users", 0)
    
    6. Scan all existing records:
       - For each record in "users":
           key = extract_column(record, offset, len)
           rid = record's RecordID
           index->insert_entry(key, rid)
    
    7. Update metadata:
       - Set cols[0].index = true
       - Write db.meta to disk
```

**Multi-Column Indexes:**
- Each column can have its own index
- Index file naming: `tablename_colindex.ndx`
- Example: `users_0.ndx` (index on column 0), `users_2.ndx` (index on column 2)

### 3. Drop Table

```
drop_table("users")
    1. Close all associated file handles:
       - Close fhs["users"]
       - For each indexed column:
           Close ihs["users_i.ndx"]
    
    2. Delete physical files:
       - RM_Manager::destroy_file("users")  → deletes users_0.rdb
       - For each indexed column i:
           IndexManager::destroy_index("users", i)  → deletes users_i.ndx
    
    3. Update catalog:
       - Remove table from db.tabs
       - Write db.meta to disk
```

### 4. Show Tables

```
show_tables()
    1. Create ShowTablesResult
    2. For each table in db.tabs:
       - result.add_table(table.name)
    3. Return result (for logging/display)
```

### 5. Describe Table

```
desc_table("users")
    1. Get table metadata: db.get_table("users")
    2. Create DescTableResult
    3. For each column in table.cols:
       - field_name = col.name
       - type_name = type_to_string(col.type)
       - has_index = col.index ? "YES" : "NO"
       - result.add_column(field_name, type_name, has_index)
    4. Return result
```

## Path Management

### The Base Directory Problem

**Issue:** The system uses `chdir()` to change to database directories, but this breaks relative paths when switching databases.

**Solution:** Absolute path management:

```cpp
// On initialization
SM_Manager::base_dir = getcwd()  // e.g., "/workspace/project/build"

// All paths are absolute
get_db_path("mydb") → "/workspace/project/build/databases/mydb"
DB_LIST_FILE → "/workspace/project/build/databases.list"
```

This ensures operations work correctly regardless of current directory.

## Integration with Other Modules

```
SQL Query: "CREATE INDEX ON users(id)"
    ↓
SM_Manager::create_index("users", "id")
    ↓
IndexManager::create_index("users", 0, TYPE_INT, 4)
    ↓
Index Handler: Creates B+ tree in users_0.ndx
    ↓
RM_Manager: Scans users_0.rdb for all records
    ↓
For each record:
    Index Handler: insert_entry(key, rid)
```

```
SQL Query: "SELECT * FROM users WHERE id = 42"
    ↓
SM_Manager: Provides table schema
    ↓
Check if column "id" has index (cols[0].index == true)
    ↓
If indexed:
    IndexHandle::lower_bound(42) → RIDs
    For each RID:
        RM_FileHandle::get_record(RID) → record data
Else:
    RM_FileHandle: Full table scan
```

## Metadata Persistence

### Write Flow (on close_db or schema change)
```
In-Memory:              Disk:
DB_Metadata        →    db.meta
    ├── name            Format:
    ├── tabs[n]         <db_name>
    │   ├── name        <num_tables>
    │   ├── cols[m]     <table1_name>
    │       ├── name    <num_cols>
    │       ├── type    <col1_meta>
    │       ├── len     <col2_meta>
    │       ├── offset  ...
    │       └── index
```

### Read Flow (on open_db)
```
Disk: db.meta  →  Parse  →  DB_Metadata (in-memory)
```

The metadata is serialized using C++ stream operators (`operator<<` and `operator>>`).

## Multi-Database Support

### Database Isolation

Each database is completely isolated:
- Separate directory
- Independent namespace (table names can repeat across databases)
- Own set of open file handles

### Database Switching

```
open_db("db1")  → current_db_name = "db1", chdir("databases/db1/")
open_db("db2")  → Closes db1, current_db_name = "db2", chdir("databases/db2/")
```

Only **one database** can be open at a time (like traditional SQL databases using `USE database_name`).

### Database Registry

`databases.list` tracks all databases:
```
database1
database2
database3
```

Used by `list_databases()` to show available databases.

## Error Handling

- **DatabaseNotFoundError**: Trying to open/drop non-existent database
- **DatabaseExistsError**: Creating database that already exists
- **TableNotFoundError**: Referencing non-existent table
- **TableExistsError**: Creating duplicate table
- **ColumnNotFoundError**: Referencing non-existent column
- **InternalError**: Attempting to drop currently open database

## Design Decisions

1. **Static Class**: SM_Manager is a singleton-like static class since only one database is open at a time
2. **Directory per Database**: Clean isolation, easy to backup/delete entire databases
3. **Central Registry**: `databases.list` provides quick database discovery without filesystem scanning
4. **In-Memory Catalog**: `DB_Metadata` loaded entirely into memory for fast schema queries
5. **Structured Output**: `ShowTablesResult` and `DescTableResult` allow external services to consume metadata
6. **Absolute Paths**: Prevents path resolution issues when changing directories

## Files

- `sm_manager.h/cpp`: Main catalog manager interface
- `sm_meta.h`: Metadata structures and serialization
- `sm_defs.h`: Output structures for queries
- `sm_test.cpp`: Test suite for system management operations
