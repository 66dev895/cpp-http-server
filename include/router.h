#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <regex>
#include "http_parser.h"
#include "http_response.h"

/**
 * Router — URL 路由分发器
 *
 * 支持精确匹配和正则匹配。
 * handler 签名：HttpResponse(const HttpRequest &)
 */
class Router {
public:
    using Handler = std::function<HttpResponse(const HttpRequest &)>;

    /// 注册 GET 路由
    Router &Get(const std::string &pattern, Handler handler);

    /// 注册 POST 路由
    Router &Post(const std::string &pattern, Handler handler);

    /// 查找并执行匹配的 handler；
    /// 未匹配返回 404
    HttpResponse Dispatch(const HttpRequest &req) const;

private:
    struct Route {
        std::regex pattern;
        Handler handler;
    };

    std::unordered_map<std::string, std::vector<Route>> routes_;
};
