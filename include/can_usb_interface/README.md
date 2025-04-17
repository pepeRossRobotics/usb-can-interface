# USB-CAN Analyzer Interface (C++20)

This project provides a modern C++20 wrapper around the USB-CAN Analyzer (V7.10) for Linux systems. It supports non-blocking reads/writes and is designed for integration into real-time applications.

---

## 📦 Hardware Compatibility

- **Device:** USB-CAN Analyzer V7.10
- **Link:** [USB-CAN Analyzer on Seeed Studio](https://www.seeedstudio.com/USB-CAN-Analyzer-p-2888.html)

---

## 🚀 Features

- Fully asynchronous and thread-safe
- C++20 standard
- Supports CAN Standard and Extended frames
- Command-line flags for easy testing
- Logger support for debug output

---

## 🔧 Prerequisites

- **Linux system**
- **CMake ≥ 3.16**
- **g++ with C++20 support**

Install dependencies on Debian/Ubuntu:
```bash
sudo apt update
sudo apt install build-essential cmake
```

---

## 🛠 Build Instructions

```bash
git clone https://github.com/your-repo/can_usb_interface.git
cd can_usb_interface
mkdir build && cd build
cmake ..
make
```

This builds:
- `libcan_usb_interface.a` — static library
- `can_usb_test` — example app

---

## 🧪 Usage

```bash
./can_usb_test [options]
```

### Available Options:

| Flag          | Description                        | Default           |
|---------------|------------------------------------|-------------------|
| `-d, --device`| USB serial device path             | `/dev/ttyUSB0`    |
| `-b, --baudrate` | Serial port baudrate           | `2000000`         |
| `-s, --speed` | CAN bus speed (enum 1-12)          | `1` (1 Mbps)      |
| `--debug`     | Enable debug logs                 | `false`           |
| `--help`      | Show usage                        |                   |

Example:
```bash
./can_usb_test -d /dev/ttyUSB0 -b 2000000 -s 1 --debug
```

---

## 🧰 Speed Enum Mapping

| Enum Value | Speed (bps) |
|------------|-------------|
| 1          | 1,000,000    |
| 2          | 800,000      |
| 3          | 500,000      |
| 4          | 400,000      |
| 5          | 250,000      |
| 6          | 200,000      |
| 7          | 125,000      |
| 8          | 100,000      |
| 9          | 50,000       |
| 10         | 20,000       |
| 11         | 10,000       |
| 12         | 5,000        |

---
