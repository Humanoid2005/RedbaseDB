#include "concurrency.h"
#include <iostream>

// Define the global semaphores
sem_t *cache_sem = nullptr;
sem_t *fd_map_sem = nullptr;
sem_t *metadata_sem = nullptr;
sem_t *file_handles_sem = nullptr;
sem_t *index_handles_sem = nullptr;

void setup_db_semaphores() {
    // Create named semaphores with initial value 1 (binary semaphore)
    cache_sem = sem_open("/db_cache_sem", O_CREAT, 0644, 1);
    if (cache_sem == SEM_FAILED) {
        std::cerr << "Failed to create cache_sem" << std::endl;
        exit(1);
    }
    
    fd_map_sem = sem_open("/db_fd_map_sem", O_CREAT, 0644, 1);
    if (fd_map_sem == SEM_FAILED) {
        std::cerr << "Failed to create fd_map_sem" << std::endl;
        exit(1);
    }
    
    metadata_sem = sem_open("/db_metadata_sem", O_CREAT, 0644, 1);
    if (metadata_sem == SEM_FAILED) {
        std::cerr << "Failed to create metadata_sem" << std::endl;
        exit(1);
    }
    
    file_handles_sem = sem_open("/db_fh_sem", O_CREAT, 0644, 1);
    if (file_handles_sem == SEM_FAILED) {
        std::cerr << "Failed to create file_handles_sem" << std::endl;
        exit(1);
    }
    
    index_handles_sem = sem_open("/db_ih_sem", O_CREAT, 0644, 1);
    if (index_handles_sem == SEM_FAILED) {
        std::cerr << "Failed to create index_handles_sem" << std::endl;
        exit(1);
    }
    
    std::cout << "Database semaphores initialized successfully" << std::endl;
}

void cleanup_db_semaphores() {
    if (cache_sem) {
        sem_close(cache_sem);
        sem_unlink("/db_cache_sem");
    }
    
    if (fd_map_sem) {
        sem_close(fd_map_sem);
        sem_unlink("/db_fd_map_sem");
    }
    
    if (metadata_sem) {
        sem_close(metadata_sem);
        sem_unlink("/db_metadata_sem");
    }
    
    if (file_handles_sem) {
        sem_close(file_handles_sem);
        sem_unlink("/db_fh_sem");
    }
    
    if (index_handles_sem) {
        sem_close(index_handles_sem);
        sem_unlink("/db_ih_sem");
    }
    
    std::cout << "Database semaphores cleaned up" << std::endl;
}
