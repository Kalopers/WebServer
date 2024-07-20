#include "HttpRequest.hpp"

HttpRequest::HttpRequest() { Init(); }

void HttpRequest::Init()
{
    _method = _path = _version = _body = "";
    _state = REQUEST_LINE;
    _header.clear();
    _post.clear();
}

const std::unordered_set<std::string> HttpRequest::DEFAULT_HTML = {"/index", "/register", "/login", "/welcome", "/video", "/picture"};
const std::unordered_map<std::string, int> HttpRequest::DEFAULT_HTML_TAG = {{"/register.html", 0}, {"/login.html", 1}, {"/welcome.html", 2}};
const std::regex HttpRequest::request_line_pattern(R"(^([^ ]*) ([^ ]*) HTTP/([^ ]*)$)", std::regex::optimize);
const std::regex HttpRequest::header_pattern(R"(^([^:]*): ?(.*)$)", std::regex::optimize);

// bool HttpRequest::IsKeepAlive() const
// {
//     if (_header.count("Connection") == 1)
//     {
//         return _header.find("Connection")->second == "keep-alive" && _version == "1.1";
//     }
//     return false;
// }

bool HttpRequest::IsKeepAlive() const
{
    auto it = _header.find("Connection");
    return it != _header.end() && it->second == "keep-alive" && _version == "1.1";
}

bool HttpRequest::parse(Buffer &buff)
{
    const char CRLF[] = "\r\n";
    if (buff.ReadableBytes() <= 0)
    {
        return false;
    }

    while (buff.ReadableBytes() && _state != FINISH)
    {
        const char *begin = buff.Peek();
        const char *end = buff.BeginWriteConst();
        const char *lineEnd = std::search(begin, end, CRLF, CRLF + 2);
        if (lineEnd == end)
        {
            LOG_ERROR("Not a complete line");
            return false;
        }

        std::string_view line(begin, lineEnd - begin);
        switch (_state)
        {
        case REQUEST_LINE:
            if (!_ParseRequestLine(line))
            {
                return false;
            }
            _ParsePath();
            break;
        case HEADERS:
            _ParseHeader(line);
            if (buff.ReadableBytes() <= 2)
            {
                _state = FINISH;
            }
            break;
        case BODY:
            _ParseBody(line);
            break;
        default:
            break;
        }
        if (lineEnd == buff.BeginWrite())
        {
            break;
        }
        buff.RetrieveUntil(lineEnd + 2);
    }
    LOG_DEBUG("[%s], [%s], [%s]", _method.c_str(), _path.c_str(), _version.c_str());
    return true;
}

// bool HttpRequest::_ParseRequestLine(const std::string &line)
// {
//     std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
//     std::smatch subMatch;

//     if (regex_match(line, subMatch, patten))
//     {
//         _method = subMatch[1];
//         _path = subMatch[2];
//         _version = subMatch[3];
//         _state = HEADERS;
//         return true;
//     }
//     LOG_ERROR("RequestLine Error");
//     return false;
// }

