#include "Log.hpp"

Log::Log()
    : _line_count(0),
      _today(0),
      _is_open(false),
      _level(1),
      _is_async(false),
      _fp(nullptr),
      _log_queue(nullptr),
      _write_thread(nullptr)
{
}

Log::~Log()
{
    while (!_log_queue->empty())
    {
        _log_queue->flush();
    }
    _log_queue->Close();
    _write_thread->join();
    if (_fp)
    {
        std::unique_lock<std::mutex> locker(_mtx);
        Flush();
        fclose(_fp);
    }
}

void Log::Flush()
{
    if (_is_async)
    {
        _log_queue->flush();
    }
    fflush(_fp);
}

void Log::FlushLogThread()
{
    Log::Instance()->AsyncWrite_();
}

Log *Log::Instance()
{
    static Log instance;
    return &instance;
}

void Log::AsyncWrite_()
{
    std::string str = "";
    while (_log_queue->pop(str))
    {
        std::unique_lock<std::mutex> locker(_mtx);
        fputs(str.c_str(), _fp);
    }
}

void Log::Init(int level, const char *path, const char *suffix, int max_queue_capacity)
{
    _is_open = true;
    _level = level;
    _path = path;
    _suffix = suffix;

    if (max_queue_capacity > 0)
    {
        _is_async = true;
        if (!_log_queue)
        {
            std::unique_ptr<BlockQueue<std::string>> new_log_queue(new BlockQueue<std::string>);
            _log_queue = std::move(new_log_queue);

            std::unique_ptr<std::thread> new_write_thread(new std::thread(AsyncWrite_));
            _write_thread = std::move(new_write_thread);
        }
    }
    else
    {
        _is_async = false;
    }

    _line_count = 0;
    time_t timer = time(nullptr);
    struct tm *sys_tm = localtime(&timer);
    char file_name[LOG_NAME_LEN] = {0};
    snprintf(file_name, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s",
             _path, sys_tm->tm_year + 1900, sys_tm->tm_mon + 1, sys_tm->tm_mday, _suffix);
    _today = sys_tm->tm_mday;

    {
        std::lock_guard<std::mutex> locker(_mtx);
        _buffer.RetrieveAll();
        if (_fp)
        {
            Flush();
            fclose(_fp);
        }
        _fp = fopen(file_name, "a");
        if (_fp == nullptr)
        {
            mkdir(_path, 0777);
            _fp = fopen(file_name, "a");
        }
    }
}

void Log::Write(int level, const char *format, ...)
{
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t t_sec = now.tv_sec;
    struct tm *sys_tm = localtime(&t_sec);
    struct tm t = *sys_tm;
    va_list vaList;

    if (_today != t.tm_mday || (_line_count && (_line_count % MAX_LINES == 0)))
    {
        std::unique_lock<std::mutex> locker(_mtx);
        locker.unlock();

        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (_today != t.tm_mday)
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", _path, tail, _suffix);
            _today = t.tm_mday;
            _line_count = 0;
        }
        else
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", _path, tail, (_line_count / MAX_LINES), _suffix);
        }

        locker.lock();
        Flush();
        fclose(_fp);
        _fp = fopen(newFile, "a");
        assert(_fp != nullptr);
    }

    {
        std::unique_lock<std::mutex> locker(_mtx);
        _line_count++;
        // Time
        int n = snprintf(_buffer.BeginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                         t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                         t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
        _buffer.HasWritten(n);
        // Level
        AppendLogLevelTitle_(level);
        // Content
        va_start(vaList, format);
        int m = vsnprintf(_buffer.BeginWrite(), _buffer.WritableBytes(), format, vaList);
        va_end(vaList);

        _buffer.HasWritten(m);
        _buffer.Append("\n\0", 2);

        if (_is_async && _log_queue && !_log_queue->full())
        {
            _log_queue->push_back(_buffer.RetrieveAllToStr());
        }
        else
        {
            fputs(_buffer.Peek(), _fp);
        }
        _buffer.RetrieveAll();
    }
}

void Log::AppendLogLevelTitle_(int level)
{
    switch (level)
    {
    case 0:
        _buffer.Append("[debug]: ", 9);
        break;
    case 1:
        _buffer.Append("[info] : ", 9);
        break;
    case 2:
        _buffer.Append("[warn] : ", 9);
        break;
    case 3:
        _buffer.Append("[error]: ", 9);
        break;
    default:
        _buffer.Append("[info] : ", 9);
        break;
    }
}

int Log::GetLevel()
{
    std::lock_guard<std::mutex> locker(_mtx);
    return _level;
}

void Log::SetLevel(int level)
{
    std::lock_guard<std::mutex> locker(_mtx);
    _level = level;
}