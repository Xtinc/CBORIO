#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cassert>

class FunctionPool
{
private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_function_queue;
    std::mutex m_lock;
    std::condition_variable m_data_condition;
    std::atomic<bool> m_terminate_pool;

public:
    FunctionPool(unsigned int workers = 1)
        : m_function_queue(), m_lock(), m_data_condition(), m_terminate_pool(true)
    {
        for (auto i = 0u; i < workers; ++i)
        {
            m_workers.push_back(std::thread([this]()
                                            { this->infinite_loop_func(); }));
        }
    }
    ~FunctionPool()
    {
        done();
    }
    template <typename T, typename... Args>
    void post(T &&f, Args &&...args)
    {
        std::function<decltype(f(args...))()> func =
            std::bind(std::forward<T>(f), std::forward<Args>(args)...);
        auto packed_func = [&func]()
        {
            func();
        };
        std::unique_lock<std::mutex> lock(m_lock);
        m_function_queue.push(packed_func);
        lock.unlock();
        m_data_condition.notify_one();
    }
    void done()
    {
        m_terminate_pool = true;
        m_data_condition.notify_all();
        for (auto &i : m_workers)
        {
            i.join();
        }
    }
    void infinite_loop_func()
    {
        std::function<void()> func;
        while (true)
        {
            {
                std::unique_lock<std::mutex> lock(m_lock);
                m_data_condition.wait(lock, [this]()
                                      { return !m_function_queue.empty() || m_terminate_pool; });
                if (m_function_queue.empty() && m_terminate_pool)
                {
                    return; // auto join threads.
                }
                func = m_function_queue.front();
                m_function_queue.pop();
            }
            func();
        }
    }
};

#endif