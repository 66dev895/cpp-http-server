#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <netinet/in.h>
#include "router.h"
#include "thread_pool.h"

/**
 * HttpServer — 基于 epoll + 线程池的高性能 HTTP 服务器
 *
 * 架构：
 *   主线程（I/O）            工作线程池
 *   ┌───────────┐          ┌──────────┐
 *   │ epoll_wait│──task──▶│ 解析请求  │
 *   │ 事件循环   │          │ 路由分发  │
 *   │ accept/read│          │ 构造响应  │
 *   └───────────┘          │ 写回socket│
 *                          └──────────┘
 *
 * 使用示例：
 *   HttpServer server(8080, 4);
 *   server.Get("/", [](auto &req) {
 *       return HttpResponse::Ok("<h1>Hello!</h1>");
 *   });
 *   server.Start();
 */
class HttpServer {
public:
    /// @param port 监听端口
    /// @param num_threads 工作线程数（0 = CPU 核数）
    explicit HttpServer(uint16_t port, size_t num_threads = 0);
    ~HttpServer();

    // 禁止拷贝
    HttpServer(const HttpServer &) = delete;
    HttpServer &operator=(const HttpServer &) = delete;

    // ---- 路由注册 ----
    Router &GetRouter() { return router_; }

    HttpServer &Get(const std::string &pattern, Router::Handler handler);
    HttpServer &Post(const std::string &pattern, Router::Handler handler);

    /// 设置静态文件根目录
    HttpServer &SetStaticDir(const std::string &dir);

    // ---- 生命周期 ----
    /// 启动服务器（阻塞，直到 Stop() 被调用）
    void Start();

    /// 优雅关闭
    void Stop();

    // ---- 统计信息 ----
    uint64_t RequestsHandled() const { return requests_handled_; }

private:
    void SetupSocket();
    void EventLoop();

    uint16_t port_;
    int listen_fd_;
    int epoll_fd_;
    std::atomic<bool> running_;
    Router router_;
    ThreadPool thread_pool_;
    std::string static_dir_;
    std::atomic<uint64_t> requests_handled_;
};