bool HttpRequest::_ParseRequestLine(std::string_view line)
{
    std::cmatch subMatch;

    if (std::regex_match(line.begin(), line.end(), subMatch, request_line_pattern))
    {
        _method = subMatch[1].str();
        _path = subMatch[2].str();
        _version = subMatch[3].str();
        _state = HEADERS;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::_ParsePath()
{
    if (_path == "/")
    {
        _path = "/index.html";
    }
    else
    {
        for (auto &item : DEFAULT_HTML)
        {
            if (item == _path)
            {
                _path += ".html";
                break;
            }
        }
    }
}

// void HttpRequest::_ParseHeader(const std::string &line)
// {
//     patten = std::regex(R"(^([^:]*): ?(.*)$)", std::regex::optimize);
//     std::smatch subMatch;
//     if (regex_match(line, subMatch, patten))
//     {
//         _header[subMatch[1]] = subMatch[2];
//     }
//     else
//     {
//         _state = BODY;
//     }
// }

void HttpRequest::_ParseHeader(std::string_view line)
{
    std::cmatch subMatch;
    if (std::regex_match(line.begin(), line.end(), subMatch, header_pattern))
    {
        _header[subMatch[1].str()] = subMatch[2].str();
    }
    else
    {
        _state = BODY;
    }
}

// void HttpRequest::_ParseBody(const std::string &line)
// {
//     _body = line;
//     _ParsePost();
//     _state = FINISH;
//     LOG_DEBUG("Body: %s, len: %d", line.c_str(), line.size());
// }

void HttpRequest::_ParseBody(std::string_view line)
{
    _body = line;
    _ParsePost();
    _state = FINISH;
    LOG_DEBUG("Body: %s, len: %d", line.data(), line.size());
}

int HttpRequest::ConverHex(char ch)
{
    if (ch >= 'A' && ch <= 'F')
    {
        return ch - 'A' + 10;
    }
    if (ch >= 'a' && ch <= 'f')
    {
        return ch - 'a' + 10;
    }
    return ch;
}

void HttpRequest::_ParsePost()
{
    if (_method == "POST" && _header["Content-Type"] == "application/x-www-form-urlencoded")
    {
        _ParseFromUrlencoded();
        if (DEFAULT_HTML_TAG.count(_path))
        {
            int tag = DEFAULT_HTML_TAG.find(_path)->second;
            LOG_DEBUG("Tag:%d", tag);
            if (tag == 0 || tag == 1)
            {
                bool isLogin = (tag == 1);
                if (UserVerify(_post["username"], _post["password"], isLogin))
                {
                    _path = "/welcome.html";
                }
                else
                {
                    _path = "/error.html";
                }
            }
        }
    }
}

void HttpRequest::_ParseFromUrlencoded()
{
    if (_body.empty())
        return;

    std::string key, value;
    int num = 0;
    int n = _body.size();
    int i = 0, j = 0;

    for (; i < n; ++i)
    {
        char ch = _body[i];
        switch (ch)
        {
        case '=':
            key = _body.substr(j, i - j);
            j = i + 1;
            break;
        case '+':
            _body[i] = ' ';
            break;
        case '%':
            num = ConverHex(_body[i + 1]) * 16 + ConverHex(_body[i + 2]);
            _body[i + 2] = num % 10 + '0';
            _body[i + 1] = num / 10 + '0';
            i += 2;
            break;
        case '&':
            value = _body.substr(j, i - j);
            j = i + 1;
            _post[key] = value;
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }

    assert(j <= i);
    if (_post.count(key) == 0 && j < i)
    {
        value = _body.substr(j, i - j);
        _post[key] = value;
    }
}

bool HttpRequest::UserVerify(const std::string &name, const std::string &pwd, bool isLogin)
{
    if (name.empty() || pwd.empty())
        return false;

    LOG_INFO("Verify name:%s, pwd:%s", name.c_str(), pwd.c_str());
    MYSQL *sql;
    SqlConnRAII(&sql, SqlConnPool::Instance());
    assert(sql);

    bool flag = false;
    // unsigned int j = 0;
    char order[256] = {0};
    // MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;

    if (!isLogin)
        flag = true;

    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if (mysql_query(sql, order))
    {
        if (res)
            mysql_free_result(res);
        return false;
    }

    res = mysql_store_result(sql);
    if (!res)
    {
        LOG_ERROR("mysql_store_result error!");
        return false;
    }

    mysql_num_fields(res);
    mysql_fetch_fields(res);
    // j = mysql_num_fields(res);
    // fields = mysql_fetch_fields(res);

    while (MYSQL_ROW row = mysql_fetch_row(res))
    {
        LOG_DEBUG("MYSQL ROW: %s, %s", row[0], row[1]);
        std::string password(row[1]);
        if (isLogin)
        {
            if (pwd == password)
            {
                flag = true;
            }
            else
            {
                flag = false;
                LOG_INFO("pwd error!");
            }
        }
        else
        {
            flag = false;
            LOG_INFO("user used!");
        }
    }
    mysql_free_result(res);

    if (!isLogin && flag)
    {
        LOG_DEBUG("register!");
        bzero(order, 256);
        snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s', '%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG("%s", order);
        if (mysql_query(sql, order))
        {
            LOG_DEBUG("Insert Error!");
            flag = false;
        }
        else
        {
            flag = true;
        }
    }

    LOG_DEBUG("UserVerify Success!");
    return flag;
}

std::string HttpRequest::path() const
{
    return _path;
}

std::string &HttpRequest::path()
{
    return _path;
}

std::string HttpRequest::method() const
{
    return _method;
}

std::string HttpRequest::version() const
{
    return _version;
}

std::string HttpRequest::GetPost(const std::string &key) const
{
    assert(!key.empty());
    auto it = _post.find(key);
    return it != _post.end() ? it->second : "";
}

std::string HttpRequest::GetPost(const char *key) const
{
    assert(key != nullptr);
    auto it = _post.find(key);
    return it != _post.end() ? it->second : "";
}
