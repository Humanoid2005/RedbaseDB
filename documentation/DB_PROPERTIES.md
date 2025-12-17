# Set of all DB properties' constants

#### *static* --> to maintain internal linkage and not being misused by another file with same variable name
#### *constexpr* --> to tell that value is available at compile time
#### This is better than #define since this ensures type safety and is backed by the advantages of static,constexpr

### PAGE_SIZE  (src/page_file_handler/pf_defs.h)  4096 --> Size of a page
### NUM_CACHE_PAGES  (src/page_file_handler/pf_defs.h)  65536 --> Number of pages in cache

### WIDTH (src/record_manager/bitmap.h) 8 --> Number of bits in each bucket (1 byte).
### HIGHEST_BIT (src/record_manager/bitmap.h) 0x80u --> Used for masking the highest bit in a byte


### RM_NO_PAGE (src/record_manager/rm_defs.h) -1  --> Indicates no page
### RM_FILE_HDR_PAGE (src/record_manager/rm_defs.h) 0 --> First page (file header)
### RM_FIRST_RECORD_PAGE (src/record_manager/rm_defs.h) 1 --> Starting page number for actual records
### RM_MAX_RECORD_SIZE (src/record_manager/rm_defs.h) 512 --> Maximum size for any record

### NDX_NO_PAGE (src/index_handler/ndx_defs.h) -1 --> Represents an invalid or non-existent page
### NDX_FILE_HDR_PAGE (src/index_handler/ndx_defs.h) 0 --> Page number reserved for the file header
### NDX_LEAF_HEADER_PAGE (src/index_handler/ndx_defs.h) 1 --> Reserved page for leaf metadata
### NDX_INIT_ROOT_PAGE (src/index_handler/ndx_defs.h) 2 --> Initial root page of the B+ tree
### NDX_INIT_NUM_PAGES (src/index_handler/ndx_defs.h) 3 --> Initial number of pages when the index is created
### NDX_MAX_COL_LEN (src/index_handler/ndx_defs.h) 512 --> Maximum length allowed for a key column

### SERVER_PORT (src/db_server.cpp,src/db_client.cpp) 8888 --> Port on which the server is listening
### BUFFER_SIZE (src/db_server.cpp,src/db_client.cpp) 65536 --> Not used currently
### MAX_THREADS (src/db_server.cpp) 100 --> maximum number of concurrent threads handling clients