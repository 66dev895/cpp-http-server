#include "http_parser.h"
#include <cstring>
#include <sstream>
#include <algorithm>

HttpParser::HttpParser()
    : state_(State::kRequestLine)
    , content_length_(0) {}

HttpParser::ParseResult HttpParser::Parse(const char *data, size_t len) {
    if (len == 0) return ParseResult::kNeedMore;

    buffer_.insert(buffer_.end(), data, data + len);

    while (true) {
        switch (state_) {
            case State::kRequestLine: {
                auto r = ParseRequestLine();
                if (r != ParseResult::kComplete) return r;
                state_ = State::kHeaders;
                break;
            }
            case State::kHeaders: {
                auto r = ParseHeaders();
                if (r != ParseResult::kComplete) return r;
                state_ = (content_length_ > 0) ? State::kBody : State::kDone;
                break;
            }
            case State::kBody: {
                auto r = ParseBody();
                if (r != ParseResult::kComplete) return r;
                state_ = State::kDone;
                break;
            }
            case State::kDone:
                return ParseResult::kComplete;
        }
    }
}

HttpParser::ParseResult HttpParser::ParseRequestLine() {
    auto it = std::search(buffer_.begin(), buffer_.end(),
                          "\r\n", "\r\n" + 2);
    if (it == buffer_.end()) return ParseResult::kNeedMore;

    std::string line(buffer_.begin(), it);
    buffer_.erase(buffer_.begin(), it + 2);

    std::istringstream ss(line);
    ss >> request_.method >> request_.path >> request_.version;
    if (request_.method.empty() || request_.path.empty()) {
        return ParseResult::kError;
    }
    return ParseResult::kComplete;
}

HttpParser::ParseResult HttpParser::ParseHeaders() {
    while (true) {
        auto it = std::search(buffer_.begin(), buffer_.end(),
                              "\r\n", "\r\n" + 2);
        if (it == buffer_.end()) return ParseResult::kNeedMore;

        std::string line(buffer_.begin(), it);
        buffer_.erase(buffer_.begin(), it + 2);

        if (line.empty()) {
            // 空行 = header 结束
            auto content_len = request_.GetHeader("Content-Length");
            if (!content_len.empty()) {
                content_length_ = std::stoul(std::string(content_len));
            }
            return ParseResult::kComplete;
        }

        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon + 1);
            // trim leading whitespace
            val.erase(0, val.find_first_not_of(" \t"));
            request_.headers[key] = val;
        }
    }
}

HttpParser::ParseResult HttpParser::ParseBody() {
    if (buffer_.size() < content_length_) return ParseResult::kNeedMore;

    request_.body.assign(buffer_.begin(), buffer_.begin() + content_length_);
    buffer_.erase(buffer_.begin(), buffer_.begin() + content_length_);
    return ParseResult::kComplete;
}

void HttpParser::Reset() {
    state_ = State::kRequestLine;
    content_length_ = 0;
    buffer_.clear();
    request_ = HttpRequest{};
}
