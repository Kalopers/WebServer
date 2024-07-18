#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <queue>
#include <condition_variable>
#include <mutex>
#include <functional>
#include <thread>
#include <assert.h>

class ThreadPool
{
private:
    struct Pool
    {
        std::mutex _mtx;
        std::condition_variable _cond;
        bool _is_close;
        std::queue<std::function<void()>> _tasks;
    };
    std::shared_ptr<Pool> _pool;

public:
    ThreadPool();
    ThreadPool(ThreadPool &&) = default;

    explicit ThreadPool(size_t num_threads);
    
    ~ThreadPool();

    template <typename T>
    void AddTask(T&& task);
};

#endif // THREADPOOL_HPP