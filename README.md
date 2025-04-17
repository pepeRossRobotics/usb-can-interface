# CAN USB ↔ SocketCAN Bridge

A real-time bridge utility between USB CAN Analyzer and a SocketCAN interface written in C++20.

---

## 🔌 Features

- Bidirectional forwarding between USB-CAN (serial) and SocketCAN
- Command-line configurable
- Thread-safe, real-time friendly
- CAN 2.0 and CAN FD support

---

## 🧰 Requirements

- Linux with CAN and serial support
- `libstdc++` with C++20
- `argparse` C++ header (or replace with manual parsing)

---

## ⚙️ Build

```bash
mkdir build && cd build
cmake ..
make
```

---

## 🚀 Run

```bash
./can_bridge [OPTIONS]
```

### Options

| Option       | Description                              | Default         |
|--------------|------------------------------------------|-----------------|
| `--usb`      | USB serial device path                   | `/dev/ttyUSB0`  |
| `--iface`    | SocketCAN interface name                 | `vcan0`         |
| `--baudrate` | Serial baudrate                          | `2000000`       |
| `--speed`    | CAN speed enum (1 = 1Mbps, etc.)         | `1`             |
| `--fd`       | Enable CAN FD                            | `false`         |
| `--debug`    | Enable logging                           | `false`         |
| `--help`     | Show help message                        |                 |

---

## 📚 Example

```bash
./can_bridge --usb /dev/ttyUSB0 --iface vcan0 --debug --fd
```

This bridges messages between a USB-CAN analyzer and a virtual CAN interface.
---
