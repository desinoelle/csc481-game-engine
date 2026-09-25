#include "niceJobSystem.hpp"
#include <cstddef>

/**
 * Worker thread for processing jobs.
 * Each worker atomically fetches the next job index and executes the job.
 * No locks needed - atomic operations ensure thread safety.
 */
void worker(SharedData& data, const JobQueue& jobs) {
    while (true) {
        // Atomically fetch and increment the next job index
        size_t jobIndex = data.nextJobIndex.fetch_add(1);
        
        // If we've gone past the job queue size, exit
        if (jobIndex >= jobs.size()) {
            break;
        }
        
        // Execute the job
        jobs[jobIndex]();
    }
}