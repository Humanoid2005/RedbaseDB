# Database Management System Makefile

# Compiler settings
CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -g -O2
CFLAGS = -Wall -Wextra -g -O2
LDFLAGS = 
INCLUDES = -I. -I./src -I./src/parser -I./src/linenoise -I./src/indexing_handler \
           -I./src/page_file_handler -I./src/query_language -I./src/record_manager \
           -I./src/system_management

# Flex and Bison settings
FLEX = flex
BISON = bison
BISONFLAGS = -d -v

# Build directory
BUILD_DIR = bin

# Target executable
TARGET = $(BUILD_DIR)/dbms

# Source directories
PARSER_DIR = ./src/parser
INDEXING_DIR = ./src/indexing_handler
PAGE_FILE_DIR = ./src/page_file_handler
QUERY_DIR = ./src/query_language
RECORD_DIR = ./src/record_manager
SYSTEM_DIR = ./src/system_management
LINENOISE_DIR = ./src/linenoise

# Header files for dependency tracking
HEADER_FILES = ./src/datetime.h \
               ./src/db_error.h \
               ./src/db_structs.h \
               ./src/interpreter.h \
               ./src/record_logger.h \
               $(PARSER_DIR)/abstract_syntax_tree.h \
               $(PARSER_DIR)/ast_printer.h \
               $(PARSER_DIR)/parser.h \
               $(PARSER_DIR)/parser_structs.h \
               $(INDEXING_DIR)/ndx.h \
               $(INDEXING_DIR)/ndx_handler.h \
               $(INDEXING_DIR)/ndx_manager.h \
               $(INDEXING_DIR)/ndx_scanner.h \
               $(INDEXING_DIR)/ndx_structs.h \
               $(PAGE_FILE_DIR)/pf.h \
               $(PAGE_FILE_DIR)/pf_manager.h \
               $(PAGE_FILE_DIR)/pf_pager.h \
               $(PAGE_FILE_DIR)/pf_structs.h \
               $(QUERY_DIR)/ql.h \
               $(QUERY_DIR)/ql_manager.h \
               $(QUERY_DIR)/ql_node.h \
               $(QUERY_DIR)/ql_structs.h \
               $(RECORD_DIR)/bitmap.h \
               $(RECORD_DIR)/rm.h \
               $(RECORD_DIR)/rm_handler.h \
               $(RECORD_DIR)/rm_manager.h \
               $(RECORD_DIR)/rm_scanner.h \
               $(RECORD_DIR)/rm_structs.h \
               $(SYSTEM_DIR)/sm.h \
               $(SYSTEM_DIR)/sm_manager.h \
               $(SYSTEM_DIR)/sm_meta.h \
               $(SYSTEM_DIR)/sm_structs.h \
               $(LINENOISE_DIR)/linenoise.h

# Generated parser files (stored in build directory)
PARSER_GENERATED = $(BUILD_DIR)/lex.yy.c $(BUILD_DIR)/yacc.tab.c $(BUILD_DIR)/yacc.tab.h

# Source files
MAIN_SOURCES = ./src/main.cpp

PARSER_SOURCES = $(PARSER_DIR)/abstract_syntax_tree.cpp

INDEXING_SOURCES = $(INDEXING_DIR)/ndx_handler.cpp \
                   $(INDEXING_DIR)/ndx_manager.cpp \
                   $(INDEXING_DIR)/ndx_scanner.cpp

PAGE_FILE_SOURCES = $(PAGE_FILE_DIR)/pf_manager.cpp \
                    $(PAGE_FILE_DIR)/pf_pager.cpp

QUERY_SOURCES = $(QUERY_DIR)/ql_manager.cpp \
                $(QUERY_DIR)/ql_node.cpp

RECORD_SOURCES = $(RECORD_DIR)/rm_handler.cpp \
                 $(RECORD_DIR)/rm_manager.cpp \
                 $(RECORD_DIR)/rm_scanner.cpp

SYSTEM_SOURCES = $(SYSTEM_DIR)/sm_manager.cpp

LINENOISE_SOURCES = $(LINENOISE_DIR)/linenoise.c

# All C++ source files
CPP_SOURCES = $(MAIN_SOURCES) $(PARSER_SOURCES) $(INDEXING_SOURCES) \
              $(PAGE_FILE_SOURCES) $(QUERY_SOURCES) $(RECORD_SOURCES) \
              $(SYSTEM_SOURCES)

# All C source files
C_SOURCES = $(LINENOISE_SOURCES)

# Object files (all stored in build directory)
CPP_OBJECTS = $(addprefix $(BUILD_DIR)/, $(CPP_SOURCES:.cpp=.o))
C_OBJECTS = $(addprefix $(BUILD_DIR)/, $(C_SOURCES:.c=.o))
PARSER_OBJECTS = $(BUILD_DIR)/lex.yy.o $(BUILD_DIR)/yacc.tab.o

ALL_OBJECTS = $(CPP_OBJECTS) $(C_OBJECTS) $(PARSER_OBJECTS)

# Create necessary subdirectories in build directory
BUILD_SUBDIRS = $(BUILD_DIR) \
                $(BUILD_DIR)/$(PARSER_DIR) \
                $(BUILD_DIR)/$(INDEXING_DIR) \
                $(BUILD_DIR)/$(PAGE_FILE_DIR) \
                $(BUILD_DIR)/$(QUERY_DIR) \
                $(BUILD_DIR)/$(RECORD_DIR) \
                $(BUILD_DIR)/$(SYSTEM_DIR) \
                $(BUILD_DIR)/$(LINENOISE_DIR)

