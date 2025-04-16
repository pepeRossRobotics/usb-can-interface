#include "can_usb.hpp"
#include <iostream>
#include <cstdlib>

using namespace can_usb;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0]
              << " <tty_device> [baudrate] [can_speed] [-d]" << std::endl;
    return EXIT_FAILURE;
  }
  std::string tty_device = argv[1];
  int baudrate = (argc > 2) ? std::atoi(argv[2]) : 2000000;
  int can_speed = (argc > 3) ? std::atoi(argv[3]) : 500000;
  bool debug = false;
  if (argc > 4) {
    std::string flag = argv[4];
    if (flag == "-d")
      debug = true;
  }

  CanUsbDevice device(tty_device, baudrate, can_speed, debug);
  if (device.open_device() < 0) {
    std::cerr << "Failed to open device." << std::endl;
    return EXIT_FAILURE;
  }
  if (!device.init_adapter()) {
    std::cerr << "Failed to initialise adapter." << std::endl;
    return EXIT_FAILURE;
  }
  std::cout << "Adapter initialised. Dumping data frames..." << std::endl;
  device.dump_data_frames();
  return EXIT_SUCCESS;
}
