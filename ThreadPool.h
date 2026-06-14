#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <future>
#include <vector>
#include <atomic>
#include <stdexcept>

// 固定大小线程池：预创建worker线程，通过enqueue提交任务，支持dispatch获取返回值
class ThreadPool {
public:
    // 创建num_threads个工作线程
    explicit ThreadPool(size_t num_threads);

    // 禁止拷贝
    ThreadPool(const ThreadPool &) = delete;
    ThreadPool &operator=(const ThreadPool &) = delete;

    // 提交无返回值任务到队列
    void enqueue(std::function<void()> task);

    // 提交带返回值的任务，返回future用于获取结果
    template<typename F, typename... Args>
    auto dispatch(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using R = std::invoke_result_t<F, Args...>;
        // packaged_task包装任务，获取future后入队
        auto task = std::make_shared<std::packaged_task<R()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        auto future = task->get_future();
        enqueue([task] { (*task)(); });
        return future;
    }

    // 通知所有线程退出并等待完成
    void shutdown();

    // 析构自动调用shutdown
    ~ThreadPool();

    // 返回工作线程数
    size_t size() const noexcept { return workers_.size(); }

private:
    // 工作线程主循环：等待并执行任务
    void worker_loop();

    std::vector<std::thread> workers_;           // 工作线程列表
    std::queue<std::function<void()>> tasks_;    // 任务队列
    std::mutex mutex_;                           // 保护任务队列的锁
    std::condition_variable cv_;                 // 条件变量：唤醒等待线程
    std::atomic<bool> stop_{false};              // 停止标记
};