#include "http_parser.h"
#include <cassert>
#include <iostream>
#include <cstring>

// 重载 ParseResult 的流输出（用于 ASSERT_EQ 报错）
inline std::ostream &operator<<(std::ostream &os, HttpParser::ParseResult r) {
    switch (r) {
        case HttpParser::ParseResult::kNeedMore: return os << "kNeedMore";
        case HttpParser::ParseResult::kComplete: return os << "kComplete";
        case HttpParser::ParseResult::kError:    return os << "kError";
    }
    return os << "unknown";
}

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    void name(); \
    name(); \
    tests_run++;

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cerr << "  FAIL: " << #a << " != " << #b \
                  << " (" << (a) << " vs " << (b) << ")" << std::endl; \
        return; \
    }

#define ASSERT_TRUE(cond) \
    if (!(cond)) { \
        std::cerr << "  FAIL: " << #cond << std::endl; \
        return; \
    }

// ============================================================
void test_simple_get() {
    tests_run++;
    HttpParser parser;
    const char *raw =
        "GET /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";

    auto result = parser.Parse(raw, strlen(raw));
    ASSERT_EQ(result, HttpParser::ParseResult::kComplete);

    const auto &req = parser.GetRequest();
    ASSERT_EQ(req.method, "GET");
    ASSERT_EQ(req.path, "/index.html");
    ASSERT_EQ(req.version, "HTTP/1.1");
    ASSERT_EQ(req.headers.at("Host"), "localhost");

    tests_passed++;
    std::cout << "  ✓ test_simple_get" << std::endl;
}

// ============================================================
void test_post_with_body() {
    tests_run++;
    HttpParser parser;
    std::string raw =
        "POST /api/data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, World!";

    auto result = parser.Parse(raw.data(), raw.size());
    ASSERT_EQ(result, HttpParser::ParseResult::kComplete);

    const auto &req = parser.GetRequest();
    ASSERT_EQ(req.method, "POST");
    ASSERT_EQ(req.body, "Hello, World!");

    tests_passed++;
    std::cout << "  ✓ test_post_with_body" << std::endl;
}

// ============================================================
void test_incremental_parsing() {
    tests_run++;
    HttpParser parser;

    // 第一批：仅发送请求行
    auto r1 = parser.Parse("GET /api HTTP/1.1\r\n", 19);
    ASSERT_EQ(r1, HttpParser::ParseResult::kNeedMore);

    // 第二批：发送 headers
    auto r2 = parser.Parse("Host: test\r\n\r\n", 14);
    ASSERT_EQ(r2, HttpParser::ParseResult::kComplete);

    const auto &req = parser.GetRequest();
    ASSERT_EQ(req.method, "GET");
    ASSERT_EQ(req.path, "/api");
    ASSERT_EQ(req.headers.at("Host"), "test");

    tests_passed++;
    std::cout << "  ✓ test_incremental_parsing" << std::endl;
}

// ============================================================
void test_reset() {
    tests_run++;
    HttpParser parser;

    auto r1 = parser.Parse("GET /first HTTP/1.1\r\nHost: a\r\n\r\n", 40);
    ASSERT_EQ(r1, HttpParser::ParseResult::kComplete);
    ASSERT_EQ(parser.GetRequest().path, "/first");

    parser.Reset();
    auto r2 = parser.Parse("GET /second HTTP/1.1\r\nHost: b\r\n\r\n", 41);
    ASSERT_EQ(r2, HttpParser::ParseResult::kComplete);
    ASSERT_EQ(parser.GetRequest().path, "/second");

    tests_passed++;
    std::cout << "  ✓ test_reset" << std::endl;
}

// ============================================================
int main() {
    std::cout << "HttpParser 单元测试" << std::endl;
    std::cout << "==================" << std::endl;

    test_simple_get();
    test_post_with_body();
    test_incremental_parsing();
    test_reset();

    std::cout << "\n结果: " << tests_passed << "/" << tests_run << " 通过" << std::endl;
    return (tests_passed == tests_run) ? 0 : 1;
}
