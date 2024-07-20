#ifndef HTTPCONN_HPP
#define HTTPCONN_HPP

#include <sys/types.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>

#include "../Log/Log.hpp"
#include "../Buffer/Buffer.hpp"
#include "../HttpRequest/HttpRequest.hpp"
#include "../HttpResponse/HttpResponse.hpp"

class HttpConn
{
private:
    int _fd;
    struct sockaddr_in _addr;

    bool _isClose;

    int _iovCnt;
    struct iovec _iov[2];

    Buffer _readBuff;
    Buffer _writeBuff;

    HttpRequest _request;
    HttpResponse _response;

public:
    HttpConn();
    ~HttpConn();

    void Init(int sockFd, const sockaddr_in &addr);
    ssize_t Read(int *saveErrno);
    ssize_t Write(int *saveErrno);
    void Close();
    int GetFd() const;
    int GetPort() const;
    const char *GetIP() const;
    sockaddr_in GetAddr() const;
    bool process();

    int ToWriteBytes() const;
    bool IsKeepAlive() const;

    static bool isET;
    static const char *srcDir;
    static std::atomic<int> userCount;
};

#endif // HTTPCONN_HPP