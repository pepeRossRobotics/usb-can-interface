#ifndef SOCKET_CAN_INTERFACE_HPP
#define SOCKET_CAN_INTERFACE_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <linux/can.h>

class SocketCanInterface {
public:
    SocketCanInterface(const std::string &interface_name, bool debug = false);
    ~SocketCanInterface();

    int open_device();
    void close_device();

    int send_frame(uint32_t can_id, const std::vector<unsigned char> &data);
    int recv_frame(uint32_t &can_id, std::vector<unsigned char> &data);

    void set_debug(bool flag);
    bool is_debug() const;

    int get_fd() const;

private:
    std::string _interface_name;
    int _socket_fd;
    bool _debug;
};

#endif // SOCKET_CAN_INTERFACE_HPP
