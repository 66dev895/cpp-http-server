# C++ HTTP Server 压测指南

## 环境准备

```bash
# 安装 wrk
sudo apt install wrk

# 或使用 ApacheBench
sudo apt install apache2-utils
```

## 启动服务器

```bash
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
./http_server
```

## 压测命令

### wrk 基准测试

```bash
# 简单 JSON 接口 — 60 秒压测，4 线程，100 连接
wrk -t4 -c100 -d60s http://localhost:8080/api/bench

# 静态页面
wrk -t4 -c100 -d30s http://localhost:8080/

# POST 回显
wrk -t4 -c50 -d30s -s post.lua http://localhost:8080/api/echo
```

### ApacheBench

```bash
# 10 万请求，100 并发
ab -n 100000 -c 100 http://localhost:8080/api/bench
```

## 预期性能指标（参考环境：4 核 8G Linux）

| 指标 | 数值 |
|------|------|
| QPS（Keep-Alive / 短连接） | 15,000+ / 5,000+ |
| 平均延迟 | < 5ms |
| P99 延迟 | < 20ms |
| 内存占用 | < 50MB |
