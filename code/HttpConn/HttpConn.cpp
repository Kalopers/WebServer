#include "HttpConn.hpp"

bool HttpConn::isET = false;
const char *HttpConn::srcDir;// = "/home/kalo/WebServer/resources/";
std::atomic<int> HttpConn::userCount(0);

HttpConn::HttpConn() : _fd(-1), _addr({0}), _isClose(true), _iovCnt(0) {}

HttpConn::~HttpConn()
{
    Close();
}

void HttpConn::Init(int sockFd, const sockaddr_in &addr)
{
    assert(sockFd > 0);
    userCount++;
    _addr = addr;
    _fd = sockFd;
    _writeBuff.RetrieveAll();
    _readBuff.RetrieveAll();
    _isClose = false;
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", _fd, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::Close()
{
    _response.UnmapFile();
    if (!_isClose)
    {
        _isClose = true;
        userCount--;
        close(_fd);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", _fd, GetIP(), GetPort(), (int)userCount);
    }
}

int HttpConn::GetFd() const
{
    return _fd;
}

const char *HttpConn::GetIP() const
{
    return inet_ntoa(_addr.sin_addr);
}

int HttpConn::GetPort() const
{
    return ntohs(_addr.sin_port);
}

sockaddr_in HttpConn::GetAddr() const
{
    return _addr;
}

ssize_t HttpConn::Read(int *saveErrno)
{
    ssize_t len = -1;
    do
    {
        len = _readBuff.ReadFd(_fd, saveErrno);
        if (len <= 0)
        {
            break;
        }
    } while (isET);
    return len;
}

ssize_t HttpConn::Write(int *saveErrno)
{
    ssize_t len = -1;
    do
    {
        len = writev(_fd, _iov, _iovCnt);
        if (len <= 0)
        {
            *saveErrno = errno;
            break;
        }
        if (_iov[0].iov_len + _iov[1].iov_len == 0)
        {
            break;
        }
        else if (static_cast<size_t>(len) > _iov[0].iov_len)
        {
            _iov[1].iov_base = (uint8_t *)_iov[1].iov_base + (len - _iov[0].iov_len);
            _iov[1].iov_len -= (len - _iov[0].iov_len);
            if (_iov[0].iov_len)
            {
                _writeBuff.RetrieveAll();
                _iov[0].iov_len = 0;
            }
        }
        else
        {
            _iov[0].iov_base = (uint8_t *)_iov[0].iov_base + len;
            _iov[0].iov_len -= len;
            _writeBuff.Retrieve(len);
        }
    } while (isET || ToWriteBytes() > 10240);
    return len;
}

bool HttpConn::process()
{
    _request.Init();
    if (_readBuff.ReadableBytes() <= 0)
    {
        return false;
    }
    else if (_request.parse(_readBuff))
    {
        LOG_DEBUG("%s", _request.path().c_str());
        _response.Init(srcDir, _request.path(), _request.IsKeepAlive(), 200);
    }
    else
    {
        _response.Init(srcDir, _request.path(), false, 400);
    }

    _response.MakeResponse(_writeBuff);
    _iov[0].iov_base = const_cast<char *>(_writeBuff.Peek());
    _iov[0].iov_len = _writeBuff.ReadableBytes();
    _iovCnt = 1;

    if (_response.FileLen() > 0 && _response.File())
    {
        _iov[1].iov_base = _response.File();
        _iov[1].iov_len = _response.FileLen();
        _iovCnt = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", _response.FileLen(), _iovCnt, ToWriteBytes());
    return true;
}

int HttpConn::ToWriteBytes() const
{
    return _iov[0].iov_len + _iov[1].iov_len;
}

bool HttpConn::IsKeepAlive() const
{
    return _request.IsKeepAlive();
}