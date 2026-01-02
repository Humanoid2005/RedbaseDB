include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

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

# Link lexer and parser
ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

add_library(AUTODB-cpp STATIC
        database_engine/page_file_handler/pf_manager.cpp database_engine/page_file_handler/pf_pager.cpp
        database_engine/record_manager/rm_manager.cpp database_engine/record_manager/rm_iterator.cpp database_engine/record_manager/rm_file_handle.cpp
        database_engine/index_handler/ndx_manager.cpp database_engine/index_handler/ndx_iterator.cpp database_engine/index_handler/ndx_handler.cpp
        database_engine/system_management/sm_manager.cpp
        database_engine/query_language/ql_manager.cpp database_engine/query_language/ql_node.cpp
        database_engine/parser/ast.cpp
        database_engine/concurrency.cpp
        ${BISON_Parser_OUTPUTS}
        ${FLEX_Lexer_OUTPUTS}
)

# Build database server executable
add_executable(db_server db_server.cpp)
target_link_libraries(db_server AUTODB-cpp pthread)

# Build database client executable
add_executable(db_client db_client.cpp)

if (AUTODB_ENABLE_TEST)
    file(GLOB_RECURSE AUTODB_TEST_FILES *_test.cpp)
    foreach (AUTODB_TEST_FILE ${AUTODB_TEST_FILES})
        get_filename_component(AUTODB_TEST_NAME ${AUTODB_TEST_FILE} NAME_WE)
        add_executable(${AUTODB_TEST_NAME} ${AUTODB_TEST_FILE})
        target_link_libraries(${AUTODB_TEST_NAME} AUTODB-cpp gtest_main)
        gtest_discover_tests(${AUTODB_TEST_NAME})
    endforeach ()
endif ()