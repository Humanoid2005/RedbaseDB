include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

add_library(AUTODB-cpp STATIC
        page_file_handler/pf_manager.cpp page_file_handler/pf_pager.cpp
        record_manager/rm_manager.cpp record_manager/rm_iterator.cpp record_manager/rm_file_handle.cpp
        index_handler/ndx_manager.cpp index_handler/ndx_iterator.cpp index_handler/ndx_handler.cpp
)

if (AUTODB_ENABLE_TEST)
    file(GLOB_RECURSE AUTODB_TEST_FILES *_test.cpp)
    foreach (AUTODB_TEST_FILE ${AUTODB_TEST_FILES})
        get_filename_component(AUTODB_TEST_NAME ${AUTODB_TEST_FILE} NAME_WE)
        add_executable(${AUTODB_TEST_NAME} ${AUTODB_TEST_FILE})
        target_link_libraries(${AUTODB_TEST_NAME} AUTODB-cpp gtest_main)
        gtest_discover_tests(${AUTODB_TEST_NAME})
    endforeach ()
endif ()