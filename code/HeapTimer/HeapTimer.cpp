#include "HeapTimer.hpp"

HeapTimer::HeapTimer() {
    _heap.reserve(64);
}

HeapTimer::~HeapTimer() {
    clear();
}

void HeapTimer::swapNode(size_t i, size_t j)
{
    assert(i >= 0 && i < _heap.size());
    assert(j >= 0 && j < _heap.size());
    std::swap(_heap[i], _heap[j]);
    _ref[_heap[i].id] = i;
    _ref[_heap[j].id] = j;
}

void HeapTimer::adjustNode(size_t i)
{
    size_t parent = (i - 1) / 2;
    while (parent >= 0 && _heap[parent] > _heap[i])
    {
        swapNode(i, parent);
        i = parent;
        parent = (i - 1) / 2;
    }

    if (i > 0 && shiftDown(i, _heap.size()))
    {
        shiftUp(i);
    }
}

void HeapTimer::shiftUp(size_t i)
{
    assert(i >= 0 && i < _heap.size());
    size_t parent = (i - 1) / 2;
    while (parent >= 0)
    {
        if (_heap[parent] > _heap[i])
        {
            swapNode(i, parent);
            i = parent;
            parent = (i - 1) / 2;
        }
        else
        {
            break;
        }
    }
}

bool HeapTimer::shiftDown(size_t i, size_t n)
{
    assert(i >= 0 && i < _heap.size());
    assert(n >= 0 && n <= _heap.size());
    auto index = i;
    auto child = 2 * index + 1;
    while (child < n)
    {
        if (child + 1 < n && _heap[child + 1] < _heap[child])
        {
            child++;
        }
        if (_heap[child] < _heap[index])
        {
            swapNode(index, child);
            index = child;
            child = 2 * index + 1;
        }
        else
        {
            break;
        }
    }
    return index > i;
}

void HeapTimer::del(size_t index)
{
    assert(index >= 0 && index < _heap.size());
    size_t tmp = index;
    size_t n = _heap.size() - 1;
    if (index < n)
    {
        swapNode(tmp, n);
        if (!shiftDown(tmp, n))
        {
            shiftUp(tmp);
        }
        // adjustNode(tmp);
    }
    _ref.erase(_heap.back().id);
    _heap.pop_back();
}

void HeapTimer::adjust(int id, int newExpires)
{
    assert(!_heap.empty() && _ref.count(id));
    auto &node = _heap[_ref[id]];
    node.expires = Clock::now() + MS(newExpires);
    if (!shiftDown(_ref[id], _heap.size()))
    {
        shiftUp(_ref[id]);
    }
}

void HeapTimer::add(int id, int timeOut, const TimeoutCallBack &cb)
{
    assert(id >= 0);
    auto it = _ref.find(id);
    if (it != _ref.end())
    {
        auto &node = _heap[it->second];
        node.expires = Clock::now() + MS(timeOut);
        node.cb = cb;
        if (!shiftDown(it->second, _heap.size()))
        {
            shiftUp(it->second);
        }
    }
    else
    {
        size_t n = _heap.size();
        _ref.emplace(id, n);
        _heap.push_back({id, Clock::now() + MS(timeOut), cb});
        shiftUp(n);
    }
}

void HeapTimer::doWork(int id)
{
    auto it = _ref.find(id);
    if (_heap.empty() || it == _ref.end())
    {
        LOG_ERROR("Timer id not found in heap.");
        return;
    }

    auto &node = _heap[it->second];
    LOG_INFO("Id %d being called.", node.id);
    node.cb();
    del(it->second);
}

void HeapTimer::tick()
{
    if (_heap.empty())
    {
        return;
    }
    while (!_heap.empty())
    {
        auto now = Clock::now();
        TimerNode node = _heap.front();
        if (std::chrono::duration_cast<MS>(node.expires - now).count() > 0)
        {
            break;
        }
        // if(node.expires > now)
        // {
        //     break;
        // }
        LOG_INFO("Timer expired, id: %d.", node.id);
        node.cb();
        pop();
    }
}

void HeapTimer::pop()
{
    assert(!_heap.empty());
    del(0);
}

void HeapTimer::clear()
{
    _ref.clear();
    _heap.clear();
}

int HeapTimer::getNextTick()
{
    tick();
    if (!_heap.empty())
    {
        auto duration = _heap.front().expires - Clock::now();
        return duration.count() > 0 ? duration.count() : 0;
    }
    return -1;
}
