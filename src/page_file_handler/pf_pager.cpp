#include "pf_pager.h"
#include <unistd.h>
#include <cassert>

PF_Pager::PF_Pager()
{
    for (size_t i = 0; i < NUM_BUFFER_PAGES; i++)
    {
        pages[i].buf = cache + i * PAGE_SIZE;
        pages[i].is_dirty = false;
        free_pages.push_back(&pages[i]);
    }
}

PF_Pager::~PF_Pager()
{
    flush_all();
    assert(free_pages.size() == NUM_BUFFER_PAGES);
}


/*
Function: read page from disk
Description:   
    This function reads a page from the disk and stores it in the provided buffer.
    It uses lseek to move the file pointer to the correct position and then reads
    the specified number of bytes into the buffer. If the read operation fails,
    it throws a runtime error.
*/
void PF_Pager::read_page(int fd, int page_no, uint8_t *buf, int num_bytes)
{
    lseek(fd, page_no * PAGE_SIZE, SEEK_SET);
    ssize_t bytes_read = read(fd, buf, num_bytes);
    if (bytes_read != num_bytes)
    {
        throw UnixError();
    }
}


/*
Function: write page to disk
Description:   
    This function writes a page to the disk from the provided buffer.
    It uses lseek to move the file pointer to the correct position and then writes
    the specified number of bytes from the buffer to the disk. If the write
    operation fails, it throws a runtime error.
*/
void PF_Pager::write_page(int fd, int page_no, const uint8_t *buf, int num_bytes) {
    lseek(fd, page_no * PAGE_SIZE, SEEK_SET);
    ssize_t bytes_write = write(fd, buf, num_bytes);
    if (bytes_write != num_bytes) {
        throw UnixError();
    }
}

/*
Function: create page
Description:   
    This function creates a new page in the cache and marks it as dirty.
    It uses the get_page function to allocate a new page and then marks
    it as dirty. The page is returned to the caller.
*/
Page *PF_Pager::create_page(int fd, int page_no) {
    Page *page = get_page<false>(fd, page_no);
    page->mark_dirty();
    return page;
}


/*
Function: fetch page
Description:   
    This function fetches a page from the cache. If the page is not in memory,
    it allocates a new page and reads it from disk. If the page is already in
    memory, it updates its position in the cache according to the LRU
    (Least Recently Used) policy. The function takes a boolean parameter EXISTS
    to indicate whether the page exists on disk or not. If EXISTS is true, it 
    reads the page from disk; otherwise, it allocates a new page.
*/
Page *PF_Pager::fetch_page(int fd, int page_no) { 
    return get_page<true>(fd, page_no); 
}


/*
Function: flush_file
Description:   
    This function flushes all pages associated with a given file descriptor
    to disk. It iterates through the busy pages list and flushes each page
    that belongs to the specified file descriptor. After flushing, it moves
    the flushed pages from the busy pages list to the free pages list.
*/
void PF_Pager::flush_file(int fd) {
    auto it_page = busy_pages.begin();
    while (it_page != busy_pages.end()) {
        auto prev_page = it_page;
        it_page++;
        if ((*prev_page)->id.fd == fd) {
            flush_page(*prev_page);
        }
    }
}

/*
Function: force_page
Description:   
    This function forces a page to be written to disk if it is dirty.
    It uses the write_page function to write the page to disk and then
    marks the page as clean (not dirty).
*/
void PF_Pager::force_page(Page *page) {
    if (page->is_dirty) {
        write_page(page->id.fd, page->id.page_number, page->buf, PAGE_SIZE);
        page->is_dirty = false;
    }
}

/*
Function: get_page
Description:   
    This function retrieves a page from the cache. If the page is not in memory,
    it allocates a new page and reads it from disk. If the page is already in
    memory, it updates its position in the cache according to the LRU
    (Least Recently Used) policy. The function takes a boolean parameter EXISTS
    to indicate whether the page exists on disk or not. If EXISTS is true, it 
    reads the page from disk; otherwise, it allocates a new page.
    The function uses a hash map to keep track of the pages in memory and a 
    linked list to maintain the order of the pages according to the LRU policy.
    The function returns a pointer to the page.
*/
template <bool EXISTS>
Page *PF_Pager::get_page(int fd, int page_no) {
    Page *page;
    PageID page_id(fd, page_no);
    auto map_it = busy_map.find(page_id);
    if (map_it == busy_map.end()) {
        // Page is not in memory (i.e. on disk). Allocate new cache page for it.
        if (free_pages.empty()) {
            // Cache is full. Need to flush a page to disk.
            assert(!busy_pages.empty());
            force_page(busy_pages.back());
            busy_map.erase(busy_pages.back()->id);
            busy_pages.splice(busy_pages.begin(), busy_pages, --busy_pages.end());
        } else {
            // Cache is not full. Allocate from free pages.
            busy_pages.splice(busy_pages.begin(), free_pages, free_pages.begin());
        }
        busy_map[page_id] = busy_pages.begin();
        page = busy_pages.front();
        page->id = page_id;
        page->is_dirty = false;
        if (EXISTS) {
            read_page(fd, page_no, page->buf, PAGE_SIZE);
        }
    } else {
        // Page is in memory
        page = *map_it->second;
        access(page);
    }
    return page;
}

/*
Function: access
Description:   
    This function updates the position of a page in the cache according to
    the LRU (Least Recently Used) policy. It moves the accessed page to the
    front of the busy pages list, indicating that it has been recently used.
*/
void PF_Pager::access(Page *page) {
    assert(in_cache(page->id));
    busy_pages.splice(busy_pages.begin(), busy_pages, busy_map[page->id]);
}

/*
Function: flush_page
Description:   
    This function flushes a page to disk. It first checks if the page is in
    memory and then writes it to disk if it is dirty. After flushing, it
    removes the page from the busy pages list and adds it to the free pages
    list. The function also updates the busy map accordingly.
*/
void PF_Pager::flush_page(Page *page) {
    assert(in_cache(page->id));
    auto map_it = busy_map.find(page->id);
    auto busy_it = map_it->second;
    force_page(page);
    free_pages.splice(free_pages.begin(), busy_pages, busy_it);
    busy_map.erase(map_it);
    assert(!in_cache(page->id));
}

/*
Function: flush_all
Description:   
    This function flushes all pages in the cache to disk. It iterates through
    the busy pages list, flushing each page to disk. After flushing, it moves
    all pages from the busy pages list to the free pages list and clears the
    busy map.
*/
void PF_Pager::flush_all() {
    for (Page *page : busy_pages) {
        force_page(page);
    }
    free_pages.insert(free_pages.end(), busy_pages.begin(), busy_pages.end());
    busy_pages.clear();
    busy_map.clear();
}