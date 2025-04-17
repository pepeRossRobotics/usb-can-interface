// example/main_with_flags.cpp
#include "can_usb_interface.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <numeric>
#include <iomanip>
#include <cstring>

void print_usage() {
    std::cout << "Usage: can_usb_test [options]\n"
              << "  -d, --device <path>      USB device path (default: /dev/ttyUSB0)\n"
              << "  -b, --baudrate <rate>    Baudrate (default: 2000000)\n"
              << "  -s, --speed <enum>       CAN speed enum [1..12] (default: 1 = 1Mbps)\n"
              << "      --debug              Enable debug logging\n"
              << "      --help               Show this help message\n";
}

int main(int argc, char* argv[]) {
    using namespace can_usb;

    std::string device = "/dev/ttyUSB0";
    int baudrate = 2000000;
    Speed can_speed = Speed::S1000000;
    bool debug = false;

    for (int i = 1; i < argc; ++i) {
        if ((std::strcmp(argv[i], "--device") == 0 || std::strcmp(argv[i], "-d") == 0) && i + 1 < argc) {
            device = argv[++i];
        } else if ((std::strcmp(argv[i], "--baudrate") == 0 || std::strcmp(argv[i], "-b") == 0) && i + 1 < argc) {
            baudrate = std::stoi(argv[++i]);
        } else if ((std::strcmp(argv[i], "--speed") == 0 || std::strcmp(argv[i], "-s") == 0) && i + 1 < argc) {
            int sp = std::stoi(argv[++i]);
            can_speed = static_cast<Speed>(sp);
        } else if ((std::strcmp(argv[i], "--debug") == 0 || std::strcmp(argv[i], "--verbose") == 0)) {
            debug = true;
        } else if (std::strcmp(argv[i], "--help") == 0) {
            print_usage();
            return 0;
        }
    }

    CanUsbDevice device_obj(device, baudrate, can_speed, debug,
        [](const std::string& msg) {
            std::cerr << "[LOG] " << msg << std::endl;
        });

    if (!device_obj.open()) {
        std::cerr << "Failed to open device." << std::endl;
        return 1;
    }

    if (!device_obj.init()) {
        std::cerr << "Failed to initialize device." << std::endl;
        return 1;
    }

    std::thread reader([&device_obj]() {
        while (true) {
            auto frame = device_obj.recv_frame();
            if (frame && frame->size() >= 5 && (*frame)[0] == 0xAA) {
                const auto& buf = *frame;
                uint8_t info = buf[1];
                uint16_t id = buf[2] | (buf[3] << 8);
                uint8_t dlc = info & 0x0F;

                std::ostringstream oss;
                oss << "← Received ID=0x" << std::hex << std::uppercase << std::setw(3)
                    << std::setfill('0') << id << " [" << std::dec << int(dlc) << "] ";

                for (size_t i = 0; i < dlc && (4 + i) < buf.size(); ++i) {
                    oss << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                        << static_cast<int>(buf[4 + i]) << " ";
                }

                std::cout << oss.str() << std::endl;
            }
        }
    });

    std::vector<uint8_t> data(8);
    uint8_t counter = 0;

    while (true) {
        std::iota(data.begin(), data.end(), counter++);
        device_obj.send_data(FrameType::Standard, 0x123, data);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    reader.join();
    return 0;
}
