#include "Msg_queue.h"

Msg_que::Msg_que(int num = 2) : th_num(num)
{
    for (int i = 0; i < num; i++)
    {
        workers_data.emplace_back();
    }
}

Msg_que::~Msg_que()
{
    stop();

    for (auto& t : threads)
    {
        if (t.joinable()) t.join();
    }
}

void Msg_que::start(std::function<void(Msg_packet msg)> handle_fuc)
{
    for (int i = 0; i < th_num; i++)
    {
        threads.emplace_back(std::thread(&Msg_que::handle_work, this, i, handle_fuc));
    }
}

void Msg_que::push(Msg_packet msg)
{
    int indext = msg.sou_fd % th_num;
    Worker_data& worker_data = workers_data[indext];
    {
        std::unique_lock<std::mutex> lock_b(worker_data.mtx_m);
        if (worker_data.shutdown) return;
        
        worker_data.buf_que.push(std::move(msg));
        worker_data.has_data_ = true;
        worker_data.cv_.notify_one();
    }
}

void Msg_que::swap(std::queue<Msg_packet>& own, std::queue<Msg_packet>& other)
{
    other.swap(own);
}

void Msg_que::handle_work(int index, std::function<void(Msg_packet msg)> handle_fuc)
{
    auto& [msg_que, buf_que, mtx_m, cv_, has_data_, shutdown] = workers_data[index];
    while (true)
    {
        std::unique_lock<std::mutex> lock_m(mtx_m);
        cv_.wait(lock_m, [&]()
        {
            return has_data_ || shutdown;
        });

        if (shutdown && !has_data_)
        {
            return;
        }

        if (has_data_.exchange(false))
        {
            swap(msg_que, buf_que);
        }
        lock_m.unlock();

        while (!msg_que.empty())
        {
            Msg_packet msg = std::move(msg_que.front());
            msg_que.pop();

            handle_fuc(msg);
        }
    }
}

void Msg_que::stop()
{
    for (Worker_data& worker_data : workers_data)
    {
        worker_data.shutdown = true;
        worker_data.cv_.notify_all();
    }
}