#include "BlockQueue.hpp"

template <typename T>
BlockQueue<T>::BlockQueue(size_t maxsize)
	: _capacity(maxsize),
	  _isClose(false)
{
	assert(maxsize > 0);
}

template <typename T>
BlockQueue<T>::~BlockQueue()
{
	Close();
}

template <typename T>
void BlockQueue<T>::Close()
{
	clear();
	_isClose = true;
	_condProducer.notify_all();
	_condCustomer.notify_all();
}

template <typename T>
void BlockQueue<T>::clear()
{
	std::lock_guard<std::mutex> locker(_mtx);
	_queue.clear();
}

template <typename T>
bool BlockQueue<T>::empty()
{
	std::lock_guard<std::mutex> locker(_mtx);
	return _queue.empty();
}

template <typename T>
bool BlockQueue<T>::full()
{
	std::lock_guard<std::mutex> locker(_mtx);
	return _queue.size() >= _capacity;
}

template <typename T>
void BlockQueue<T>::push_back(const T &item)
{
	std::unique_lock<std::mutex> locker(_mtx);
	while (_queue.size() >= _capacity)
	{
		_condProducer.wait(locker);
	}
	_queue.push_back(item);
	_condCustomer.notify_one();
}

template <typename T>
void BlockQueue<T>::push_front(const T &item)
{
	std::unique_lock<std::mutex> locker(_mtx);
	while (_queue.size() >= _capacity)
	{
		_condProducer.wait(locker);
	}
	_queue.push_front(item);
	_condCustomer.notify_one();
}

template <typename T>
bool BlockQueue<T>::pop(T &item)
{
	std::unique_lock<std::mutex> locker(_mtx);
	while (_queue.empty())
	{
		_condCustomer.wait(locker);
		if (_isClose)
		{
			return false;
		}
	}
	item = _queue.front();
	_queue.pop_front();
	_condProducer.notify_one();
	return true;
}

template <typename T>
bool BlockQueue<T>::pop(T &item, int timeout)
{
	std::unique_lock<std::mutex> locker(_mtx);
	while (_queue.empty())
	{
		if (_condCustomer.wait_for(locker, std::chrono::seconds(timeout)) == std::cv_status::timeout)
		{
			return false;
		}	
		if (_isClose)
		{
			return false;
		}
	}
	item = _queue.front();
	_queue.pop_front();
	_condProducer.notify_one();
	return true;
}

template <typename T>
T BlockQueue<T>::front()
{
	std::lock_guard<std::mutex> locker(_mtx);
	return _queue.front();
}

template <typename T>
T BlockQueue<T>::back()
{
	std::lock_guard<std::mutex> locker(_mtx);
	return _queue.back();
}

template <typename T>
size_t BlockQueue<T>::capacity()
{
	std::lock_guard<std::mutex> locker(_mtx);
	return _capacity;
}

template <typename T>
size_t BlockQueue<T>::size()
{
	std::lock_guard<std::mutex> locker(_mtx);
	return _queue.size();
}

template <typename T>
void BlockQueue<T>::flush()
{
	_condCustomer.notify_one();
}
