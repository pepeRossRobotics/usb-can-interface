#include "can_usb_interface.hpp"
#include "socket_can_interface.hpp"

#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>
#include <unordered_map>

using namespace std::chrono_literals;
std::atomic<bool> running = true;

void signal_handler(int) {
    running = false;
}

std::unordered_map<std::string, std::string> parse_args(int argc, char* argv[]) {
    std::unordered_map<std::string, std::string> args;
    for (int i = 1; i < argc; ++i) {
        if (std::strncmp(argv[i], "--", 2) == 0 && i + 1 < argc) {
            args[argv[i]] = argv[i + 1];
            ++i;
        } else if (std::strncmp(argv[i], "--debug", 7) == 0) {
            args["--debug"] = "true";
        } else if (std::strncmp(argv[i], "--fd", 4) == 0) {
            args["--fd"] = "true";
        } else if (std::strncmp(argv[i], "--help", 6) == 0) {
            args["--help"] = "true";
        }
    }
    return args;
}

void print_help() {
    std::cout << "Usage: ./can_bridge [OPTIONS]\n"
              << "Options:\n"
              << "  --usb <device>       USB device path (default: /dev/ttyUSB0)\n"
              << "  --iface <name>       SocketCAN interface (default: vcan0)\n"
              << "  --baudrate <value>   Serial baudrate (default: 2000000)\n"
              << "  --speed <enum>       CAN speed enum (default: 1)\n"
              << "  --debug              Enable debug logging\n"
              << "  --fd                 Use CAN FD\n"
              << "  --help               Show this help\n";
}

int main(int argc, char** argv) {
    auto args = parse_args(argc, argv);
    if (args.contains("--help")) {
        print_help();
        return 0;
    }

    std::string usb_dev = args.contains("--usb") ? args["--usb"] : "/dev/ttyUSB0";
    std::string iface = args.contains("--iface") ? args["--iface"] : "vcan0";
    int baudrate = args.contains("--baudrate") ? std::stoi(args["--baudrate"]) : 2000000;
    int speed_enum = args.contains("--speed") ? std::stoi(args["--speed"]) : 1;
    bool debug = args.contains("--debug");
    bool use_fd = args.contains("--fd");

    auto logger = [](const std::string& msg) { std::cerr << "[LOG] " << msg << "\n"; };

    can_usb::CanUsbDevice usb(usb_dev, baudrate, static_cast<can_usb::Speed>(speed_enum), debug, logger);
    SocketCanInterface sock(iface, use_fd ? SocketCanInterface::Mode::CAN_FD : SocketCanInterface::Mode::CAN_2_0, debug, logger);

    if (!usb.open() || !usb.init()) {
        std::cerr << "Failed to open or initialize USB CAN device." << std::endl;
        return 1;
    }

    if (sock.open_device() != SocketCanInterface::Status::Success) {
        std::cerr << "Failed to open SocketCAN interface." << std::endl;
        return 1;
    }

    signal(SIGINT, signal_handler);

    std::thread usb_to_sock([&]() {
        while (running) {
            auto frame = usb.recv_frame();
            if (frame) {
                const auto& f = *frame;
                if (f.size() >= 5) {
                    uint32_t id = f[2] | (f[3] << 8);
                    std::span<const uint8_t> data(f.data() + 4, f.size() - 5);
                    sock.send_frame(id, data);
                }
            }
        }
    });

    std::thread sock_to_usb([&]() {
        while (running) {
            auto frame = sock.recv_frame();
            if (frame) {
                std::vector<uint8_t> payload = frame->second;
                usb.send_data(can_usb::FrameType::Standard, frame->first, payload);
            }
        }
    });

    usb_to_sock.join();
    sock_to_usb.join();

    usb.close();
    sock.close_device();

    return 0;
}
