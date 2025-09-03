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