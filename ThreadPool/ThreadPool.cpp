#include "ThreadPool.hpp"

ThreadPool::ThreadPool(size_t num_threads) : _pool(std::make_shared<Pool>())
{
    assert(num_threads > 0);
    for (int i = 0; i < num_threads; ++i)
    {
        std::thread([this]()
                    {
            std::unique_lock<std::mutex> locker(_pool->_mtx);
            while(true)
            {
                if (!_pool->_tasks.empty())
                {
                    auto task = std::move(_pool->_tasks.front());
                    _pool->_tasks.pop();
                    locker.unlock();
                    task();
                    locker.lock();
                }
                else if (_pool->_is_close)
                {
                    break;
                }
                else
                {
                    _pool->_cond.wait(locker);
                }
            } })
            .detach();
    }
}

ThreadPool::~ThreadPool()
{
    if (_pool)
    {
        std::unique_lock<std::mutex> locker(_pool->_mtx);
        _pool->_is_close = true;
    }
    _pool->_cond.notify_all();
}
