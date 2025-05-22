#ifndef PF_PAGER_H
#define PF_PAGER_H

#include "pf_structs.h"
#include "../db_error.h"
#include <list>
#include <unordered_map>

class PF_Pager
{
private:
    static void force_page(Page *page);

    // Get the page from memory corresponding to the disk page.
    // If the page is not in memory, allocate a page and read the disk.
    template <bool EXISTS>
    Page *get_page(int fd, int page_no);

    void access(Page *page);
    uint8_t cache[NUM_BUFFER_PAGES * PAGE_SIZE];
    Page pages[NUM_BUFFER_PAGES];
    std::unordered_map<PageID, std::list<Page *>::iterator> busy_map;
    std::list<Page *> busy_pages;
    std::list<Page *> free_pages;

public:

    static void read_page(int fd, int page_no, uint8_t *buf, int num_bytes);
    static void write_page(int fd, int page_no, const uint8_t *buf, int num_bytes);

    Page *create_page(int fd, int page_no);

    Page *fetch_page(int fd, int page_no);

    void flush_file(int fd);
    void flush_page(Page *page);
    void flush_all();

    bool in_cache(const PageID &page_id) const{
        return busy_map.find(page_id) != busy_map.end();
    }

    const std::list<Page *> &get_free_pages() const { 
        return free_pages; 
    }

    const std::list<Page *> &get_busy_pages() const { 
        return busy_pages; 
    }
    const std::unordered_map<PageID, std::list<Page *>::iterator> &get_busy_map() const { 
        return busy_map; 
    }
};

#endif