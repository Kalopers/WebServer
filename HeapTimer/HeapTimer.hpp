#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <cassert>
#include <time.h>
#include <arpa/inet.h> 
#include <algorithm>

#include "../Log/Log.hpp"

typedef std::function<void()> TimeoutCallBack;
typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::milliseconds MS;
typedef Clock::time_point TimeStamp;

struct TimerNode
{
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const TimerNode &t) const
    {
        return expires < t.expires;
    }
    bool operator>(const TimerNode &t) const
    {
        return expires > t.expires;
    }
};

class HeapTimer
{
public:
    HeapTimer();
    ~HeapTimer();

    void adjust(int id, int newExpires);
    void add(int id, int timeOut, const TimeoutCallBack &cb);
    void doWork(int id);
    void clear();
    void tick();
    void pop();
    int getNextTick();

private:
    void del(size_t index);
    void shiftUp(size_t i);
    bool shiftDown(size_t i, size_t n);
    void swapNode(size_t i, size_t j);
    void adjustNode(size_t i);

    std::vector<TimerNode> _heap;
    std::unordered_map<int, size_t> _ref; // id对应的在heap中的下标，方便用heap的时候查找
};

#endif // HEAP_TIMER_H
