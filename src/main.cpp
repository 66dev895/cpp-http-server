#include "server.h"
#include "http_response.h"
#include <iostream>
#include <csignal>
#include <sstream>
#include <chrono>
#include <ctime>

// 全局 server 指针，用于信号处理
static HttpServer *g_server = nullptr;

void SignalHandler(int) {
    std::cout << "\n[HttpServer] 收到退出信号，正在优雅关闭..." << std::endl;
    if (g_server) g_server->Stop();
}

int main() {
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    try {
        HttpServer server(8080, 4);
        g_server = &server;

        // ======== API 路由 ========

        // GET / — 首页
        server.Get("/", [](const HttpRequest &) {
            std::string html = R"(<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <title>C++ HTTP Server</title>
    <style>
        * { margin:0; padding:0; box-sizing:border-box; }
        body { font-family: 'Segoe UI', sans-serif; background:#0d1117; color:#c9d1d9;
               display:flex; justify-content:center; align-items:center; min-height:100vh; }
        .card { background:#161b22; border:1px solid #30363d; border-radius:12px;
                padding:48px; max-width:600px; text-align:center; }
        h1 { color:#58a6ff; font-size:2rem; margin-bottom:12px; }
        .tag { display:inline-block; background:#1f6feb22; color:#58a6ff; border:1px solid #1f6feb44;
               padding:4px 12px; border-radius:20px; font-size:0.85rem; margin:4px; }
        .stats { display:flex; gap:24px; justify-content:center; margin:24px 0; }
        .stat { text-align:center; }
        .stat .num { font-size:2rem; font-weight:700; color:#58a6ff; }
        .stat .label { font-size:0.8rem; color:#8b949e; }
        a { color:#58a6ff; text-decoration:none; }
        .footer { margin-top:32px; color:#484f58; font-size:0.8rem; }
    </style>
</head>
<body>
    <div class="card">
        <h1>⚡ C++ HTTP Server</h1>
        <p style="color:#8b949e;margin:8px 0;">
            基于 epoll + 线程池的高性能 HTTP/1.1 服务器
        </p>
        <div>
            <span class="tag">C++17</span>
            <span class="tag">epoll</span>
            <span class="tag">Thread Pool</span>
            <span class="tag">OOP</span>
            <span class="tag">Linux</span>
        </div>
        <div class="stats">
            <div class="stat">
                <div class="num" id="requests">—</div>
                <div class="label">已处理请求</div>
            </div>
            <div class="stat">
                <div class="num" id="uptime">—</div>
                <div class="label">运行时间</div>
            </div>
        </div>
        <p style="margin-top:16px;">
            <a href="/api/hello">GET /api/hello →</a> &nbsp;
            <a href="/api/bench">GET /api/bench →</a>
        </p>
        <div class="footer">
            Built with ❤️ | <a href="https://github.com/66dev895/cpp-http-server">GitHub</a>
        </div>
    </div>
    <script>
        fetch('/api/stats').then(r=>r.json()).then(d=>{
            document.getElementById('requests').textContent = d.requests;
            document.getElementById('uptime').textContent = d.uptime + 's';
        });
    </script>
</body>
</html>)";
            return HttpResponse::Ok(html);
        });

        // GET /api/hello
        server.Get("/api/hello", [](const HttpRequest &) {
            return HttpResponse::Json(R"({"message":"Hello from C++ HTTP Server!","status":"ok"})");
        });

        // GET /api/stats
        server.Get("/api/stats", [](const HttpRequest &req) {
            // 通过外部 g_server 获取统计（实际项目中会改进注入方式）
            (void)req;
            std::ostringstream json;
            json << R"({"requests":)" << g_server->RequestsHandled()
                 << R"(,"uptime":)" << std::time(nullptr)
                 << R"(,"threads":)" << std::thread::hardware_concurrency()
                 << "}";
            return HttpResponse::Json(json.str());
        });

        // GET /api/bench — 纯 JSON 响应（用于压测）
        server.Get("/api/bench", [](const HttpRequest &) {
            return HttpResponse::Json(R"({"result":"ok"})");
        });

        // POST /api/echo — 回显请求体
        server.Post("/api/echo", [](const HttpRequest &req) {
            std::ostringstream json;
            json << R"({"echo":")" << req.body << R"(","length":)" << req.body.size() << "}";
            return HttpResponse::Json(json.str());
        });

        std::cout << "========================================" << std::endl;
        std::cout << "  C++ HTTP Server — Ready" << std::endl;
        std::cout << "  http://localhost:8080" << std::endl;
        std::cout << "========================================" << std::endl;

        server.Start();

    } catch (const std::exception &e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "[HttpServer] 已关闭。共处理 "
              << g_server->RequestsHandled() << " 个请求。" << std::endl;
    return 0;
}
