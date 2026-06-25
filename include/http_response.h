#pragma once

#include <string>
#include <unordered_map>

/**
 * HttpResponse — HTTP 响应构造器
 *
 * 流式接口设计：SetStatus() → SetHeader() → SetBody() → ToString()
 */
class HttpResponse {
public:
    HttpResponse();

    HttpResponse &SetStatus(int code, const std::string &message);
    HttpResponse &SetHeader(const std::string &key, const std::string &value);
    HttpResponse &SetBody(const std::string &body);
    HttpResponse &SetBody(std::string &&body);

    /// 将响应序列化为 HTTP/1.1 字节流
    std::string ToString() const;

    // ---- 快捷静态方法 ----
    static HttpResponse Ok(const std::string &body,
                           const std::string &content_type = "text/html");
    static HttpResponse Json(const std::string &body);
    static HttpResponse NotFound();
    static HttpResponse BadRequest(const std::string &msg = "Bad Request");
    static HttpResponse ServerError(const std::string &msg = "Internal Server Error");
    static HttpResponse File(const std::string &path, const std::string &mime = "");

private:
    static std::string GetMimeType(const std::string &path);

    int status_code_;
    std::string status_message_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};
