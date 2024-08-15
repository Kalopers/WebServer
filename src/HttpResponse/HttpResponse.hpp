#ifndef HTTPRESPONSE_HPP
#define HTTPRESPONSE_HPP

#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "../Buffer/Buffer.hpp"
#include "../Log/Log.hpp"

class HttpResponse
{
public:
    HttpResponse();
    ~HttpResponse();

    void Init(const std::string &srcDir, std::string &path, bool isKeepAlive = false, int code = -1);
    void MakeResponse(Buffer &buff);
    void UnmapFile();
    char *File();
    // std::unique_ptr<char[]> File();
    size_t FileLen() const;
    void ErrorContent(Buffer &buff, std::string message);
    int Code() const;

private:
    void AddStateLine_(Buffer &buff);
    void AddHeader_(Buffer &buff);
    void AddContent_(Buffer &buff);
    // std::unique_ptr<char[]> MapFile(const std::string &path, size_t &fileSize, Buffer &buff);

    void ErrorHtml();
    std::string GetFileType();

    int _code;
    bool _isKeepAlive;

    std::string _path;
    std::string _srcDir;

    char *_mmFile;
    // std::unique_ptr<char[]> _mmFile;
    struct stat _mmFileStat;

    static const std::unordered_map<std::string, std::string> _SUFFIX_TYPE;
    static const std::unordered_map<int, std::string> _CODE_STATUS;
    static const std::unordered_map<int, std::string> _CODE_PATH;
};

#endif // HTTPRESPONSE_HPP