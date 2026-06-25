#include "http_response.h"
#include <sstream>
#include <fstream>
#include <filesystem>
#include <ctime>

namespace fs = std::filesystem;

HttpResponse::HttpResponse()
    : status_code_(200), status_message_("OK") {}

HttpResponse &HttpResponse::SetStatus(int code, const std::string &message) {
    status_code_ = code;
    status_message_ = message;
    return *this;
}

HttpResponse &HttpResponse::SetHeader(const std::string &key, const std::string &value) {
    headers_[key] = value;
    return *this;
}

HttpResponse &HttpResponse::SetBody(const std::string &body) {
    body_ = body;
    return *this;
}

HttpResponse &HttpResponse::SetBody(std::string &&body) {
    body_ = std::move(body);
    return *this;
}

std::string HttpResponse::ToString() const {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status_code_ << " " << status_message_ << "\r\n";
    oss << "Content-Length: " << body_.size() << "\r\n";
    oss << "Connection: keep-alive\r\n";
    for (const auto &[k, v] : headers_) {
        oss << k << ": " << v << "\r\n";
    }
    oss << "\r\n";
    oss << body_;
    return oss.str();
}

// ---- 快捷静态方法 ----

HttpResponse HttpResponse::Ok(const std::string &body, const std::string &content_type) {
    return HttpResponse()
        .SetStatus(200, "OK")
        .SetHeader("Content-Type", content_type)
        .SetBody(body);
}

HttpResponse HttpResponse::Json(const std::string &body) {
    return HttpResponse()
        .SetStatus(200, "OK")
        .SetHeader("Content-Type", "application/json")
        .SetBody(body);
}

HttpResponse HttpResponse::NotFound() {
    std::string body = "<h1>404 Not Found</h1>";
    return HttpResponse()
        .SetStatus(404, "Not Found")
        .SetHeader("Content-Type", "text/html")
        .SetBody(body);
}

HttpResponse HttpResponse::BadRequest(const std::string &msg) {
    return HttpResponse()
        .SetStatus(400, "Bad Request")
        .SetHeader("Content-Type", "text/plain")
        .SetBody(msg);
}

HttpResponse HttpResponse::ServerError(const std::string &msg) {
    return HttpResponse()
        .SetStatus(500, "Internal Server Error")
        .SetHeader("Content-Type", "text/plain")
        .SetBody(msg);
}

HttpResponse HttpResponse::File(const std::string &path, const std::string &mime) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f.is_open()) return NotFound();

    std::streamsize size = f.tellg();
    f.seekg(0, std::ios::beg);

    std::string content(size, '\0');
    f.read(content.data(), size);

    std::string content_type = mime.empty() ? GetMimeType(path) : mime;
    return HttpResponse()
        .SetStatus(200, "OK")
        .SetHeader("Content-Type", content_type)
        .SetBody(std::move(content));
}

std::string HttpResponse::GetMimeType(const std::string &path) {
    static const std::unordered_map<std::string, std::string> mime_map = {
        {".html", "text/html"},
        {".css",  "text/css"},
        {".js",   "application/javascript"},
        {".json", "application/json"},
        {".png",  "image/png"},
        {".jpg",  "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif",  "image/gif"},
        {".svg",  "image/svg+xml"},
        {".ico",  "image/x-icon"},
        {".txt",  "text/plain"},
        {".pdf",  "application/pdf"},
    };

    fs::path p(path);
    std::string ext = p.extension().string();
    auto it = mime_map.find(ext);
    return it != mime_map.end() ? it->second : "application/octet-stream";
}
