#include "Buffer.hpp"

Buffer::Buffer(size_t init_size)
    : _buffer(init_size),
      _read_pos(0),
      _write_pos(0)
{
    assert(ReadableBytes() == 0);
    assert(WritableBytes() == init_size);
    assert(PrependableBytes() == 0);
}

size_t Buffer::WritableBytes() const
{
    return _buffer.size() - _write_pos;
}

size_t Buffer::ReadableBytes() const
{
    return _write_pos - _read_pos;
}

size_t Buffer::PrependableBytes() const
{
    return _read_pos;
}

const char *Buffer::Peek() const
{
    return &_buffer[_read_pos];
}

void Buffer::EnsureWriteable(size_t len)
{
    if (len > WritableBytes())
    {
        _MakeSpace(len);
    }
    assert(len <= WritableBytes());
}

void Buffer::HasWritten(size_t len)
{
    assert(len <= WritableBytes());
    _write_pos += len;
}

void Buffer::Retrieve(size_t len)
{
    assert(len <= ReadableBytes());
    if (len < ReadableBytes())
    {
        _read_pos += len;
    }
    else
    {
        RetrieveAll();
    }
}

void Buffer::RetrieveUntil(const char *end)
{
    assert(Peek() <= end);
    Retrieve(end - Peek());
}

void Buffer::RetrieveAll()
{
    _read_pos = 0;
    _write_pos = 0;
    bzero(&_buffer[0], _buffer.size());
}

std::string Buffer::RetrieveAllToStr()
{
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

const char *Buffer::BeginWriteConst() const
{
    return &_buffer[_write_pos];
}

char *Buffer::BeginWrite()
{
    return &_buffer[_write_pos];
}

void Buffer::Append(const char *str, size_t len)
{
    assert(str);
    EnsureWriteable(len);                    // 确保可写的长度
    std::copy(str, str + len, BeginWrite()); // 将str放到写下标开始的地方
    HasWritten(len);                         // 移动写下标
}

void Buffer::Append(const void *data, size_t len)
{
    assert(data);
    Append(static_cast<const char *>(data), len);
}

void Buffer::Append(const std::string &str)
{
    Append(str.c_str(), str.size());
}

void Buffer::Append(const Buffer &buff)
{
    Append(buff.Peek(), buff.ReadableBytes());
}

ssize_t Buffer::ReadFd(int fd, int *Errno)
{
    char buff[65535];
    struct iovec iov[2];
    const size_t writable = WritableBytes();
    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    const ssize_t len = readv(fd, iov, 2);
    if (len < 0)
    {
        *Errno = errno;
    }
    else if (static_cast<size_t>(len) <= writable)
    {
        _write_pos += len;
    }
    else
    {
        _write_pos = _buffer.size();
        Append(buff, len - writable);
    }
    return len;
}

ssize_t Buffer::WriteFd(int fd, int *Errno)
{
    size_t read_size = ReadableBytes();
    ssize_t len = write(fd, Peek(), read_size);
    if (len < 0)
    {
        *Errno = errno;
        return len;
    }
    Retrieve(len);
    return len;
}

char *Buffer::_Begin_Ptr()
{
    return &_buffer[0];
}

const char *Buffer::_Begin_Ptr() const
{
    return &_buffer[0];
}

void Buffer::_MakeSpace(size_t len)
{
    if (WritableBytes() + PrependableBytes() < len)
    {
        _buffer.resize(_write_pos + len + 1);
    }
    else
    {
        size_t readable = ReadableBytes();
        std::copy(_Begin_Ptr() + _read_pos, _Begin_Ptr() + _write_pos, _Begin_Ptr());
        _read_pos = 0;
        _write_pos = readable;
        assert(readable == ReadableBytes());
    }
}
