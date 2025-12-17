# Build Process Documentation

## Build Tools Overview

### What is Make?
**Make** is a build automation tool that compiles source code into executables. It reads a `Makefile` containing rules about how to build targets from source files.

- Tracks dependencies between files
- Only recompiles changed files (incremental builds)
- Can run build steps in parallel

### What is CMake?
**CMake** is a cross-platform build system generator. It doesn't compile code directly—it generates build files (like Makefiles) for various platforms.

- Write once (`CMakeLists.txt`), build anywhere (Linux, Windows, macOS)
- Handles complex dependencies automatically
- Generates platform-specific build systems (Make, Ninja, Visual Studio, etc.)

### What is CMakeLists.txt?
A configuration file that tells CMake how to build your project. Written in CMake's scripting language.

---

## CMakeLists.txt Basic Syntax

```cmake
# Set minimum CMake version
cmake_minimum_required(VERSION 3.16)

# Define project name
project(MyProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Add compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g")

# Create a library from source files
add_library(mylib STATIC file1.cpp file2.cpp)

# Create an executable
add_executable(myapp main.cpp)

# Link library to executable
target_link_libraries(myapp mylib pthread)

# Process subdirectory
add_subdirectory(src)

# Include directories for headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Find external packages
find_package(BISON REQUIRED)
```

---

## Common Commands

### CMake Commands
```bash
# Generate build system (from build directory)
cmake ..

# Generate with specific build type
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build using CMake (cross-platform)
cmake --build .

# Build specific target
cmake --build . --target db_server

# Clean build files
cmake --build . --target clean
```

### Make Commands
```bash
# Build all targets
make

# Build with parallel jobs (uses all CPU cores)
make -j

# Build with 4 parallel jobs
make -j4

# Build specific target only
make db_server

# Clean compiled files
make clean

# Verbose output (show all commands)
make VERBOSE=1
```

---

## Our Project Build Process

### Project Structure
```
Redbase DB/
├── CMakeLists.txt          # Root configuration
├── src/
│   ├── CMakeLists.txt      # Source configuration
│   ├── db_server.cpp
│   ├── db_client.cpp
│   └── database_engine/    # Core DB components
└── build/                  # Build directory (out-of-source)
```

### Root CMakeLists.txt Configuration

**Sets up project-wide settings:**
- Project name: `AutoDB`
- C++ standard: C++17
- Compiler flags: `-g -Wall -pedantic-errors`
- Output directories:
  - Executables → `build/bin/`
  - Libraries → `build/lib/`
- Enables GoogleTest framework (downloads automatically)
- Processes `src/` subdirectory

### Source CMakeLists.txt Configuration

**Defines build targets:**

1. **Parser/Lexer Generation**
   - Uses BISON to generate parser from `yacc.y`
   - Uses FLEX to generate lexer from `lex.l`
   - Output: `yacc.tab.cpp`, `yacc.tab.h`, `lex.yy.cpp`

2. **AUTODB-cpp Library** (static)
   - Page file handler
   - Record manager
   - Index handler
   - System management
   - Query language processor
   - Parser/lexer outputs

3. **Executables**
   - `db_server` - Links with AUTODB-cpp + pthread
   - `db_client` - Standalone client
   - Test executables - Link with AUTODB-cpp + gtest

### Build Workflow

#### Step 1: Configure (cmake ..)
```bash
cd build
cmake ..
```

**What happens:**
1. Reads root `CMakeLists.txt`
2. Downloads GoogleTest if needed
3. Finds BISON and FLEX tools
4. Reads `src/CMakeLists.txt`
5. Generates `Makefile` and dependency files
6. Creates build configuration in `build/`

**Output:** Build system ready, but no compilation yet.

#### Step 2: Build (make -j)
```bash
make -j
```

**What happens (in order):**

1. **Generate Parser/Lexer**
   ```
   yacc.y → yacc.tab.cpp + yacc.tab.h
   lex.l  → lex.yy.cpp
   ```

2. **Compile Library Sources** (parallel)
   ```
   pf_manager.cpp      → pf_manager.o
   rm_manager.cpp      → rm_manager.o
   ndx_manager.cpp     → ndx_manager.o
   sm_manager.cpp      → sm_manager.o
   ql_manager.cpp      → ql_manager.o
   ast.cpp             → ast.o
   yacc.tab.cpp        → yacc.tab.o
   lex.yy.cpp          → lex.yy.o
   ... (all sources)
   ```

3. **Archive Static Library**
   ```
   *.o files → libAUTODB-cpp.a (in lib/)
   ```

4. **Compile Executables** (parallel)
   ```
   db_server.cpp → db_server.o
   db_client.cpp → db_client.o
   *_test.cpp    → *_test.o
   ```

5. **Link Executables**
   ```
   db_server.o + libAUTODB-cpp.a + pthread → bin/db_server
   db_client.o                             → bin/db_client
   *_test.o + libAUTODB-cpp.a + gtest     → bin/*_test
   ```

**Output:** Executables in `build/bin/`, library in `build/lib/`

---

## Typical Development Workflow

```bash
# First time setup
mkdir build
cd build
cmake ..
make -j

# After modifying source code
make -j              # Only rebuilds changed files

# After modifying CMakeLists.txt
cmake ..             # Regenerate build system
make -j              # Rebuild

# Build specific target
make -j db_server    # Only build server

# Complete rebuild
make clean
make -j

# Run the server
./bin/db_server
```

---

## Why Out-of-Source Builds?

Building in a separate `build/` directory keeps source tree clean:
- Generated files don't mix with source code
- Easy to delete entire `build/` directory to start fresh
- Can have multiple build configurations (Debug, Release)
- Cleaner version control (`.gitignore` just `build/`)

---

## Key Advantages

**CMake:**
- Cross-platform (works on Linux, Windows, macOS)
- Manages complex dependencies automatically
- Integrates external tools (BISON, FLEX, GoogleTest)

**Make with -j:**
- Parallel compilation (much faster)
- Smart incremental builds (only changed files)
- Automatic dependency tracking

**Our Setup:**
- Clean separation: configuration (CMake) vs compilation (Make)
- Modular: separate CMakeLists for different components
- Efficient: parallel builds, out-of-source builds
