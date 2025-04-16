#ifndef TERMIOS2_FALLBACK_HPP
#define TERMIOS2_FALLBACK_HPP

#ifndef TCGETS2
#define TCGETS2 0x802C542A
#define TCSETS2 0x402C542B
#define BOTHER 0010000

struct termios2 {
  unsigned int c_iflag;
  unsigned int c_oflag;
  unsigned int c_cflag;
  unsigned int c_lflag;
  unsigned char c_line;
  unsigned char c_cc[19];
  unsigned int c_ispeed;
  unsigned int c_ospeed;
};
#endif

#endif // TERMIOS2_FALLBACK_HPP
