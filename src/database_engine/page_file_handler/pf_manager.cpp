#include "pf_manager.h"
#include "database_engine/concurrency.h"

std::unordered_map<std::string, int> PF_Manager::_path2fd;
std::unordered_map<int, std::string> PF_Manager::_fd2path;
PF_Pager PF_Manager::pager;

bool PF_Manager::is_file(const std::string &path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

void PF_Manager::create_file(const std::string &path) {
    sem_wait(fd_map_sem);  // SEMAPHORE: Protect fd maps
    
    if (is_file(path)) {
        sem_post(fd_map_sem);
        throw FileExistsError(path);
    }
    int fd = open(path.c_str(), O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        sem_post(fd_map_sem);
        throw UnixError();
    }
    if (close(fd) != 0) {
        sem_post(fd_map_sem);
        throw UnixError();
    }
    
    sem_post(fd_map_sem);  // SEMAPHORE: Release
}

void PF_Manager::destroy_file(const std::string &path) {
    sem_wait(fd_map_sem);  // SEMAPHORE: Protect fd maps
    
    if (!is_file(path)) {
        sem_post(fd_map_sem);
        throw FileNotFoundError(path);
    }
    // If file is open, cannot destroy file
    if (_path2fd.count(path)) {
        sem_post(fd_map_sem);
        throw FileNotClosedError(path);
    }
    // Remove file from disk
    if (unlink(path.c_str()) != 0) {
        sem_post(fd_map_sem);
        throw UnixError();
    }
    
    sem_post(fd_map_sem);  // SEMAPHORE: Release
}

int PF_Manager::open_file(const std::string &path) {
    sem_wait(fd_map_sem);  // SEMAPHORE: Protect fd maps
    
    if (!is_file(path)) {
        sem_post(fd_map_sem);
        throw FileNotFoundError(path);
    }
    if (_path2fd.count(path)) {
        // File is already open
        sem_post(fd_map_sem);
        throw FileNotClosedError(path);
    }
    // Open file and return the file descriptor
    int fd = open(path.c_str(), O_RDWR);
    if (fd < 0) {
        sem_post(fd_map_sem);
        throw UnixError();
    }
    // Memorize the opened unix file descriptor
    _path2fd[path] = fd;
    _fd2path[fd] = path;
    
    sem_post(fd_map_sem);  // SEMAPHORE: Release
    return fd;
}

void PF_Manager::close_file(int fd) {
    sem_wait(fd_map_sem);  // SEMAPHORE: Protect fd maps
    
    auto pos = _fd2path.find(fd);
    if (pos == _fd2path.end()) {
        sem_post(fd_map_sem);
        throw FileNotOpenError(fd);
    }
    pager.flush_file(fd);
    const std::string &filename = pos->second;
    _path2fd.erase(filename);
    _fd2path.erase(pos);
    if (close(fd) != 0) {
        sem_post(fd_map_sem);
        throw UnixError();
    }
    
    sem_post(fd_map_sem);  // SEMAPHORE: Release
}