# Default target
all: $(TARGET)

# Create build directories
$(BUILD_SUBDIRS):
	mkdir -p $@

# Build the main executable
$(TARGET): $(ALL_OBJECTS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Generate lexer from flex file
$(BUILD_DIR)/lex.yy.c: $(PARSER_DIR)/lex.l $(BUILD_DIR)/yacc.tab.h | $(BUILD_DIR)
	$(FLEX) -o $@ $<

# Generate parser from bison file
$(BUILD_DIR)/yacc.tab.c $(BUILD_DIR)/yacc.tab.h: $(PARSER_DIR)/yacc.y | $(BUILD_DIR)
	$(BISON) $(BISONFLAGS) -o $(BUILD_DIR)/yacc.tab.c $<

# Compile C++ source files
$(BUILD_DIR)/%.o: %.cpp $(HEADER_FILES) | $(BUILD_SUBDIRS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Compile C source files using gcc
$(BUILD_DIR)/%.o: %.c $(HEADER_FILES) | $(BUILD_SUBDIRS)
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile generated parser files
$(BUILD_DIR)/lex.yy.o: $(BUILD_DIR)/lex.yy.c | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(BUILD_DIR) -c $< -o $@

$(BUILD_DIR)/yacc.tab.o: $(BUILD_DIR)/yacc.tab.c | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I$(BUILD_DIR) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	@echo "Clean complete"

# Clean everything including backup files
distclean: clean
	rm -f *~ */*~ */*/*~
	rm -f *.bak */*.bak */*/*.bak

# Install target (optional)
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/dbms
	@echo "Installed dbms to /usr/local/bin/"

# Uninstall target (optional)
uninstall:
	rm -f /usr/local/bin/dbms
	@echo "Uninstalled dbms"

# Debug build
debug: CXXFLAGS += -DDEBUG -g3 -O0
debug: CFLAGS += -DDEBUG -g3 -O0
debug: $(TARGET)

# Release build
release: CXXFLAGS += -DNDEBUG -O3
release: CFLAGS += -DNDEBUG -O3
release: $(TARGET)

# Test target (if you have tests)
test: $(TARGET)
	./$(TARGET) < test_input.sql

# Run the program
run: $(TARGET)
	./$(TARGET)

# Show build information
info:
	@echo "Build configuration:"
	@echo "  Target: $(TARGET)"
	@echo "  Build directory: $(BUILD_DIR)"
	@echo "  C++ Compiler: $(CXX)"
	@echo "  C Compiler: $(CC)"
	@echo "  C++ Flags: $(CXXFLAGS)"
	@echo "  C Flags: $(CFLAGS)"
	@echo "  Sources: $(words $(CPP_SOURCES) $(C_SOURCES)) files"
	@echo "  Objects: $(words $(ALL_OBJECTS)) files"

# Check for missing source files
check-sources:
	@echo "Checking for missing source files..."
	@echo "Expected source files:"
	@for src in $(CPP_SOURCES) $(C_SOURCES); do \
		if [ ! -f "$src" ]; then \
			echo "  MISSING: $src"; \
		else \
			echo "  Found: $src"; \
		fi; \
	done
	@echo "Expected header files:"
	@for hdr in $(HEADER_FILES); do \
		if [ ! -f "$hdr" ]; then \
			echo "  MISSING: $hdr"; \
		else \
			echo "  Found: $hdr"; \
		fi; \
	done

# Show help
help:
	@echo "Available targets:"
	@echo "  all         - Build the main executable (default)"
	@echo "  clean       - Remove the entire build directory"
	@echo "  distclean   - Remove build directory and backup files"
	@echo "  debug       - Build with debug flags"
	@echo "  release     - Build with optimization flags"
	@echo "  install     - Install binary to /usr/local/bin/"
	@echo "  uninstall   - Remove binary from /usr/local/bin/"
	@echo "  test        - Run basic test (requires test_input.sql)"
	@echo "  run         - Build and run the program"
	@echo "  info        - Show build configuration"
	@echo "  check-sources - Check for missing source/header files"
	@echo "  help        - Show this help message"

# Dependency tracking
depend: $(CPP_SOURCES) $(C_SOURCES) | $(BUILD_DIR)
	$(CXX) -MM $(CXXFLAGS) $(INCLUDES) $^ | sed 's|^\([^:]*\):|$(BUILD_DIR)/\1:|' > $(BUILD_DIR)/.depend

# Include dependencies if they exist
-include $(BUILD_DIR)/.depend

# Phony targets
.PHONY: all clean distclean install uninstall debug release test run info help depend check-sources

# Pattern rules for different file types
.SUFFIXES: .cpp .c .o .h .l .y

# Additional compiler flags for specific files if needed
$(BUILD_DIR)/$(PARSER_DIR)/%.o: CXXFLAGS += -Wno-unused-function -Wno-sign-compare
$(BUILD_DIR)/lex.yy.o: CXXFLAGS += -Wno-unused-function -Wno-sign-compare
$(BUILD_DIR)/yacc.tab.o: CXXFLAGS += -Wno-unused-function -Wno-sign-compare

# Make sure generated files are not deleted as intermediate files
.PRECIOUS: $(BUILD_DIR)/lex.yy.c $(BUILD_DIR)/yacc.tab.c $(BUILD_DIR)/yacc.tab.h

# Force rebuild if Makefile changes
$(ALL_OBJECTS): Makefile