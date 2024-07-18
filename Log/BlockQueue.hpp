#ifndef BLOCKQUEUE_HPP
#define BLOCKQUEUE_HPP

#include <deque>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <sys/time.h>

template <typename T>
class BlockQueue
{
private:
    std::deque<T> _queue;
    size_t _capacity;
    std::mutex _mtx;
    std::condition_variable _condCustomer;
    std::condition_variable _condProducer;
    std::atomic<bool> _isClose;

public:
    explicit BlockQueue(size_t maxsize = 1000);
    ~BlockQueue();
    bool empty();
    bool full();
    void push_back(const T &item);
    void push_front(const T &item);
    bool pop(T &item);              
    bool pop(T &item, int timeout); 
    void clear();
    T front();
    T back();
    size_t capacity();
    size_t size();

    void flush();
    void Close();
};

#endif // BLOCKQUEUE_HPP