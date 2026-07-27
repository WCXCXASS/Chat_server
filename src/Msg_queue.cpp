#include "Msg_queue.h"

Msg_que::Msg_que(int num = 2) : th_num(num) { }

Msg_que::~Msg_que()
{
    stop();

    for (auto& t : workers)
    {
        if (t.joinable()) t.join();
    }
}

void Msg_que::start(std::function<void(Msg_packet msg)> handle_fuc)
{
    for (int i = 0; i < th_num; i++)
    {
        workers.emplace_back(std::thread(&Msg_que::handle_work, this, handle_fuc));
    }
}

void Msg_que::push(Msg_packet msg)
{
    {
        std::unique_lock<std::mutex> lock(mtx_b);
        if (!shoutdown) buf_que.push(std::move(msg));
    }
    
    has_data_ = true;
    cv_.notify_one();
}

void Msg_que::swap()
{
    msg_que.swap(buf_que);
}

void Msg_que::handle_work(std::function<void(Msg_packet msg)> handle_fuc)
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mtx_m);

        if (shoutdown && msg_que.empty())
        {
            std::unique_lock<std::mutex> lock_b(mtx_b);
            if (!buf_que.empty())
            {
                has_data_ = false;
                swap();
            }
            else
            {
                return;
            }
        }
        
        if (!msg_que.empty())
        {
            Msg_packet msg = std::move(msg_que.front());
            msg_que.pop();
            lock.unlock();

            handle_fuc(msg);

            continue;
        }

        if (has_data_.exchange(false, std::memory_order_seq_cst))
        {
            std::unique_lock<std::mutex> lock_b(mtx_b);
            if (!buf_que.empty()) swap();

            lock.unlock();
            continue;
        }

        cv_.wait(lock, [this](){
            return !msg_que.empty() || has_data_ || shoutdown;
        });
    }
}

void Msg_que::stop()
{
    shoutdown = true;
    cv_.notify_all();
}