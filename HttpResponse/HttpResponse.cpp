#include "HttpResponse.hpp"

const std::unordered_map<std::string, std::string> HttpResponse::_SUFFIX_TYPE = {
    {".html", "text/html"},
    {".xml", "text/xml"},
    {".xhtml", "application/xhtml+xml"},
    {".txt", "text/plain"},
    {".rtf", "application/rtf"},
    {".pdf", "application/pdf"},
    {".word", "application/nsword"},
    {".png", "image/png"},
    {".gif", "image/gif"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".au", "audio/basic"},
    {".mpeg", "video/mpeg"},
    {".mpg", "video/mpeg"},
    {".avi", "video/x-msvideo"},
    {".gz", "application/x-gzip"},
    {".tar", "application/x-tar"},
    {".css", "text/css "},
    {".js", "text/javascript "},
};

const std::unordered_map<int, std::string> HttpResponse::_CODE_STATUS = {
    {200, "OK"},
    {400, "Bad Request"},
    {403, "Forbidden"},
    {404, "Not Found"},
};

const std::unordered_map<int, std::string> HttpResponse::_CODE_PATH = {
    {400, "/400.html"},
    {403, "/403.html"},
    {404, "/404.html"},
};

HttpResponse::HttpResponse()
{
    _code = -1;
    _path = _srcDir = "";
    _isKeepAlive = false;
    _mmFile = nullptr;
    _mmFileStat = {0};
};

HttpResponse::~HttpResponse()
{
    UnmapFile();
}

void HttpResponse::Init(const std::string &srcDir, std::string &path, bool isKeepAlive, int code)
{
    assert(srcDir != "");
    if (_mmFile)
    {
        UnmapFile();
    }
    _code = code;
    _isKeepAlive = isKeepAlive;
    _path = path;
    _srcDir = srcDir;
    _mmFile = nullptr;
    _mmFileStat = {0};
}

void HttpResponse::MakeResponse(Buffer &buff)
{
    if (stat((_srcDir + _path).data(), &_mmFileStat) < 0 || S_ISDIR(_mmFileStat.st_mode))
    {
        _code = 404;
    }
    else if (!(_mmFileStat.st_mode & S_IROTH))
    {
        _code = 403;
    }
    else if (_code == -1)
    {
        _code = 200;
    }
    ErrorHtml();
    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}

char *HttpResponse::File()
{
    return _mmFile;
}

size_t HttpResponse::FileLen() const
{
    return _mmFileStat.st_size;
}

void HttpResponse::ErrorHtml()
{
    if (_CODE_PATH.count(_code) == 1)
    {
        _path = _CODE_PATH.find(_code)->second;
        stat((_srcDir + _path).data(), &_mmFileStat);
    }
}

void HttpResponse::AddStateLine_(Buffer &buff)
{
    std::string status;
    if (_CODE_STATUS.count(_code) == 1)
    {
        status = _CODE_STATUS.find(_code)->second;
    }
    else
    {
        _code = 400;
        status = _CODE_STATUS.find(400)->second;
    }
    buff.Append("HTTP/1.1 " + std::to_string(_code) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_(Buffer &buff)
{
    buff.Append("Connection: ");
    if (_isKeepAlive)
    {
        buff.Append("keep-alive\r\n");
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    }
    else
    {
        buff.Append("close\r\n");
    }
    buff.Append("Content-type: " + GetFileType() + "\r\n");
}

void HttpResponse::AddContent_(Buffer &buff)
{
    int srcFd = open((_srcDir + _path).data(), O_RDONLY);
    if (srcFd < 0)
    {
        ErrorContent(buff, "File NotFound!");
        return;
    }

    LOG_DEBUG("file path %s", (_srcDir + _path).data());
    int *mmRet = (int *)mmap(0, _mmFileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if (*mmRet == -1)
    {
        ErrorContent(buff, "File NotFound!");
        return;
    }
    _mmFile = (char *)mmRet;
    close(srcFd);
    buff.Append("Content-length: " + std::to_string(_mmFileStat.st_size) + "\r\n\r\n");
}

void HttpResponse::UnmapFile()
{
    if (_mmFile)
    {
        munmap(_mmFile, _mmFileStat.st_size);
        _mmFile = nullptr;
    }
}

std::string HttpResponse::GetFileType()
{
    std::string::size_type idx = _path.find_last_of('.');
    if (idx == std::string::npos)
    {
        return "text/plain";
    }
    std::string suffix = _path.substr(idx);
    if (_SUFFIX_TYPE.count(suffix) == 1)
    {
        return _SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer &buff, std::string message)
{
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if (_CODE_STATUS.count(_code) == 1)
    {
        status = _CODE_STATUS.find(_code)->second;
    }
    else
    {
        status = "Bad Request";
    }
    body += std::to_string(_code) + " : " + status + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.Append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}