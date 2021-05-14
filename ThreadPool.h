#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <thread>
#include <vector>
#include <type_traits>
#include <future>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <utility>

class ThreadPool {
public:
    ThreadPool(size_t n) {
        workers_.reserve(n);
        for (size_t i = 0; i < n; ++i) {
            workers_.push_back(std::thread(std::bind(&ThreadPool::work, this)));
        }
    }
    ~ThreadPool() {
        if (!stop_) {
            stop();
        }
        auto n = workers_.size();
        for (auto i = 0; i < n; ++i) {
            workers_[i].join();
        }
    }
    ThreadPool(const ThreadPool& rhs) = delete;
    ThreadPool(ThreadPool&& rhs) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
    ThreadPool& operator=(ThreadPool&& rhs) = delete;

    template<typename F, typename ...Args>
    decltype(auto) enqueue(F&& f, Args&&... args) {
        using ret_type = std::invoke_result_t<F, Args...>;
        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        // if don't use pointer, use obj, what will happend?
        auto ptask = std::make_shared<std::packaged_task<ret_type()>>(task);
        auto fu = ptask->get_future();

        auto func = [ptask]() { (*ptask)(); };

        {
            std::unique_lock l(lock_);
            if (tasks_.empty()) {
                cv_.notify_all();
            }
            tasks_.emplace(func);
        }

        return fu;
    }
    void dequeue() {
        std::unique_lock l(lock_);
        if (!tasks_.empty()) {
            tasks_.pop();
        }
    }
    void stop() {
        {
            std::unique_lock l(lock_);
            stop_ = true;
        }
        cv_.notify_all();
    }
    void drain() {
        std::unique_lock l(lock_);
        wait_cv_.wait(l, [this]() { return tasks_.empty(); });
    }
private:
    void work() {
        std::unique_lock l(lock_);
        
        while (!stop_) {
            cv_.wait(l, [this]() { return stop_ || !tasks_.empty(); });
            if (stop_) {
                break;
            }

            auto task = tasks_.front();
            tasks_.pop();

            lock_.unlock();
            task();
            lock_.lock();
            // after task end
            if (tasks_.empty()) {
                wait_cv_.notify_all();
            }
        }
    }

private:
    std::queue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;

    std::mutex lock_;
    std::condition_variable cv_;
    std::condition_variable wait_cv_;
    bool stop_{false};
};

#endif // THREAD_POOL_H
