#include "SQLconnPool.hpp"

SqlConnPool::~SqlConnPool()
{
    ClosePool();
}

SqlConnPool *SqlConnPool::Instance()
{
    static SqlConnPool pool;
    return &pool;
}

void SqlConnPool::Init(const char *host, int port, const char *user, const char *pwd, const char *dbName, int connSize)
{
    assert(connSize > 0);
    for (int i = 0; i < connSize; ++i)
    {
        MYSQL *conn = nullptr;
        conn = mysql_init(conn);
        if (!conn)
        {
            LOG_ERROR("MySQL Init Error!");
            assert(conn);
        }
        conn = mysql_real_connect(conn, host, user, pwd, dbName, port, nullptr, 0);
        if (!conn)
        {
            LOG_ERROR("MySQL Conn Error!")
        }
        _connQue.emplace(conn);
    }
    _MAX_CONN = connSize;
    sem_init(&_semId, 0, _MAX_CONN);
}

MYSQL *SqlConnPool::GetConn()
{
    MYSQL *conn = nullptr;
    if (_connQue.empty())
    {
        LOG_WARN("SQLConnPool Busy!");
        return nullptr;
    }
    sem_wait(&_semId);
    std::lock_guard<std::mutex> locker(_mtx);
    conn = _connQue.front();
    _connQue.pop();
    return conn;
}

void SqlConnPool::FreeConn(MYSQL *conn)
{
    assert(conn);
    std::lock_guard<std::mutex> locker(_mtx);
    _connQue.push(conn);
    sem_post(&_semId);
}

void SqlConnPool::ClosePool()
{
    std::lock_guard<std::mutex> locker(_mtx);
    while (!_connQue.empty())
    {
        auto conn = _connQue.front();
        _connQue.pop();
        mysql_close(conn);
    }
    mysql_library_end();
}

int SqlConnPool::GetFreeConnCount()
{
    std::lock_guard<std::mutex> locker(_mtx);
    return _connQue.size();
}

SqlConnRAII::SqlConnRAII(MYSQL **sql, SqlConnPool *connpool)
{
    assert(connpool);
    *sql = connpool->GetConn();
    _sql = *sql;
    _connPool = connpool;
}

SqlConnRAII::~SqlConnRAII()
{
    if (_sql)
    {
        _connPool->FreeConn(_sql);
    }
}