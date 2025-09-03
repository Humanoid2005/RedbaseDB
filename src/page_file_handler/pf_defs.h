#ifndef PF_DEFS_H
#define PF_DEFS_H

#include "../db_defs.h"
#include <cinttypes>
#include <cstdlib>

/*
static --> to maintain internal linkage and not being misused by another file with same variable name
constexpr --> to tell that value is available at compile time
This is better than #define since this ensures type safety and is backed by the advantages of static,constexpr
*/
static constexpr int PAGE_SIZE = 4096;
static constexpr int NUM_CACHE_PAGES = 65536;//2^16

struct PageID {
    int fd;
    int page_no;

    PageID() = default;
    PageID(int fd_, int page_no_) : fd(fd_), page_no(page_no_) {}

    friend bool operator==(const PageID &x, const PageID &y) { return x.fd == y.fd && x.page_no == y.page_no; }
    friend bool operator!=(const PageID &x, const PageID &y) { return !(x == y); }

    friend std::ostream &operator<<(std::ostream &os, const PageID &self) {
        return os << "PageID(fd=" << self.fd << ", page_no=" << self.page_no << ")";
    }
};

namespace std {
template <>
struct hash<PageID> {
    size_t operator()(const PageID &pid) const noexcept { return (pid.fd << 16) | pid.page_no; }
};
} // namespace std

struct Page {
    PageID id;
    uint8_t *buf;
    bool is_dirty;

    void mark_dirty() { is_dirty = true; }
};

#endif
