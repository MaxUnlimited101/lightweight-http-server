#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <type_traits>

class ThreadPool
{
public:
    ThreadPool(size_t num_threads);
    ~ThreadPool();

    // Enqueue a task to be executed by a worker thread
    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using return_type = typename std::invoke_result<F, Args...>::type;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop)
            {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            this->tasks.emplace([task]()
                                { (*task)(); });
        }
        condition.notify_one(); // Wake up one waiting thread
        return res;
    }

private:
    // Need to keep track of threads so we can join them
    std::vector<std::thread> workers;

    // The task queue
    std::queue<std::function<void()>> tasks;

    // Synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

    // Worker function for each thread
    void worker_main();
};

#endif // THREAD_POOL_H