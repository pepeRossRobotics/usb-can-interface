#include "socket_can_interface.hpp"

#include <iostream>
#include <iomanip>
#include <getopt.h>
#include <random>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>
#include <sstream>

std::atomic<bool> keep_running{true};

void signal_handler(int) {
    keep_running = false;
}

SocketCanInterface::Mode parse_mode(const std::string& mode_str) {
    if (mode_str == "fd" || mode_str == "FD") return SocketCanInterface::Mode::CAN_FD;
    return SocketCanInterface::Mode::CAN_2_0;
}

bool parse_bool(const std::string& str) {
    return (str == "1" || str == "true" || str == "TRUE");
}

int main(int argc, char* argv[]) {
    std::string interface;
    bool debug = false;
    SocketCanInterface::Mode mode = SocketCanInterface::Mode::CAN_2_0;

    const struct option long_opts[] = {
        {"interface", required_argument, nullptr, 'i'},
        {"debug",     optional_argument, nullptr, 'd'},
        {"mode",      optional_argument, nullptr, 'm'},
        {nullptr,     0,                 nullptr,  0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "i:d::m::", long_opts, nullptr)) != -1) {
        switch (opt) {
            case 'i': interface = optarg; break;
            case 'd': debug = parse_bool(optarg ? optarg : "true"); break;
            case 'm': mode = parse_mode(optarg ? optarg : "2.0"); break;
            default:
                std::cerr << "Usage: " << argv[0] << " -i <interface> [--debug true|false] [--mode 2.0|fd]\n";
                return 1;
        }
    }

    if (interface.empty()) {
        std::cerr << "Error: CAN interface is required.\n";
        return 1;
    }

    auto logger = [](const std::string& msg) {
        std::cerr << "[LOG] " << msg << std::endl;
    };

    SocketCanInterface can(interface, mode, debug, logger);

    if (can.open_device() != SocketCanInterface::Status::Success) {
        std::cerr << "Failed to open CAN device.\n";
        return 1;
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::thread reader([&can, debug]() {
        while (keep_running) {
            auto frame = can.recv_frame();
            if (frame) {
                auto [id, data] = *frame;
                std::ostringstream oss;
                oss << "← Received ID=0x" << std::hex << std::uppercase << std::setw(3)
                    << std::setfill('0') << id << " [";
                for (uint8_t byte : data) {
                    oss << std::hex << std::uppercase << std::setw(2)
                        << std::setfill('0') << static_cast<int>(byte) << " ";
                }
                oss << "]";
                if (debug) std::cerr << oss.str() << "\n";
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    });

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<uint32_t> id_dist(0x100, 0x7FF);
    std::vector<uint8_t> payload(8, 0);
    size_t count = 0;

    while (keep_running) {
        for (size_t i = 0; i < payload.size(); ++i) {
            payload[i] = static_cast<uint8_t>(count + i);
        }

        uint32_t can_id = id_dist(rng);
        can.send_frame(can_id, payload);
        ++count;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    reader.join();
    can.close_device();
    return 0;
}
