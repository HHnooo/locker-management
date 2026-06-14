#include "ThreadPool.h"

// 创建num_threads个工作线程，启动worker_loop
ThreadPool::ThreadPool(size_t num_threads) {
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
}

// 将任务加入队列，唤醒一个等待线程
// 若线程池已关闭则抛出异常
void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard lock(mutex_);
        if (stop_) throw std::runtime_error("线程池已关闭，无法接受新任务");
        tasks_.push(std::move(task));
    }
    cv_.notify_one();  // 唤醒一个worker
}

// worker主循环：等待任务→取出→执行，直到stop且队列空
void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock lock(mutex_);
            // 等待直到有任务或线程池停止
            cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
            // stop且队列空则退出
            if (stop_ && tasks_.empty()) return;
            // 取出队首任务
            task = std::move(tasks_.front());
            tasks_.pop();
        }
        task();  // 解锁后执行，允许其他线程并发取任务
    }
}

// 设置stop标记，唤醒所有worker，等待全部结束
void ThreadPool::shutdown() {
    {
        std::lock_guard lock(mutex_);
        stop_ = true;
    }
    cv_.notify_all();
    for (auto &worker : workers_) {
        if (worker.joinable()) worker.join();
    }
}

// 析构：自动关闭线程池
ThreadPool::~ThreadPool() {
    shutdown();
}