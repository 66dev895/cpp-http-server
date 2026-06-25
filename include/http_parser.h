#pragma once

#include <string>
#include <unordered_map>
#include <string_view>
#include <vector>

/**
 * HttpRequest — 解析后的 HTTP 请求对象
 */
struct HttpRequest {
    std::string method;
    std::string path;
    std::string version;
    std::unordered_map<std::string, std::string> headers;
    std::string body;

    std::string_view GetHeader(const std::string &key) const {
        auto it = headers.find(key);
        return it != headers.end()
            ? std::string_view(it->second)
            : std::string_view{};
    }
};

/**
 * HttpParser — 增量式 HTTP/1.1 请求解析器
 *
 * 支持 TCP 流式数据的增量解析。
 * 内置缓冲区管理，上层只需不断 append 收到的数据。
 */
class HttpParser {
public:
    enum class ParseResult { kNeedMore, kComplete, kError };

    HttpParser();

    ParseResult Parse(const char *data, size_t len);
    const HttpRequest &GetRequest() const { return request_; }
    void Reset();

private:
    enum class State { kRequestLine, kHeaders, kBody, kDone };

    ParseResult ParseRequestLine();
    ParseResult ParseHeaders();
    ParseResult ParseBody();

    std::vector<char> buffer_;
    State state_;
    HttpRequest request_;
    size_t content_length_;
};
