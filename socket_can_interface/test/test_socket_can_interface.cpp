#include "socket_can_interface.hpp"

#include <gtest/gtest.h>
#include <sstream>
#include <vector>

// Helper to capture logs
class LoggerCapture {
public:
    void operator()(const std::string& msg) {
        log << msg << "\n";
    }
    std::ostringstream log;
};

TEST(SocketCanInterfaceTest, DefaultConstruction) {
    SocketCanInterface iface("vcan0");
    EXPECT_FALSE(iface.is_debug());
}

TEST(SocketCanInterfaceTest, DebugToggle) {
    SocketCanInterface iface("vcan0", SocketCanInterface::Mode::CAN_2_0, false);
    EXPECT_FALSE(iface.is_debug());
    iface.set_debug(true);
    EXPECT_TRUE(iface.is_debug());
}

TEST(SocketCanInterfaceTest, ModeEnum) {
    EXPECT_EQ(SocketCanInterface::Mode::CAN_2_0, SocketCanInterface::Mode::CAN_2_0);
    EXPECT_NE(SocketCanInterface::Mode::CAN_FD, SocketCanInterface::Mode::CAN_2_0);
}

TEST(SocketCanInterfaceTest, LoggerIsCalled) {
    LoggerCapture logger;
    SocketCanInterface iface("vcan0", SocketCanInterface::Mode::CAN_2_0, true,
        [&logger](const std::string& msg) { logger(msg); });
    iface.open_device();
    std::vector<uint8_t> payload = {0x01, 0x02};
    iface.send_frame(0x123, payload);
    EXPECT_NE(logger.log.str().find("Sent to SocketCAN"), std::string::npos);
    iface.close_device();
}

TEST(SocketCanInterfaceTest, InvalidSendDataLength) {
    std::vector<uint8_t> oversized(100, 0xFF);  // Too large for CAN 2.0
    LoggerCapture logger;
    SocketCanInterface iface("vcan0", SocketCanInterface::Mode::CAN_2_0, true,
        [&logger](const std::string& msg) { logger(msg); });
    iface.open_device();
    auto status = iface.send_frame(0x100, oversized);
    EXPECT_EQ(status, SocketCanInterface::Status::InvalidDataLength);
    iface.close_device();
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
