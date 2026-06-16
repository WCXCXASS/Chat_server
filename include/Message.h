#ifndef _MESSAGE_BOX_H_
#define _MESSAGE_BOX_H_

#include <vector>
#include <cstdint>
#include <unistd.h>

struct message_box
{
    bool is_file;
    uint32_t len;
    std::vector<char> data;
};

class Message
{
public:
    Message();
    ~Message();

    void handle_file_data();
    void handle_chat_data();
    void handle_submit_data(sockaddr_in cli_addr);

private:
    std::vector<char> chat_data;
};

#endif