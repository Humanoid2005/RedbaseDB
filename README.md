# AutoDB

A SQL Database written in C++

## System Overview

This is a relational database management system (RDBMS) with:
- **Storage Engine**: Page-based file management with LRU caching
- **Record Management**: Variable-length records with bitmap allocation
- **Index System**: B+ tree indexing for fast lookups
- **Query Language**: Full SQL support (CREATE, INSERT, SELECT, UPDATE, DELETE)
- **Client-Server**: TCP/IP-based remote access
- **Type System**: INT, FLOAT, CHAR(n), DATETIME
- **Concurrency Control**: Binary semaphores for in-memory structures + file locks (fcntl) for disk I/O

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Application Layer                       │
│                    (Client/Server/Tests)                     │
└─────────────────────────────────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                    Query Language (QL)                       │
│    SELECT, INSERT, UPDATE, DELETE with WHERE clauses        │
└─────────────────────────────────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                  System Management (SM)                      │
│    Database/Table creation, Schema management               │
└─────────────────────────────────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                   Index Handler (NDX)                        │
│           B+ tree indexing for fast lookups                  │
└─────────────────────────────────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                  Record Manager (RM)                         │
│     Variable-length records, bitmap allocation              │
└─────────────────────────────────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                  Page File Handler (PF)                      │
│        Page caching (LRU), buffering, I/O                    │
└─────────────────────────────────────────────────────────────┘
                              ▼
┌─────────────────────────────────────────────────────────────┐
│                      File System                             │
│                    Linux filesystem                          │
└─────────────────────────────────────────────────────────────┘
```

---

### Data Flow
```
Client                Server                Database Engine
  │                      │                         │
  │  1. QueryRequest     │                         │
  ├─────────────────────►│                         │
  │                      │  2. parse_sql()         │
  │                      ├────────────────────────►│
  │                      │  3. interp_sql()        │
  │                      ├────────────────────────►│
  │                      │  4. Execute query       │
  │                      │◄────────────────────────┤
  │                      │  5. Format result       │
  │  6. QueryResponse    │                         │
  │◄─────────────────────┤                         │
  │  7. Display result   │                         │
  │                      │                         │
```


## Project Structure
```
Redbase DB/
├── CMakeLists.txt              # Root build configuration
├── README.md                   # Main documentation
├── README_CLIENT_SERVER.md     # Client-server guide
├── IMPLEMENTATION_SUMMARY.md   # Technical summary
├── build/                      # Build artifacts
│   ├── bin/                    # Executables
│   │   ├── db_server           # Database server
│   │   ├── db_client           # Database client
│   │   └── *_test              # Test executables
│   └── lib/                    # Libraries
│       └── libAUTODB-cpp.a     # Core database library
├── src/                        # Source code
│   ├── CMakeLists.txt          # Source build config
│   ├── db_server.cpp           # Server implementation
│   ├── db_client.cpp           # Client implementation
│   └── database_engine/        # Core engine
│       ├── parser/             # SQL parser (Flex/Bison)
│       ├── interpreter.h       # SQL interpreter
│       ├── query_language/     # QL layer
│       ├── system_management/  # SM layer
│       ├── index_handler/      # NDX layer
│       ├── record_manager/     # RM layer
│       └── page_file_handler/  # PF layer
└── documentation/              # Additional docs
```

## Installation

### Prerequisites
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake flex bison
```

## Run Locally

Clone the project

```bash
  git clone https://github.com/Humanoid2005/AutoDB
```

Go to the project directory

```bash
  cd AutoDB
```

Create build folder and move to it

```bash
    mkdir build & cd build
```

Configure build directory

```bash
    cmake ..
```

Compile and build all files

```bash
    make -j
```

Run the client

```bash
    ./bin/db_client
```

Run the server

```bash
    ./bin/db_server
```


## Current Limitations

1. **No authentication**: Open access
2. **No encryption**: Plaintext transmission
3. **No connection pooling**: New connection per client
4. **No prepared statements**: Parse every query
5. **No transactions**: No BEGIN/COMMIT/ROLLBACK (concurrency control implemented)
6. **Buffer limits**: 4KB query, 1KB message

## Contributions
I welcome contributions! Feel free to fork the project and open a pull request for any improvements, bug fixes, or new features.

## License
This project is licensed under the MIT License. See the LICENSE file for more details.