#ifndef SQLCONNPOOL_HPP
#define SQLCONNPOOL_HPP

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../Log/Log.hpp"

class SqlConnPool
{
private:
    SqlConnPool() = default;
    ~SqlConnPool();

    int _MAX_CONN;

    std::queue<MYSQL *> _connQue;
    std::mutex _mtx;
    sem_t _semId;

public:
    static SqlConnPool *Instance();

    MYSQL *GetConn();
    void FreeConn(MYSQL *conn);
    int GetFreeConnCount();

    void Init(const char *host, int port,
              const char *user, const char *pwd,
              const char *dbName, int connSize);
    void ClosePool();
};

class SqlConnRAII
{
private:
    MYSQL *_sql;
    SqlConnPool *_connPool;

public:
    SqlConnRAII(MYSQL **sql, SqlConnPool *connpool);
    ~SqlConnRAII();
};

#endif // SQLCONNPOOL_HPP