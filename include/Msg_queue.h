#ifndef MSG_QUEUE_H_
#define MSG_QUEUE_H_

#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>

struct Msg_packet
{
    int sou_fd;
    std::vector<char> msg;
};

class Msg_que
{
public:
    Msg_que(int num);
    ~Msg_que();

    void start(std::function<void(Msg_packet)> handle_fuc);

    void push(Msg_packet msg);
    void swap();
    void handle_work(std::function<void(Msg_packet)> handle_fuc);

    void stop();

private:
    std::queue<Msg_packet> buf_que;
    std::queue<Msg_packet> msg_que;
    std::condition_variable cv_;
    std::atomic<bool> has_data_ = false;
    std::mutex mtx_m;
    std::mutex mtx_b;

    int th_num = 0;
    std::vector<std::thread> workers;
    std::atomic<bool> shoutdown = false;
};

#endif