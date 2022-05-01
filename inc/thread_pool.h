#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <cassert>

class FunctionPool
{
private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_function_queue;
    std::mutex m_lock;
    std::condition_variable m_data_condition;
    bool m_terminated;

public:
    FunctionPool() : m_function_queue(), m_lock(), m_data_condition(), m_terminated(false)
    {
        for (auto i = 0; i < 2; ++i)
        {
            m_workers.push_back(std::thread(&FunctionPool::infinite_loop_func, this));
        }
    }
    ~FunctionPool()
    {
        done();
        for (auto &i : m_workers)
        {
            i.join();
        }
    };

    template <typename T, typename... Args>
    void post(T &&f, Args &&...args)
    {
        auto func = std::bind(std::forward<T>(f), std::forward<Args>(args)...);
        std::unique_lock<std::mutex> lock(m_lock);
        m_function_queue.push(func);
        lock.unlock();
        m_data_condition.notify_one();
    }

    void done()
    {
        std::unique_lock<std::mutex> lock(m_lock);
        m_terminated = true;
        lock.unlock();
        m_data_condition.notify_all();
    }

    void infinite_loop_func()
    {
        std::function<void()> func;
        while (true)
        {
            {
                std::unique_lock<std::mutex> lock(m_lock);
                m_data_condition.wait(lock, [this]()
                                      { return !m_function_queue.empty() || m_terminated; });
                if (m_terminated && m_function_queue.empty())
                {
                    // finish the thread loop and let it join in the main thread.
                    return;
                }
                func = m_function_queue.front();
                m_function_queue.pop();
            }
            func();
        }
    }
};

#endif