#include <iostream>
#include "threadpool.hpp"

// 0 for default number of threads
ThreadPool::ThreadPool(size_t num_threads) : stop(false) {
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency() > 0 ? std::thread::hardware_concurrency() : 1;
        std::cout << "ThreadPool: Using " << num_threads << " threads (hardware concurrency)." << std::endl;
    } else {
        std::cout << "ThreadPool: Using " << num_threads << " threads." << std::endl;
    }

    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back(std::bind(&ThreadPool::worker_main, this));
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(this->queue_mutex);
        stop = true;
    }
    condition.notify_all(); // Wake up all waiting threads
    for (std::thread& worker : workers) {
        worker.join(); // Wait for all threads to finish their current task and exit
    }
}

void ThreadPool::worker_main() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            // Wait until there's a task or the pool is stopping
            condition.wait(lock, [this] { return stop || !tasks.empty(); });

            if (this->stop && tasks.empty()) {
                return; // Exit thread if stopping and no more tasks
            }
            task = tasks.front();
            this->tasks.pop();
        }
        task(); // Execute the task
    }
}
