#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>
#include <error.h>
#include <mysql/mysql.h>

#include "../Buffer/Buffer.hpp"
#include "../Log/Log.hpp"
#include "../SQL/SQLconnPool.hpp"

class HttpRequest
{
private:
    // bool _ParseRequestLine(std::string_view line);
    // void _ParseHeader(std::string_view line);
    // void _ParseBody(std::string_view line);
    bool _ParseRequestLine(const std::string& lin);
    void _ParseHeader(const std::string& lin);
    void _ParseBody(const std::string& lin);

    void _ParsePath();
    void _ParsePost();
    void _ParseFromUrlencoded();

    static bool UserVerify(const std::string &name, const std::string &pwd, bool isLogin);
    static int ConverHex(char ch);

private:
    enum PARSE_STATE
    {
        REQUEST_LINE,
        HEADERS,
        BODY,
        FINISH,
    };
    
    PARSE_STATE _state;
    std::string _method, _path, _version, _body;
    std::unordered_map<std::string, std::string> _header;
    std::unordered_map<std::string, std::string> _post;
    
    static const std::regex request_line_pattern;
    static const std::regex header_pattern;
    static const std::unordered_set<std::string> DEFAULT_HTML;
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    
public:
    HttpRequest();
    ~HttpRequest() = default;

    void Init();
    bool parse(Buffer &buff);
    
    std::string path() const;
    std::string &path();
    std::string method() const;
    std::string version() const;
    std::string GetPost(const std::string &key) const;
    std::string GetPost(const char *key) const;

    bool IsKeepAlive() const;
};

#endif // HTTPREQUEST_HPP