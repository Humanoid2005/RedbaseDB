#ifndef CONCURRENCY_H
#define CONCURRENCY_H

#include <semaphore.h>
#include <fcntl.h>

// Global semaphores for database engine
extern sem_t *cache_sem;          // Buffer pool protection
extern sem_t *fd_map_sem;         // File descriptor maps
extern sem_t *metadata_sem;       // Database metadata
extern sem_t *file_handles_sem;   // RM file handles map
extern sem_t *index_handles_sem;  // Index handles map

// Initialize all semaphores
void setup_db_semaphores();

// Cleanup all semaphores
void cleanup_db_semaphores();

#endif // CONCURRENCY_H
