#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/socket.h>

typedef enum {
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
} CANUSB_SPEED;

typedef enum {
  CANUSB_MODE_NORMAL          = 0x00,
  CANUSB_MODE_LOOPBACK        = 0x01,
  CANUSB_MODE_SILENT          = 0x02,
  CANUSB_MODE_LOOPBACK_SILENT = 0x03,
} CANUSB_MODE;

typedef enum {
  CANUSB_FRAME_STANDARD = 0x01,
  CANUSB_FRAME_EXTENDED = 0x02,
} CANUSB_FRAME;

int debug_mode = 0;

static int generate_checksum(const unsigned char *data, int data_len)
{
  int i, checksum = 0;
  for (i = 0; i < data_len; i++) {
    checksum += data[i];
  }
  return checksum & 0xff;
}

static int frame_send(int tty_fd, const unsigned char *frame, int frame_len)
{
  int result, i;
  if (debug_mode) {
    fprintf(stderr, "Sending frame: ");
    for (i = 0; i < frame_len; i++)
      fprintf(stderr, "%02x ", frame[i]);
    fprintf(stderr, "\n");
  }
  result = write(tty_fd, frame, frame_len);
  if (result == -1) {
    fprintf(stderr, "write() failed: %s\n", strerror(errno));
    return -1;
  }
  return frame_len;
}

static int command_settings(int tty_fd, CANUSB_SPEED speed, CANUSB_MODE mode, CANUSB_FRAME frame)
{
  int cmd_frame_len;
  unsigned char cmd_frame[20];

  cmd_frame_len = 0;
  cmd_frame[cmd_frame_len++] = 0xaa;
  cmd_frame[cmd_frame_len++] = 0x55;
  cmd_frame[cmd_frame_len++] = 0x12;
  cmd_frame[cmd_frame_len++] = speed;
  cmd_frame[cmd_frame_len++] = frame;
  cmd_frame[cmd_frame_len++] = 0;
  cmd_frame[cmd_frame_len++] = 0;
  cmd_frame[cmd_frame_len++] = 0;
  cmd_frame[cmd_frame_len++] = 0;
  cmd_frame[cmd_frame_len++] = 0;
  cmd_frame[cmd_frame_len++] = 0;
  cmd_frame[cmd_frame_len++] = 0;
  cmd_frame[cmd_frame_len++] = 0;
  cmd_frame[cmd_frame_len++] = mode;
  cmd_frame[cmd_frame_len++] = 0x01;
  cmd_frame[cmd_frame_len++] = 0;
  cmd_frame[cmd_frame_len++] = 0;
  cmd_frame[cmd_frame_len++] = 0;
  cmd_frame[cmd_frame_len++] = 0;
  cmd_frame[cmd_frame_len++] = generate_checksum(&cmd_frame[2], 17);

  return frame_send(tty_fd, cmd_frame, cmd_frame_len);
}

static CANUSB_SPEED canusb_int_to_speed(int speed)
{
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
    default:      return 0;
  }
}

int open_tty(const char *tty_path)
{
  int fd = open(tty_path, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) {
    perror("open_tty");
    return -1;
  }
  struct termios opts;
  if (tcgetattr(fd, &opts) < 0) {
    perror("tcgetattr");
    close(fd);
    return -1;
  }
  cfmakeraw(&opts);
  cfsetispeed(&opts, B115200);
  cfsetospeed(&opts, B115200);
  if (tcsetattr(fd, TCSANOW, &opts) < 0) {
    perror("tcsetattr");
    close(fd);
    return -1;
  }
  return fd;
}

int open_can_socket(const char *ifname)
{
  int s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (s < 0) {
    perror("open_can_socket");
    return -1;
  }
  struct ifreq ifr;
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
  if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
    perror("ioctl");
    close(s);
    return -1;
  }
  struct sockaddr_can addr = {0};
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    close(s);
    return -1;
  }
  return s;
}

// TTY protocol: 0xaa, info (lower nibble = DLC), 2 ID bytes, data bytes, terminator 0x55.
int read_tty_frame(int tty_fd, unsigned char *buf, int maxlen)
{
  int n;
  unsigned char byte;
  do {
    n = read(tty_fd, &byte, 1);
    if (n <= 0)
      return -1;
  } while (byte != 0xaa);
  buf[0] = byte;
  n = read(tty_fd, &byte, 1);
  if (n <= 0)
    return -1;
  buf[1] = byte;
  int dlc = byte & 0x0f;
  int remaining = 2 + 2 + dlc + 1;
  if (remaining > maxlen - 2)
    return -1;
  int offset = 2;
  while (remaining > 0) {
    n = read(tty_fd, &byte, 1);
    if (n <= 0)
      return -1;
    buf[offset++] = byte;
    remaining--;
  }
  if (buf[offset - 1] != 0x55)
    return -1;
  return offset;
}

