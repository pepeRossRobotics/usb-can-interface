#ifndef CAN_USB_HPP
#define CAN_USB_HPP

#include <string>
#include <vector>

namespace can_usb {

// Enums for speed, mode, frame and payload mode.
enum CANUSB_SPEED {
  CANUSB_SPEED_1000000 = 0x01,
  CANUSB_SPEED_800000  = 0x02,
  CANUSB_SPEED_500000  = 0x03,
  CANUSB_SPEED_400000  = 0x04,
  CANUSB_SPEED_250000  = 0x05,
  CANUSB_SPEED_200000  = 0x06,
  CANUSB_SPEED_125000  = 0x07,
  CANUSB_SPEED_100000  = 0x08,
  CANUSB_SPEED_50000   = 0x09,
  CANUSB_SPEED_20000   = 0x0a,
  CANUSB_SPEED_10000   = 0x0b,
  CANUSB_SPEED_5000    = 0x0c,
};

enum CANUSB_MODE {
  CANUSB_MODE_NORMAL          = 0x00,
  CANUSB_MODE_LOOPBACK        = 0x01,
  CANUSB_MODE_SILENT          = 0x02,
  CANUSB_MODE_LOOPBACK_SILENT = 0x03,
};

enum CANUSB_FRAME {
  CANUSB_FRAME_STANDARD = 0x01,
  CANUSB_FRAME_EXTENDED = 0x02,
};

enum CANUSB_PAYLOAD_MODE {
  CANUSB_INJECT_PAYLOAD_MODE_RANDOM      = 0,
  CANUSB_INJECT_PAYLOAD_MODE_INCREMENTAL = 1,
  CANUSB_INJECT_PAYLOAD_MODE_FIXED       = 2,
};

class CanUsbDevice {
public:
  // Constructor and destructor.
  CanUsbDevice(const std::string &tty_device, int baudrate = 2000000,
               int can_speed_bps = 500000, bool debug = false);
  ~CanUsbDevice();

  // Open and close the USB device.
  int open_device();
  void close_device();

  // Initialises the adapter (sends command settings).
  bool init_adapter();

  int get_fd() const;

  // Send and receive raw frames.
  int send_frame(const std::vector<unsigned char> &frame);
  int recv_frame(std::vector<unsigned char> &frame);

  // Send a data frame.
  int send_data_frame(CANUSB_FRAME frame_type, unsigned long id,
                      const std::vector<unsigned char> &data);

  // Dump received frames (runs an infinite loop for debugging).
  int dump_data_frames();

  // Set debug mode.
  void set_debug(bool flag);
  bool is_debug() const;

  // Utility functions.
  static int hex_value(char c);
  static unsigned long convert_hex_id(const std::string &hex_string);
  static int convert_from_hex(const std::string &hex_string,
                              std::vector<unsigned char> &bin);

private:
  int _tty_fd;
  std::string _tty_device;
  int _baudrate;
  int _can_speed_bps;
  bool _debug;

  // Helper functions.
  static int generate_checksum(const unsigned char *data, int data_len);
  static bool frame_is_complete(const unsigned char *frame, int frame_len);
  int send_command_settings();

  // Helper to convert integer speed to enum.
  static CANUSB_SPEED int_to_speed(int speed);
};

} // namespace can_usb

#endif // CAN_USB_HPP
