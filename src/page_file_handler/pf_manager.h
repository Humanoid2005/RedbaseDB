#ifndef PF_MANAGER_H
#define PF_MANAGER_H

#include <algorithm>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
#include "pf_pager.h"

class PF_Manager
{
public:
    static PF_Pager pager;

    static bool is_file(const std::string &path);

    static void create_file(const std::string &path);

    static void destroy_file(const std::string &path);

    static int open_file(const std::string &path);

    static void close_file(int fd);

private:
    static std::unordered_map<std::string, int> path2fd;
    static std::unordered_map<int, std::string> fd2path;
};

#endif