int tty_frame_to_can_frame(const unsigned char *tty_frame, int len, struct can_frame *cf)
{
  if (len < 5 || tty_frame[0] != 0xaa)
    return -1;
  unsigned char info = tty_frame[1];
  int dlc = info & 0x0f;
  if (len != (5 + dlc))
    return -1;
  unsigned int id = tty_frame[2] | (tty_frame[3] << 8);
  cf->can_id = id;
  cf->can_dlc = dlc;
  memcpy(cf->data, &tty_frame[4], dlc);
  return 0;
}

int can_frame_to_tty_frame(const struct can_frame *cf, unsigned char *buf, int maxlen)
{
  int dlc = cf->can_dlc;
  int frame_len = 5 + dlc;
  if (maxlen < frame_len)
    return -1;
  buf[0] = 0xaa;
  buf[1] = 0xc0 | (dlc & 0x0f);
  buf[2] = cf->can_id & 0xff;
  buf[3] = (cf->can_id >> 8) & 0xff;
  memcpy(&buf[4], cf->data, dlc);
  buf[4 + dlc] = 0x55;
  return frame_len;
}

void print_hex(const unsigned char *data, int len)
{
  for (int i = 0; i < len; i++)
    fprintf(stderr, "%02X ", data[i]);
  fprintf(stderr, "\n");
}

int main(int argc, char *argv[])
{
  int tty_fd, can_sock;
  const char *tty_device;
  const char *can_interface;
  int opt, can_speed = 500000; // default speed
  while ((opt = getopt(argc, argv, "ds:")) != -1) {
    switch (opt) {
      case 'd':
        debug_mode = 1;
        break;
      case 's':
        can_speed = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Usage: %s [-d] [-s speed] <tty_device> <can_interface>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
  }
  if (optind + 2 != argc) {
    fprintf(stderr, "Usage: %s [-d] [-s speed] <tty_device> <can_interface>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  tty_device = argv[optind];
  can_interface = argv[optind + 1];

  tty_fd = open_tty(tty_device);
  if (tty_fd < 0)
    return EXIT_FAILURE;
  can_sock = open_can_socket(can_interface);
  if (can_sock < 0)
    return EXIT_FAILURE;

  CANUSB_SPEED speed_enum = canusb_int_to_speed(can_speed);
  if (speed_enum == 0) {
    fprintf(stderr, "Invalid CAN speed specified.\n");
    exit(EXIT_FAILURE);
  }
  if (command_settings(tty_fd, speed_enum, CANUSB_MODE_NORMAL, CANUSB_FRAME_STANDARD) < 0) {
    fprintf(stderr, "Failed to set adapter settings.\n");
    exit(EXIT_FAILURE);
  }
  if (debug_mode)
    fprintf(stderr, "Adapter initialised with speed %d bps\n", can_speed);

  int maxfd = (tty_fd > can_sock ? tty_fd : can_sock) + 1;
  while (1) {
    fd_set rset;
    FD_ZERO(&rset);
    FD_SET(tty_fd, &rset);
    FD_SET(can_sock, &rset);
    int ret = select(maxfd, &rset, NULL, NULL, NULL);
    if (ret < 0) {
      perror("select");
      break;
    }
    if (FD_ISSET(tty_fd, &rset)) {
      unsigned char tty_buf[32];
      int len = read_tty_frame(tty_fd, tty_buf, sizeof(tty_buf));
      if (len > 0) {
        if (debug_mode) {
          fprintf(stderr, "TTY read: ");
          print_hex(tty_buf, len);
        }
        struct can_frame cf;
        if (tty_frame_to_can_frame(tty_buf, len, &cf) == 0) {
          if (write(can_sock, &cf, sizeof(cf)) < 0)
            perror("write CAN");
          else if (debug_mode) {
            fprintf(stderr, "Written to CAN: ID %03X, DLC %d, data: ", cf.can_id, cf.can_dlc);
            print_hex(cf.data, cf.can_dlc);
          }
        }
      }
    }
    if (FD_ISSET(can_sock, &rset)) {
      struct can_frame cf;
      int n = read(can_sock, &cf, sizeof(cf));
      if (n > 0) {
        if (debug_mode) {
          fprintf(stderr, "CAN read: ID %03X, DLC %d, data: ", cf.can_id, cf.can_dlc);
          print_hex(cf.data, cf.can_dlc);
        }
        unsigned char tty_buf[32];
        int frame_len = can_frame_to_tty_frame(&cf, tty_buf, sizeof(tty_buf));
        if (frame_len > 0) {
          if (write(tty_fd, tty_buf, frame_len) < 0)
            perror("write TTY");
          else if (debug_mode) {
            fprintf(stderr, "Written to TTY: ");
            print_hex(tty_buf, frame_len);
          }
        }
      }
    }
  }
  close(tty_fd);
  close(can_sock);
  return EXIT_SUCCESS;
}
