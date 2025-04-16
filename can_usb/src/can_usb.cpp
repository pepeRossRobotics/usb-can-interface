#define _GNU_SOURCE
#include "termios2_fallback.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
#include <vector>

#include "can_usb.hpp"


namespace can_usb {

// --- Helper functions ---
int CanUsbDevice::generate_checksum(const unsigned char *data, int data_len) {
  int checksum = 0;
  for (int i = 0; i < data_len; i++) {
    checksum += data[i];
  }
  return checksum & 0xff;
}

bool CanUsbDevice::frame_is_complete(const unsigned char *frame, int frame_len) {
  if (frame_len > 0 && frame[0] != 0xaa)
    return true;
  if (frame_len < 2)
    return false;
  if (frame[1] == 0x55)
    return frame_len >= 20;
  else if ((frame[1] >> 4) == 0xc)
    return frame_len >= ((frame[1] & 0x0F) + 5);
  return true;
}

CANUSB_SPEED CanUsbDevice::int_to_speed(int speed) {
  switch (speed) {
    case 1000000: return CANUSB_SPEED_1000000;
    case 800000:  return CANUSB_SPEED_800000;
    case 500000:  return CANUSB_SPEED_500000;
    case 400000:  return CANUSB_SPEED_400000;
    case 250000:  return CANUSB_SPEED_250000;
    case 200000:  return CANUSB_SPEED_200000;
    case 125000:  return CANUSB_SPEED_125000;
    case 100000:  return CANUSB_SPEED_100000;
    case 50000:   return CANUSB_SPEED_50000;
    case 20000:   return CANUSB_SPEED_20000;
    case 10000:   return CANUSB_SPEED_10000;
    case 5000:    return CANUSB_SPEED_5000;
    default:      return (CANUSB_SPEED)0;
  }
}

// --- Constructor/Destructor ---
CanUsbDevice::CanUsbDevice(const std::string &tty_device, int baudrate,
                           int can_speed_bps, bool debug)
  : _tty_device(tty_device), _baudrate(baudrate),
    _can_speed_bps(can_speed_bps), _debug(debug), _tty_fd(-1) {
}

CanUsbDevice::~CanUsbDevice() {
  close_device();
}

// --- Debug ---
void CanUsbDevice::set_debug(bool flag) {
  _debug = flag;
}
bool CanUsbDevice::is_debug() const {
  return _debug;
}

// --- Open/Close device ---
int CanUsbDevice::open_device() {
  _tty_fd = open(_tty_device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (_tty_fd == -1) {
    perror("open_device: open");
    return -1;
  }
#ifdef __linux__
  struct termios2 tio;
  if (ioctl(_tty_fd, TCGETS2, &tio) == -1) {
    perror("open_device: ioctl TCGETS2");
    close(_tty_fd);
    _tty_fd = -1;
    return -1;
  }
  tio.c_cflag &= ~CBAUD;
  tio.c_cflag = BOTHER | CS8 | CSTOPB;
  tio.c_iflag = IGNPAR;
  tio.c_oflag = 0;
  tio.c_lflag = 0;
  tio.c_ispeed = _baudrate;
  tio.c_ospeed = _baudrate;
  if (ioctl(_tty_fd, TCSETS2, &tio) == -1) {
    perror("open_device: ioctl TCSETS2");
    close(_tty_fd);
    _tty_fd = -1;
    return -1;
  }
#endif
  if (_debug)
    fprintf(stderr, "Opened device %s at baudrate %d\n", _tty_device.c_str(), _baudrate);
  return _tty_fd;
}

void CanUsbDevice::close_device() {
  if (_tty_fd != -1) {
    close(_tty_fd);
    _tty_fd = -1;
  }
}

// --- Frame send/receive ---
int CanUsbDevice::send_frame(const std::vector<unsigned char> &frame) {
  if (_tty_fd == -1)
    return -1;
  if (_debug) {
    fprintf(stderr, "Sending frame: ");
    for (size_t i = 0; i < frame.size(); i++)
      fprintf(stderr, "%02X ", frame[i]);
    fprintf(stderr, "\n");
  }
  int res = write(_tty_fd, frame.data(), frame.size());
  if (res == -1)
    perror("send_frame: write");
  return res;
}

int CanUsbDevice::recv_frame(std::vector<unsigned char> &frame) {
  if (_tty_fd == -1)
    return -1;
  unsigned char byte;
  frame.clear();
  while (true) {
    int n = read(_tty_fd, &byte, 1);
    if (n <= 0)
      return -1;
    frame.push_back(byte);
    if (frame_is_complete(frame.data(), frame.size()))
      break;
    usleep(10);
  }
  if (_debug) {
    fprintf(stderr, "Received frame: ");
    for (auto b : frame)
      fprintf(stderr, "%02X ", b);
    fprintf(stderr, "\n");
  }
  if (frame.size() == 20 && frame[0] == 0xaa && frame[1] == 0x55) {
    int chksum = generate_checksum(&frame[2], 17);
    if (chksum != frame.back()) {
      fprintf(stderr, "recv_frame: Checksum incorrect\n");
      return -1;
    }
  }
  return frame.size();
}

// --- Send data frame ---
int CanUsbDevice::send_data_frame(CANUSB_FRAME frame_type, unsigned long id,
                                  const std::vector<unsigned char> &data) {
  if (data.size() > 8) {
    fprintf(stderr, "send_data_frame: DLC must be <= 8\n");
    return -1;
  }
  std::vector<unsigned char> frame;
  frame.push_back(0xaa);
  unsigned char info = 0;
  info |= 0xC0; // Bits 7 and 6 fixed.
  if (frame_type == CANUSB_FRAME_STANDARD)
    info &= 0xDF; // Standard frame.
  else
    info |= 0x20; // Extended frame.
  info &= 0xEF;    // Data frame.
  info |= static_cast<unsigned char>(data.size() & 0x0F);
  frame.push_back(info);
  frame.push_back(id & 0xFF);
  frame.push_back((id >> 8) & 0xFF);
  for (auto b : data)
    frame.push_back(b);
  frame.push_back(0x55);
  return send_frame(frame);
}

// --- Command settings ---
int CanUsbDevice::send_command_settings() {
  std::vector<unsigned char> frame;
  frame.push_back(0xaa);
  frame.push_back(0x55);
  frame.push_back(0x12);
  CANUSB_SPEED speed_val = int_to_speed(_can_speed_bps);
  frame.push_back(static_cast<unsigned char>(speed_val));
  frame.push_back(static_cast<unsigned char>(CANUSB_FRAME_STANDARD));
  for (int i = 0; i < 4; i++)
    frame.push_back(0x00);
  for (int i = 0; i < 4; i++)
    frame.push_back(0x00);
  frame.push_back(static_cast<unsigned char>(CANUSB_MODE_NORMAL));
  frame.push_back(0x01);
  for (int i = 0; i < 4; i++)
    frame.push_back(0x00);
  frame.push_back(0x00);
  int checksum = generate_checksum(frame.data() + 2, 17);
  frame.push_back(static_cast<unsigned char>(checksum));
  return send_frame(frame);
}

bool CanUsbDevice::init_adapter() {
  return send_command_settings() >= 0;
}

// --- Dump data frames ---
int CanUsbDevice::dump_data_frames() {
  std::vector<unsigned char> frame;
  while (true) {
    int len = recv_frame(frame);
    if (len > 0) {
      if (frame.size() >= 6 && frame[0] == 0xaa && ((frame[1] >> 4) == 0xc)) {
        unsigned int id = frame[2] | (frame[3] << 8);
        int dlc = frame[1] & 0x0F;
        printf("can0  %08X   [%d]  ", id, dlc);
        for (int i = 0; i < dlc; i++)
          printf("%02X ", frame[4 + i]);
        printf("\n");
      } else {
        printf("Unknown frame: ");
        for (auto b : frame)
          printf("%02X ", b);
        printf("\n");
      }
    }
    usleep(100);
  }
  return 0;
}

// --- Utility functions ---
int CanUsbDevice::hex_value(char c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  else if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;
  else if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;
  else
    return -1;
}

unsigned long CanUsbDevice::convert_hex_id(const std::string &hex_string) {
  unsigned long result = 0;
  for (char c : hex_string) {
    int val = hex_value(c);
    if (val >= 0)
      result = (result << 4) | val;
  }
  return result;
}

int CanUsbDevice::get_fd() const {
  return _tty_fd;
}

int CanUsbDevice::convert_from_hex(const std::string &hex_string,
                                   std::vector<unsigned char> &bin) {
  bin.clear();
  int high = -1;
  for (char c : hex_string) {
    int val = hex_value(c);
    if (val >= 0) {
      if (high == -1) {
        high = c;
      } else {
        int high_val = hex_value(high);
        bin.push_back(static_cast<unsigned char>(high_val * 16 + val));
        high = -1;
      }
    }
  }
  return bin.size();
}

} // namespace can_usb
