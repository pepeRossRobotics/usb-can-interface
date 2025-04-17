# SocketCAN Interface Library

This project provides a robust and efficient C++ interface for working with SocketCAN on Linux, supporting both CAN 2.0 and CAN FD modes. It is suitable for real-time systems and includes a unit test suite using GoogleTest.

---

## 📦 Features
- Non-blocking SocketCAN communication
- Thread-safe send and receive operations
- Configurable debug logging
- CAN 2.0 and CAN FD support
- Unit-tested with `vcan0` loopback

---

## 🛠️ Requirements

### System Dependencies
- Linux with SocketCAN support
- CMake >= 3.16
- GCC >= 9 (with C++20 support)

### Packages
```bash
sudo apt update
sudo apt install -y build-essential cmake libgtest-dev libpthread-stubs0-dev
```

> **Note**: `libgtest-dev` only provides source. Build and install manually:
```bash
cd /usr/src/gtest
sudo cmake .
sudo make
sudo cp lib/*.a /usr/lib
```

### Virtual CAN Setup (for testing)
```bash
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
```

---

## 🧪 Building & Testing

```bash
git clone <repo-url>
cd socket_can_interface
mkdir build && cd build
cmake ..
make
ctest  # or ./socket_can_tests
```

---

## 🚀 Usage

### Command Line Tool: `can_test`
```bash
./can_test -i <interface> [--debug true|false] [--mode 2.0|fd]
```

#### Example
```bash
./can_test -i vcan0 --debug true --mode fd
```
- Sends an incremental payload every second
- Random CAN ID
- Logs received frames if debug is enabled

---

## 🧪 Example Unit Tests
```bash
./socket_can_tests
```
- Tests construction, debug toggling, logger output, and invalid data handling
- Uses `vcan0` for loopback

---

## 🧹 Cleaning
```bash
rm -rf build
```

---

## 📁 Project Structure
```
├── include/              # Public headers
├── src/                  # Implementation
├── example/              # Command-line tool
├── test/                 # GoogleTest-based unit tests
└── CMakeLists.txt
```

---

## 🧩 Extending
You can extend this project to:
- Use epoll/select for async IO
- Add ring buffer logging
