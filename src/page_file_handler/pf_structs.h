#ifndef PF_STRUCTS_H
#define PF_STRUCTS_H

#include <cstdlib>
#include <iostream>
#include <cinttypes>
#include <functional>

/*
static --> to maintain internal linkage and not being misused by another file with same variable name
constexpr --> to tell that value is available at compile time
This is better than #define since this ensures type safety and is backed by the advantages of static,constexpr
*/
static constexpr int PAGE_SIZE = 4096;
static constexpr int NUM_BUFFER_PAGES = 65536; // 2^16

class PageID
{
public:
    int fd;
    int page_number;

    PageID() = default;

    PageID(int fd, int page_number)
    {
        this->fd = fd;
        this->page_number = page_number;
    }

    friend std::ostream &operator<<(std::ostream &out, const PageID &self)
    {
        return out << "PageId(fd=" << self.fd << ", page_number=" << self.page_number << ")";
    }

    bool operator==(const PageID &other) const{
        return this->fd==other.fd && this->page_number==other.page_number;
    }

    bool operator!=(const PageID &other)const {
        return this->fd!=other.fd || this->page_number!=other.page_number;
    }
};

namespace std {
    template <>
    struct hash<PageID> {
        size_t operator()(const PageID &pid) const noexcept { return (pid.fd << 16) | pid.page_number; }
    };
};

class Page
{
public:
    PageID id;
    uint8_t *buf;
    bool is_dirty;

    void mark_dirty()
    {
        is_dirty = true;
    }
};

#endif