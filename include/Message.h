#ifndef _MESSAGE_BOX_H_
#define _MESSAGE_BOX_H_

#include <map>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

enum class Msg_type : uint8_t
{
    INVALID = 0,
    FILE_DATA,
    FILE_NAME,
    SUBMIT_DATA,     
    CHAT_DATA_P,    
    CHAT_DATA_B,
    ERROR_RESP
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

private:
    Msg_type msg_type = Msg_type::INVALID;
    uint32_t len = 0;
    std::vector<char> msg_box;
};

class Message
{
public:
    Message();
    ~Message();

    void send_all_message_data(int cli_fd, Message_box send_msg);
    void send_error_message(int cli_fd, std::string msg);
    void handle_message_data(int cli_fd, std::map<std::string, int>& cliets_fd, std::vector<std::string>& files_name);
    void handle_file_data(std::vector<char> msg);
    void handle_file_name(std::vector<std::string>& files_name, std::vector<char> msg);
    void handle_chat_private_data(int sou_cli_fd, std::map<std::string, int>& clients_fd, Message_box msg);
    void handle_chat_broadcast_data(std::map<std::string, int>& clients_fd, Message_box msg);
    void handle_submit_data(int cli_fd, std::map<std::string, int>& clients_fd, std::vector<char> msg);

private:
    const int head_size = 5;    // Msg_type + uint32_t
    std::vector<char> chat_data;
};

#endif