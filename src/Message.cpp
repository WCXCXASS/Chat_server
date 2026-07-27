#include "Message.h"

Message_box::Message_box (Msg_type type)
     : msg_type(type) { };

Message_box::Message_box (std::vector<char> msg)
{
    msg_box = msg;

    memcpy(&msg_type, msg.data(), sizeof(uint8_t));
    memcpy(&len, msg.data() + sizeof(uint8_t), sizeof(uint32_t));

    len = ntohl(len);
}

Message_box::~Message_box() = default;

Msg_type Message_box::get_type()
{
    return msg_type;
}

int Message_box::data_len()
{
    return len;
}

void Message_box::set_data(const char* data, size_t n)
{
    len = static_cast<uint32_t>(n);
    msg_box.clear();

    const char* type_ptr = reinterpret_cast<const char*>(&msg_type);
    msg_box.insert(msg_box.end(), type_ptr, type_ptr + sizeof(uint8_t));

    uint32_t h_len = htonl(len);
    const char* len_ptr = reinterpret_cast<const char*>(&h_len);
    msg_box.insert(msg_box.end(), len_ptr, len_ptr + sizeof(uint32_t));

    if (n)
    {
        msg_box.insert(msg_box.end(), data, data + n);
    }
}

const char* Message_box::get_data()
{
    return msg_box.data();
}

int Message_box::get_size()
{
    return msg_box.size();
}

void Message_box::clear()
{
    msg_box.clear();
}