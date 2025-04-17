// #include "can_usb.hpp"
// #include "socket_can_interface.hpp"

// #include <iostream>
// #include <getopt.h>
// #include <unistd.h>
// #include <vector>
// #include <poll.h>



// void print_usage(const char *prog) {
//   std::cerr << "Usage: " << prog << " --tty /dev/ttyUSB0 --speed 500000 --iface vcan0 [--debug]\n";
// }

// int main(int argc, char **argv) {
//   std::string tty = "/dev/ttyUSB0";
//   int can_speed = 1000000;
//   std::string iface = "vcan0";
//   bool debug = false;

//   const struct option long_opts[] = {
//     {"tty", required_argument, nullptr, 't'},
//     {"speed", required_argument, nullptr, 's'},
//     {"iface", required_argument, nullptr, 'i'},
//     {"debug", no_argument, nullptr, 'd'},
//     {nullptr, 0, nullptr, 0}
//   };

//   int opt;
//   while ((opt = getopt_long(argc, argv, "t:s:i:d", long_opts, nullptr)) != -1) {
//     switch (opt) {
//       case 't': tty = optarg; break;
//       case 's': can_speed = std::stoi(optarg); break;
//       case 'i': iface = optarg; break;
//       case 'd': debug = true; break;
//       default:
//         print_usage(argv[0]);
//         return 1;
//     }
//   }

//   can_usb::CanUsbDevice usb_dev(tty, 2000000, can_speed, debug);
//   SocketCanInterface can_dev(iface, debug);

//   if (usb_dev.open_device() < 0 || !usb_dev.init_adapter()) {
//     std::cerr << "Failed to open/init USB-CAN device\n";
//     return 1;
//   }
//   if (can_dev.open_device() < 0) {
//     std::cerr << "Failed to open SocketCAN interface\n";
//     return 1;
//   }

//   struct pollfd fds[2];
//   fds[0].fd = can_dev.get_fd();
//   fds[0].events = POLLIN;
//   fds[1].fd = usb_dev.get_fd();
//   fds[1].events = POLLIN;

//   while (true) {
//     int ret = poll(fds, 2, 100);
//     if (ret < 0) {
//       perror("poll");
//       break;
//     }

//     // SocketCAN → USB
//     if (fds[0].revents & POLLIN) {
//       uint32_t id;
//       std::vector<unsigned char> data;
//       if (can_dev.recv_frame(id, data) > 0) {
//         usb_dev.send_data_frame(can_usb::CANUSB_FRAME_STANDARD, id, data);
//       }
//     }

//     // USB → SocketCAN
//     if (fds[1].revents & POLLIN) {
//       std::vector<unsigned char> usb_frame;
//       if (usb_dev.recv_frame(usb_frame) > 0) {
//         if (usb_frame.size() >= 6 && usb_frame[0] == 0xaa && ((usb_frame[1] >> 4) == 0xc)) {
//           uint32_t id = usb_frame[2] | (usb_frame[3] << 8);
//           int dlc = usb_frame[1] & 0x0F;
//           std::vector<unsigned char> payload(usb_frame.begin() + 4, usb_frame.begin() + 4 + dlc);
//           can_dev.send_frame(id, payload);
//         }
//       }
//     }
//   }

//   return 0;
// }



#include "socket_can_interface.hpp"

#include <iostream>
#include <random>
#include <thread>
#include <chrono>
#include <csignal>
#include <cstring>

std::atomic<bool> keep_running{true};

void signal_handler(int) {
    keep_running = false;
}

SocketCanInterface::Mode parse_mode(const std::string& mode_str) {
    if (mode_str == "fd" || mode_str == "FD") return SocketCanInterface::Mode::CAN_FD;
    return SocketCanInterface::Mode::CAN_2_0;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <interface> <debug: on|off> <type: 2.0|fd>\n";
        return 1;
    }

    std::string interface = argv[1];
    bool debug = (std::string(argv[2]) == "on");
    auto mode = parse_mode(argv[3]);

    SocketCanInterface can(interface, mode, debug, [](const std::string& msg) {
        std::cerr << "[DEBUG] " << msg << "\n";
    });

    if (can.open_device() != SocketCanInterface::Status::Success) {
        std::cerr << "Failed to open CAN device.\n";
        return 1;
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    std::thread reader([&can]() {
        while (keep_running) {
            auto frame = can.recv_frame();
            if (frame) {
                auto [id, data] = *frame;
                std::cout << "← Received ID=0x" << std::hex << id << " [";
                for (uint8_t byte : data) std::cout << std::hex << int(byte) << " ";
                std::cout << "]\n";
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
        auto status = can.send_frame(can_id, payload);

        if (status != SocketCanInterface::Status::Success) {
            std::cerr << "Failed to send frame.\n";
        }

        ++count;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    reader.join();
    can.close_device();
    std::cout << "Program terminated.\n";
    return 0;
}
