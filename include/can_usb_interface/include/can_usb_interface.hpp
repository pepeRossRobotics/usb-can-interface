#pragma once

#include <string>
#include <vector>
#include <span>
#include <functional>
#include <optional>
#include <mutex>

namespace can_usb {

enum class Speed : uint8_t {
    S1000000 = 0x01, S800000, S500000, S400000, S250000, S200000,
    S125000, S100000, S50000, S20000, S10000, S5000
};

enum class Mode : uint8_t {
    Normal = 0x00, Loopback, Silent, LoopbackSilent
};

enum class FrameType : uint8_t {
    Standard = 0x01, Extended = 0x02
};

class CanUsbDevice {
public:
    CanUsbDevice(std::string device, int baudrate = 2000000, Speed speed = Speed::S500000,
                 bool debug = false, std::function<void(const std::string&)> logger = nullptr);
    ~CanUsbDevice();

    bool open();
    void close();
    bool init();

    bool send_frame(std::span<const uint8_t> frame);
    std::optional<std::vector<uint8_t>> recv_frame();

    bool send_data(FrameType type, uint16_t id, std::span<const uint8_t> data);

    int get_fd() const;
    void set_debug(bool enable);
    bool is_debug() const;

    static int checksum(std::span<const uint8_t> data);
    static bool is_complete(const std::vector<uint8_t>& buf);

private:
    std::string device_;
    int baudrate_;
    Speed can_speed_;
    int fd_ = -1;
    bool debug_;
    std::function<void(const std::string&)> logger_;

    std::mutex send_mutex_;
    std::mutex recv_mutex_;

    void log(const std::string& msg) const;
    bool send_settings();
};

} // namespace can_usb
