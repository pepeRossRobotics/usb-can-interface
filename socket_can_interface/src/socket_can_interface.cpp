// socket_can_interface.cpp
#include "socket_can_interface.hpp"
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>

SocketCanInterface::SocketCanInterface(const std::string &interface_name, Mode mode,
                                       bool debug,
                                       std::function<void(const std::string&)> logger)
    : _interface_name(interface_name), _socket_fd(-1), _mode(mode), _debug(debug), _logger(std::move(logger)) {}

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

void SocketCanInterface::log(const std::string &message) const {
    if (_debug && _logger) {
        _logger(message);
    } else if (_debug) {
        std::cerr << message << std::endl;
    }
}

SocketCanInterface::Status SocketCanInterface::open_device() {
    _socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (_socket_fd < 0) {
        perror("Socket");
        return Status::SocketClosed;
    }

    if (_mode == Mode::CAN_FD) {
        int enable_canfd = 1;
        if (setsockopt(_socket_fd, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &enable_canfd, sizeof(enable_canfd)) < 0) {
            perror("setsockopt CAN FD");
            return Status::SocketClosed;
        }
    }

    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, _interface_name.c_str(), IFNAMSIZ);
    if (ioctl(_socket_fd, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        return Status::SocketClosed;
    }

    struct sockaddr_can addr = {};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(_socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Bind");
        return Status::SocketClosed;
    }

    int flags = fcntl(_socket_fd, F_GETFL, 0);
    fcntl(_socket_fd, F_SETFL, flags | O_NONBLOCK);

    log("SocketCAN opened on " + _interface_name + (_mode == Mode::CAN_FD ? " (CAN FD)" : " (CAN 2.0)"));
    return Status::Success;
}

void SocketCanInterface::close_device() {
    if (_socket_fd != -1) {
        close(_socket_fd);
        _socket_fd = -1;
    }
}

SocketCanInterface::Status SocketCanInterface::send_frame(uint32_t can_id, std::span<const uint8_t> data) {
    if (_socket_fd < 0) return Status::SocketClosed;

    std::lock_guard<std::mutex> lock(_send_mutex);

    if (_mode == Mode::CAN_FD) {
        if (data.size() > 64) return Status::InvalidDataLength;
        struct canfd_frame frame_fd = {};
        frame_fd.can_id = can_id;
        frame_fd.len = data.size();
        std::memcpy(frame_fd.data, data.data(), data.size());

        int nbytes = write(_socket_fd, &frame_fd, sizeof(frame_fd));
        if (nbytes != sizeof(frame_fd)) {
            perror("send_frame: write FD");
            return Status::WriteFailed;
        }
    } else {
        if (data.size() > 8) return Status::InvalidDataLength;
        struct can_frame frame = {};
        frame.can_id = can_id;
        frame.can_dlc = data.size();
        std::memcpy(frame.data, data.data(), data.size());

        int nbytes = write(_socket_fd, &frame, sizeof(frame));
        if (nbytes != sizeof(frame)) {
            perror("send_frame: write");
            return Status::WriteFailed;
        }
    }

    std::ostringstream oss;
    oss << "→ Sent to SocketCAN: ID=0x" << std::hex << std::uppercase << std::setw(3)
        << std::setfill('0') << can_id << " [";
    for (auto byte : data) {
        oss << std::hex << std::uppercase << std::setw(2)
            << std::setfill('0') << static_cast<int>(byte) << " ";
    }
    oss << "]";
    log(oss.str());

    return Status::Success;
}

bool SocketCanInterface::poll_readable(int timeout_ms) const {
    struct pollfd fds = { _socket_fd, POLLIN, 0 };
    return poll(&fds, 1, timeout_ms) > 0;
}

std::optional<std::pair<uint32_t, std::vector<uint8_t>>> SocketCanInterface::recv_frame() {
    if (_socket_fd < 0) return std::nullopt;
    if (!poll_readable(0)) return std::nullopt;

    std::lock_guard<std::mutex> lock(_recv_mutex);

    if (_mode == Mode::CAN_FD) {
        struct canfd_frame frame_fd;
        int nbytes = read(_socket_fd, &frame_fd, sizeof(frame_fd));
        if (nbytes <= 0) return std::nullopt;

        std::vector<uint8_t> data(frame_fd.data, frame_fd.data + frame_fd.len);
        std::ostringstream oss;
        oss << "← Received SocketCAN FD: ID=0x" << std::hex << std::uppercase << std::setw(3)
            << std::setfill('0') << frame_fd.can_id << " [";
        for (auto byte : data) {
            oss << std::hex << std::uppercase << std::setw(2)
                << std::setfill('0') << static_cast<int>(byte) << " ";
        }
        oss << "]";
        log(oss.str());
        return std::make_pair(frame_fd.can_id, data);
    } else {
        struct can_frame frame;
        int nbytes = read(_socket_fd, &frame, sizeof(frame));
        if (nbytes <= 0) return std::nullopt;

        std::vector<uint8_t> data(frame.data, frame.data + frame.can_dlc);
        std::ostringstream oss;
        oss << "← Received SocketCAN: ID=0x" << std::hex << std::uppercase << std::setw(3)
            << std::setfill('0') << frame.can_id << " [";
        for (auto byte : data) {
            oss << std::hex << std::uppercase << std::setw(2)
                << std::setfill('0') << static_cast<int>(byte) << " ";
        }
        oss << "]";
        log(oss.str());
        return std::make_pair(frame.can_id, data);
    }
}
