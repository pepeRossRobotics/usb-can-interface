#include "socket_can_interface.hpp"
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cstring>
#include <cstdint>
#include <iostream>

SocketCanInterface::SocketCanInterface(const std::string &interface_name, bool debug)
    : _interface_name(interface_name), _socket_fd(-1), _debug(debug) {}

SocketCanInterface::~SocketCanInterface() {
    close_device();
}

void SocketCanInterface::set_debug(bool flag) {
    _debug = flag;
}

bool SocketCanInterface::is_debug() const {
    return _debug;
}

int SocketCanInterface::get_fd() const {
  return _socket_fd;
}

int SocketCanInterface::open_device() {
    _socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (_socket_fd < 0) {
        perror("Socket");
        return -1;
    }

    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, _interface_name.c_str(), IFNAMSIZ);
    if (ioctl(_socket_fd, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        return -1;
    }

    struct sockaddr_can addr = {};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(_socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        return -1;
    }

    if (_debug) {
        std::cerr << "SocketCAN opened on " << _interface_name << "\n";
    }

    return _socket_fd;
}

void SocketCanInterface::close_device() {
    if (_socket_fd != -1) {
        close(_socket_fd);
        _socket_fd = -1;
    }
}

int SocketCanInterface::send_frame(uint32_t can_id, const std::vector<unsigned char> &data) {
    if (_socket_fd < 0 || data.size() > 8) return -1;

    struct can_frame frame;
    frame.can_id = can_id;
    frame.can_dlc = data.size();
    std::memcpy(frame.data, data.data(), data.size());

    int nbytes = write(_socket_fd, &frame, sizeof(frame));
    if (nbytes != sizeof(frame)) {
        perror("send_frame: write");
        return -1;
    }

    if (_debug) {
        std::cerr << "→ Sent to CAN: ID=0x" << std::hex << can_id
                  << " [" << std::dec << int(data.size()) << "]\n";
    }

    return nbytes;
}

int SocketCanInterface::recv_frame(uint32_t &can_id, std::vector<unsigned char> &data) {
    if (_socket_fd < 0) return -1;

    struct can_frame frame;
    int nbytes = read(_socket_fd, &frame, sizeof(frame));
    if (nbytes <= 0) return -1;

    can_id = frame.can_id;
    data.assign(frame.data, frame.data + frame.can_dlc);

    if (_debug) {
        std::cerr << "← Received CAN: ID=0x" << std::hex << can_id
                  << " [" << std::dec << int(frame.can_dlc) << "]\n";
    }

    return nbytes;
}
