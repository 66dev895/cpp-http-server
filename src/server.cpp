#include "server.h"

#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

static constexpr int kMaxEvents = 1024;
static constexpr int kMaxBuf    = 4096;

// ============================================================
// 辅助：设置 fd 为非阻塞
// ============================================================
static void SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

// ============================================================
// HttpServer 构造 & 析构
// ============================================================
HttpServer::HttpServer(uint16_t port, size_t num_threads)
    : port_(port)
    , listen_fd_(-1)
    , epoll_fd_(-1)
    , running_(false)
    , thread_pool_(num_threads > 0 ? num_threads
                                   : std::thread::hardware_concurrency())
    , requests_handled_(0)
{
    SetupSocket();
}

HttpServer::~HttpServer() {
    Stop();
    if (listen_fd_ >= 0) close(listen_fd_);
    if (epoll_fd_ >= 0)  close(epoll_fd_);
}

// ============================================================
// 路由注册（链式调用）
// ============================================================
HttpServer &HttpServer::Get(const std::string &pattern, Router::Handler handler) {
    router_.Get(pattern, std::move(handler));
    return *this;
}

HttpServer &HttpServer::Post(const std::string &pattern, Router::Handler handler) {
    router_.Post(pattern, std::move(handler));
    return *this;
}

HttpServer &HttpServer::SetStaticDir(const std::string &dir) {
    static_dir_ = dir;
    return *this;
}

// ============================================================
// 初始化 socket + epoll
// ============================================================
void HttpServer::SetupSocket() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        throw std::runtime_error("socket() 失败");
    }

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
               &opt, sizeof(opt));
    SetNonBlocking(listen_fd_);

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port_);

    if (bind(listen_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error("bind() 失败，端口 " + std::to_string(port_) + " 可能被占用");
    }
    if (listen(listen_fd_, SOMAXCONN) < 0) {
        throw std::runtime_error("listen() 失败");
    }

    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        throw std::runtime_error("epoll_create1() 失败");
    }

    epoll_event ev{};
    ev.events  = EPOLLIN | EPOLLET;  // 边缘触发
    ev.data.fd = listen_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);
}

// ============================================================
// 启动服务器
// ============================================================
void HttpServer::Start() {
    running_.store(true);
    std::cout << "[HttpServer] 监听 0.0.0.0:" << port_
              << " | 工作线程: " << std::thread::hardware_concurrency()
              << " | 静态文件: " << (static_dir_.empty() ? "(无)" : static_dir_)
              << std::endl;
    EventLoop();
}

void HttpServer::Stop() {
    running_.store(false);
}

// ============================================================
// epoll 事件循环（主线程）
// ============================================================
void HttpServer::EventLoop() {
    epoll_event events[kMaxEvents];

    while (running_.load()) {
        int nfds = epoll_wait(epoll_fd_, events, kMaxEvents, 1000);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            std::cerr << "epoll_wait 错误" << std::endl;
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;

            if (fd == listen_fd_) {
                // ---- 新连接 ----
                while (true) {
                    sockaddr_in client_addr{};
                    socklen_t addr_len = sizeof(client_addr);
                    int conn_fd = accept4(listen_fd_,
                                          reinterpret_cast<sockaddr *>(&client_addr),
                                          &addr_len,
                                          SOCK_NONBLOCK);
                    if (conn_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        continue;
                    }

                    epoll_event ev{};
                    ev.events  = EPOLLIN | EPOLLET | EPOLLONESHOT;
                    ev.data.fd = conn_fd;
                    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, conn_fd, &ev);
                }
            } else {
                // ---- 可读事件：交给工作线程 ----
                thread_pool_.Submit([this, fd]() {
                    char buf[kMaxBuf];
                    HttpParser parser;

                    while (true) {
                        ssize_t n = recv(fd, buf, sizeof(buf), 0);
                        if (n > 0) {
                            auto result = parser.Parse(buf, static_cast<size_t>(n));
                            if (result == HttpParser::ParseResult::kComplete) {
                                const auto &req = parser.GetRequest();

                                // 静态文件优先
                                HttpResponse resp;
                                if (!static_dir_.empty() &&
                                    req.method == "GET" &&
                                    req.path.find("..") == std::string::npos) {
                                    std::string file_path = static_dir_ + req.path;
                                    if (req.path == "/") file_path = static_dir_ + "/index.html";
                                    resp = HttpResponse::File(file_path);
                                } else {
                                    resp = router_.Dispatch(req);
                                }

                                std::string raw = resp.ToString();
                                send(fd, raw.data(), raw.size(), MSG_NOSIGNAL);
                                requests_handled_.fetch_add(1);
                                parser.Reset();
                            } else if (result == HttpParser::ParseResult::kError) {
                                auto bad = HttpResponse::BadRequest().ToString();
                                send(fd, bad.data(), bad.size(), MSG_NOSIGNAL);
                                break;
                            }
                            // kNeedMore → 继续读取
                        } else if (n == 0) {
                            break; // 对端关闭
                        } else {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                            break; // 读错误
                        }
                    }

                    // 重新挂载 EPOLLONESHOT 事件
                    epoll_event ev{};
                    ev.events  = EPOLLIN | EPOLLET | EPOLLONESHOT;
                    ev.data.fd = fd;
                    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev);
                });
            }
        }
    }
}
