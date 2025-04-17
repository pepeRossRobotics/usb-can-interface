#include "can_usb_interface.hpp"
#include "termios2_fallback.hpp"

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <iostream>
#include <sstream>
#include <cstring>

namespace can_usb {

CanUsbDevice::CanUsbDevice(std::string device, int baudrate, Speed speed,
                           bool debug, std::function<void(const std::string&)> logger)
    : device_(std::move(device)), baudrate_(baudrate), can_speed_(speed),
      debug_(debug), logger_(std::move(logger)) {}

CanUsbDevice::~CanUsbDevice() { close(); }

void CanUsbDevice::set_debug(bool enable) { debug_ = enable; }
bool CanUsbDevice::is_debug() const { return debug_; }
int CanUsbDevice::get_fd() const { return fd_; }

void CanUsbDevice::log(const std::string& msg) const {
    if (debug_) {
        if (logger_) logger_(msg);
        else std::cerr << msg << '\n';
    }
}

bool CanUsbDevice::open() {
    fd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ == -1) {
        perror("open");
        return false;
    }

    struct termios2 tio;
    if (ioctl(fd_, TCGETS2, &tio) < 0) {
        perror("TCGETS2");
        close();
        return false;
    }

    tio.c_cflag &= ~CBAUD;
    tio.c_cflag |= BOTHER | CS8 | CLOCAL | CREAD;
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_ispeed = baudrate_;
    tio.c_ospeed = baudrate_;

    if (ioctl(fd_, TCSETS2, &tio) < 0) {
        perror("TCSETS2");
        close();
        return false;
    }

    log("Opened USB CAN on " + device_);
    return true;
}

void CanUsbDevice::close() {
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

int CanUsbDevice::checksum(std::span<const uint8_t> data) {
    int sum = 0;
    for (auto b : data) sum += b;
    return sum & 0xFF;
}

bool CanUsbDevice::is_complete(const std::vector<uint8_t>& buf) {
    if (buf.size() < 2) return false;
    if (buf[0] != 0xAA) return true;

    uint8_t info = buf[1];

    if (info == 0x55)
        return buf.size() >= 20;

    if ((info >> 4) == 0xC) {
        size_t expected_len = 1 + 1 + 2 + (info & 0x0F) + 1;
        return buf.size() >= expected_len;
    }

    return true;
}

bool CanUsbDevice::send_frame(std::span<const uint8_t> frame) {
    std::lock_guard lock(send_mutex_);
    if (fd_ < 0 || frame.empty()) return false;

    if (::write(fd_, frame.data(), frame.size()) < 0) {
        perror("send_frame");
        return false;
    }

    std::ostringstream oss;
    oss << "→ Sent frame USBCAN: " << frame.size() << " bytes";
    log(oss.str());
    return true;
}

std::optional<std::vector<uint8_t>> CanUsbDevice::recv_frame() {
    std::lock_guard lock(recv_mutex_);
    if (fd_ < 0) return std::nullopt;

    std::vector<uint8_t> frame;
    frame.reserve(24);
    uint8_t byte;

    while (true) {
        int n = ::read(fd_, &byte, 1);
        if (n <= 0) return std::nullopt;

        frame.push_back(byte);
        if (is_complete(frame)) break;
    }

    if (frame.size() == 20 && frame[0] == 0xAA && frame[1] == 0x55) {
        int chk = checksum({frame.begin() + 2, frame.begin() + 19});
        if (chk != frame.back()) {
            log("Checksum mismatch");
            return std::nullopt;
        }
    }

    if (!frame.empty() && frame.back() != 0x55) {
        log("Frame missing stop byte");
        return std::nullopt;
    }

    std::ostringstream oss;
    oss << "← Received frame USBCAN: " << frame.size() << " bytes";
    log(oss.str());

    return frame;
}

bool CanUsbDevice::send_data(FrameType type, uint16_t id, std::span<const uint8_t> data) {
    if (data.size() > 8) return false;

    std::vector<uint8_t> frame;
    frame.reserve(13);
    frame.push_back(0xAA);
    uint8_t info = 0xC0 | (type == FrameType::Extended ? 0x20 : 0x00);
    info |= data.size() & 0x0F;
    frame.push_back(info);
    frame.push_back(id & 0xFF);
    frame.push_back((id >> 8) & 0xFF);
    frame.insert(frame.end(), data.begin(), data.end());
    frame.push_back(0x55);

    return send_frame(frame);
}

bool CanUsbDevice::send_settings() {
    std::vector<uint8_t> frame = {
        0xAA, 0x55, 0x12,
        static_cast<uint8_t>(can_speed_),
        static_cast<uint8_t>(FrameType::Standard),
        0,0,0,0, 0,0,0,0,
        static_cast<uint8_t>(Mode::Normal), 0x01,
        0,0,0,0, 0x00
    };
    int chk = checksum({frame.begin() + 2, frame.begin() + 19});
    frame.push_back(chk);
    return send_frame(frame);
}

bool CanUsbDevice::init() {
    return send_settings();
}

} // namespace can_usb
