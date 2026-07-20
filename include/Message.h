#ifndef MESSAGE_BOX_H_
#define MESSAGE_BOX_H_

#include <map>
#include <tuple>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include "Thlog_w.h"

enum class Msg_type : uint8_t
{
    INVALID = 0,
    FILE_DATA,
    FILE_NAME,
    FILE_END_MSG,
    LOGIN_DATA,
    REGISTER_DATA,     
    CHAT_DATA_P,    
    CHAT_DATA_B,
    ERROR_RESP,

    FILE_LIST_REQ,
    FILE_LIST_RESP,
    FILE_LIST_END,
    FILE_REQUST
};

class Message_box
{
public:
    Message_box(Msg_type type);
    Message_box(std::vector<char> msg);
    ~Message_box();

    void set_data(const char* data, size_t n);
    const char* get_data();
    int get_size();

    void clear();

private:
    Msg_type msg_type = Msg_type::INVALID;
    uint32_t len = 0;
    std::vector<char> msg_box;
};

#endif