#ifndef LOG_HPP
#define LOG_HPP

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/stat.h>
#include "BlockQueue.hpp"
#include "../Buffer/Buffer.hpp"

class Log final
{
private:
    Log();
    virtual ~Log();
    void AppendLogLevelTitle_(int level);
    void AsyncWrite_();

private:
    static const int LOG_PATH_LEN = 256;
    static const int LOG_NAME_LEN = 256;
    static const int MAX_LINES = 50000;

    const char *_path;
    const char *_suffix;

    int _max_lines;

    int _line_count;
    int _today;

    bool _is_open;

    Buffer _buffer;
    int _level;
    bool _is_async;

    FILE *_fp;
    std::unique_ptr<BlockQueue<std::string>> _log_queue;
    std::unique_ptr<std::thread> _write_thread;
    std::mutex _mtx;

public:
    static Log *Instance();
    static void FlushLogThread();

    void Init(int level,
              const char *path = "./log",
              const char *suffix = ".log",
              int max_queue_capacity = 1024);
    void Write(int level, const char *format, ...);
    void Flush();

    int GetLevel();
    void SetLevel(int level);

    bool IsOpen();
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::Instance();\
        if (log->IsOpen() && log->GetLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);    
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);


#endif // LOG_HPP