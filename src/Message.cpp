#include "Message.h"

Message_box::Message_box (Msg_type type)
     : msg_type(type) { };

Message_box::Message_box (std::vector<char> msg)
{
    msg_box = msg;
}

Message_box::~Message_box() = default;

void Message_box::set_data(const char* data, size_t n)
{
    msg_box.resize(sizeof(uint8_t) + sizeof(uint32_t) + n);
    len = static_cast<uint32_t>(n);

    msg_box.insert(msg_box.end(), &msg_type, &msg_type + sizeof(uint8_t));
    
    uint32_t h_len = htonl(len);
    msg_box.insert(msg_box.end(), &h_len, &h_len + sizeof(uint32_t));

    msg_box.insert(msg_box.end(), data, data + n);
}

const char* Message_box::get_data()
{
    return msg_box.data();
}

int Message_box::get_size()
{
    return msg_box.size();
}

Message::Message() = default;
Message::~Message() = default;

void Message::handle_message_data(int cli_fd, std::map<std::string, int>& clients, std::vector<std::string>& files_name)
{
    std::vector<char> buffer(4096);

    while (true)
    {
        char tem[4096];
        int len = recv(cli_fd, tem, sizeof(tem), 0);
        if (len <= 0)
        {
            if (len == 0)
            {
                printf("%d finish\n", cli_fd);
                break;
            }
            perror("handle_message recv failed: ");
            break;
        }
        buffer.insert(buffer.end(), tem, tem + len);

        while (buffer.size() >= head_size)
        {
            Msg_type msg_type; // = //*((Msg_type*)buffer.data());
            uint32_t data_len; // = *((uint32_t*)(buffer.data() + sizeof(Msg_type)));
            memcpy(&msg_type, buffer.data(), sizeof(Msg_type));
            memcpy(&data_len, buffer.data() + sizeof(Msg_type), sizeof(uint32_t));
            data_len = ntohl(data_len);

            if (data_len + head_size > buffer.size()) break;

            std::vector<char> packet(buffer.begin(), buffer.begin() + (head_size + data_len));
            buffer.erase(buffer.begin(), buffer.begin() + (head_size + data_len));

            switch (msg_type)
            {
                case Msg_type::SUBMIT_DATA:
                {
                    handle_submit_data(cli_fd, clients, packet); 
                }break;

                case Msg_type::CHAT_DATA_B:
                {
                    handle_chat_broadcast_data(clients, packet);
                }break;

                case Msg_type::CHAT_DATA_P:
                {
                    handle_chat_private_data(cli_fd, clients, packet);
                }break;

                case Msg_type::FILE_NAME:
                {
                    handle_file_name(files_name, packet);
                }

                case Msg_type::FILE_DATA:
                {
                    handle_file_data(packet);
                }break;

                default:
                {
                    perror("Msy_type invailed: ");
                }break;
            }
        }
    }
}

void Message::send_error_message(int cli_fd, std::string msg)
{      
    Message_box send_msg(Msg_type::ERROR_RESP);
    send_msg.set_data(msg.data(), msg.size());
    send_all_message_data(cli_fd, send_msg);
}

void Message::send_all_message_data(int cli_fd, Message_box send_msg)
{
    int send_all_len = send_msg.get_size();
    int send_len = 0;
    while (send_len < send_all_len)
    {
        int len = send(cli_fd, send_msg.get_data() + send_len, send_all_len - send_len, 0);
        if (len <= 0)
        {
            if (len == 0) return;

            perror("send failed: ");
            break;
        }
        send_len += len;
    }
}

void Message::handle_submit_data(int cli_fd, std::map<std::string, int>& clients, std::vector<char> msg)    // Msg_typ + username\0
{
    std::string sub_name(msg.data() + sizeof(Msg_type));

    if (clients.find(sub_name) != clients.end())
    {
        send_error_message(cli_fd, "Username already exists");
        return;
    }

    clients.insert({sub_name, cli_fd});
}

void Message::handle_chat_broadcast_data(std::map<std::string, int>& clients, Message_box msg)
{
    for (auto& [cli_name, cli_fd] : clients)
    {
        send_all_message_data(cli_fd, msg);
    }
}

void Message::handle_chat_private_data(int sou_cli_fd, std::map<std::string, int>& clients, Message_box msg)   // Msg_type + data_len + desition_client_name\0 + chat_message
{
    const char* msg_data = msg.get_data();
    std::string user_name(msg_data + head_size);

    /*
    uint32_t msg_len;
    memcpy(&msg_len, msg_data + sizeof(Msg_type), sizeof(uint32_t));
    msg_len = ntohl(msg_len);
    msg_len -= user_name.size();

    const char* msg_chat = msg_data + head_size + user_name.size() + 1; // "1" -> \0
    */

    auto it = clients.find(user_name);
    if (it == clients.end())
    {
        send_error_message(sou_cli_fd, "not find the client");
        return;
    }
    int des_cli_fd = it->second;

    send_all_message_data(des_cli_fd, msg);
}

void Message::handle_file_name(std::vector<std::string>& files_name, std::vector<char> msg)   // Msg_type + data_len + name\0
{
    std::string file_name = msg.data() + head_size;
    files_name.push_back(file_name);
}

void Message::handle_file_data(std::vector<char> msg)
{
    std::string file_name;
    std::ofstream file(file_name, std::ios::binary);
    if (!file.is_open())
    {
        perror("file open failed: ");
        return;
    }
}