#ifndef SOCKET_CAN_INTERFACE_HPP
#define SOCKET_CAN_INTERFACE_HPP

#include <string>
#include <array>
#include <vector>
#include <mutex>
#include <cstdint>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <functional>
#include <optional>
#include <span>

class SocketCanInterface {
public:
    enum class Status {
        Success,
        SocketClosed,
        WriteFailed,
        ReadFailed,
        InvalidDataLength
    };

    enum class Mode {
        CAN_2_0,
        CAN_FD
    };

    SocketCanInterface(const std::string &interface_name,
                       Mode mode = Mode::CAN_2_0,
                       bool debug = false,
                       std::function<void(const std::string&)> logger = nullptr);
    ~SocketCanInterface();

    Status open_device();
    void close_device();

    Status send_frame(uint32_t can_id, std::span<const uint8_t> data);
    std::optional<std::pair<uint32_t, std::vector<uint8_t>>> recv_frame();

    void set_debug(bool flag);
    bool is_debug() const;
    int get_fd() const;

private:
    std::string _interface_name;
    int _socket_fd;
    Mode _mode;
    bool _debug;
    std::function<void(const std::string&)> _logger;

    std::mutex _send_mutex;
    std::mutex _recv_mutex;

    void log(const std::string &message) const;
    bool poll_readable(int timeout_ms) const;
};

#endif // SOCKET_CAN_INTERFACE_HPP
