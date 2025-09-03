#include "pf_manager.h"

std::unordered_map<std::string, int> PF_Manager::path2fd;
std::unordered_map<int, std::string> PF_Manager::fd2path;
PF_Pager PF_Manager::pager;


/*
Function: is_file
Description:   
    This function checks if a given path corresponds to a regular file.
    It uses the stat system call to retrieve information about the file
    and checks if the file type is regular.
*/
bool PF_Manager::is_file(const std::string &path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}


/*
Function: create_file
Description:   
    This function creates a new file at the specified path. If the file
    already exists, it throws a FileExistsError. It uses the open system
    call to create the file with read and write permissions for the user.
*/
void PF_Manager::create_file(const std::string &path) {
    if (is_file(path)) {
        throw FileExistsError(path);
    }
    int fd = open(path.c_str(), O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        throw UnixError();
    }
    if (close(fd) != 0) {
        throw UnixError();
    }
}

/*
Function: destroy_file
Description:   
    This function destroys a file at the specified path. If the file
    does not exist, it throws a FileNotFoundError. If the file is open,
    it throws a FileNotClosedError. It uses the unlink system call to
    remove the file from disk.
*/
void PF_Manager::destroy_file(const std::string &path) {
    if (!is_file(path)) {
        throw FileNotFoundError(path);
    }
    // If file is open, cannot destroy file
    if (path2fd.count(path)) {
        throw FileNotClosedError(path);
    }
    // Remove file from disk
    if (unlink(path.c_str()) != 0) {
        throw UnixError();
    }
}

/*
Function: open_file
Description:   
    This function opens a file at the specified path. If the file does
    not exist, it throws a FileNotFoundError. If the file is already
    open, it throws a FileNotClosedError. It uses the open system call
    to open the file with read and write permissions for the user.
    It returns the file descriptor for the opened file.
*/
int PF_Manager::open_file(const std::string &path) {
    if (!is_file(path)) {
        throw FileNotFoundError(path);
    }
    if (path2fd.count(path)) {
        // File is already open
        throw FileNotClosedError(path);
    }
    // Open file and return the file descriptor
    int fd = open(path.c_str(), O_RDWR);
    if (fd < 0) {
        throw UnixError();
    }
    // Memorize the opened unix file descriptor
    path2fd[path] = fd;
    fd2path[fd] = path;
    return fd;
}

/*
Function: close_file
Description:   
    This function closes a file associated with the given file descriptor.
    If the file descriptor is not valid, it throws a FileNotOpenError.
    It flushes the file to ensure all changes are written to disk before
    closing it. It uses the close system call to close the file descriptor.
*/
void PF_Manager::close_file(int fd) {
    auto pos = fd2path.find(fd);
    if (pos == fd2path.end()) {
        throw FileNotOpenError(fd);
    }
    pager.flush_file(fd);
    const std::string &filename = pos->second;
    path2fd.erase(filename);
    fd2path.erase(pos);
    if (close(fd) != 0) {
        throw UnixError();
    }
}