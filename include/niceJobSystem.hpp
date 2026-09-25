#pragma once
#include <functional>
#include <vector>
#include "niceSharedData.hpp"

// Job is a std::function that takes no arguments and returns void
using Job = std::function<void()>;

// JobQueue is a vector of jobs
using JobQueue = std::vector<Job>;

/**
 * Worker thread function - processes jobs from the queue.
 * Uses atomic fetch_add for lock-free job distribution.
 */
void worker(SharedData& data, const JobQueue& jobs);