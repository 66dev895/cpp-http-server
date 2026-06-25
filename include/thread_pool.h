#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <type_traits>

/**
 * ThreadPool — 固定大小的线程池
 *
 * 使用变参模板 submit() 提交任意可调用对象，返回 std::future<Result>。
 * 析构时自动等待所有任务完成。
 *
 * 使用示例：
 *   ThreadPool pool(4);
 *   auto future = pool.Submit([](int x) { return x * 2; }, 21);
 *   int result = future.get(); // 42
 */
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads);
    ~ThreadPool();

    // 禁止拷贝和移动
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    /// 提交任务，返回 future 以获取结果
    template <typename F, typename... Args>
    auto Submit(F &&f, Args &&...args)
        -> std::future<typename std::invoke_result<F, Args...>::type>;

    /// 当前队列中的任务数
    size_t PendingTasks() const;

private:
    void WorkerLoop();

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool stop_;
};

// ---- 模板实现 ----

template <typename F, typename... Args>
auto ThreadPool::Submit(F &&f, Args &&...args)
    -> std::future<typename std::invoke_result<F, Args...>::type> {

    using ReturnType = typename std::invoke_result<F, Args...>::type;

    // 将任务打包为 packaged_task
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<ReturnType> result = task->get_future();

    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (stop_) {
            throw std::runtime_error("ThreadPool: 向已停止的线程池提交任务");
        }
        tasks_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return result;
}
