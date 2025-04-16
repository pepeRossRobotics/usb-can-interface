#include "can_usb.hpp"
#include "socket_can_interface.hpp"

#include <iostream>
#include <getopt.h>
#include <unistd.h>
#include <vector>
#include <poll.h>



void print_usage(const char *prog) {
  std::cerr << "Usage: " << prog << " --tty /dev/ttyUSB0 --speed 500000 --iface vcan0 [--debug]\n";
}

int main(int argc, char **argv) {
  std::string tty = "/dev/ttyUSB0";
  int can_speed = 1000000;
  std::string iface = "vcan0";
  bool debug = false;

  const struct option long_opts[] = {
    {"tty", required_argument, nullptr, 't'},
    {"speed", required_argument, nullptr, 's'},
    {"iface", required_argument, nullptr, 'i'},
    {"debug", no_argument, nullptr, 'd'},
    {nullptr, 0, nullptr, 0}
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "t:s:i:d", long_opts, nullptr)) != -1) {
    switch (opt) {
      case 't': tty = optarg; break;
      case 's': can_speed = std::stoi(optarg); break;
      case 'i': iface = optarg; break;
      case 'd': debug = true; break;
      default:
        print_usage(argv[0]);
        return 1;
    }
  }

  can_usb::CanUsbDevice usb_dev(tty, 2000000, can_speed, debug);
  SocketCanInterface can_dev(iface, debug);

  if (usb_dev.open_device() < 0 || !usb_dev.init_adapter()) {
    std::cerr << "Failed to open/init USB-CAN device\n";
    return 1;
  }
  if (can_dev.open_device() < 0) {
    std::cerr << "Failed to open SocketCAN interface\n";
    return 1;
  }

  struct pollfd fds[2];
  fds[0].fd = can_dev.get_fd();
  fds[0].events = POLLIN;
  fds[1].fd = usb_dev.get_fd();
  fds[1].events = POLLIN;

  while (true) {
    int ret = poll(fds, 2, 100);
    if (ret < 0) {
      perror("poll");
      break;
    }

    // SocketCAN → USB
    if (fds[0].revents & POLLIN) {
      uint32_t id;
      std::vector<unsigned char> data;
      if (can_dev.recv_frame(id, data) > 0) {
        usb_dev.send_data_frame(can_usb::CANUSB_FRAME_STANDARD, id, data);
      }
    }

    // USB → SocketCAN
    if (fds[1].revents & POLLIN) {
      std::vector<unsigned char> usb_frame;
      if (usb_dev.recv_frame(usb_frame) > 0) {
        if (usb_frame.size() >= 6 && usb_frame[0] == 0xaa && ((usb_frame[1] >> 4) == 0xc)) {
          uint32_t id = usb_frame[2] | (usb_frame[3] << 8);
          int dlc = usb_frame[1] & 0x0F;
          std::vector<unsigned char> payload(usb_frame.begin() + 4, usb_frame.begin() + 4 + dlc);
          can_dev.send_frame(id, payload);
        }
      }
    }
  }

  return 0;
}
