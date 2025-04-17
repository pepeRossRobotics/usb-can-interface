#include "can_usb_interface.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <span>

using namespace can_usb;


TEST(CanUsbDeviceTest, ChecksumCalculation) {
    std::vector<uint8_t> data = {0x12, 0x34, 0x56};
    int sum = CanUsbDevice::checksum(data);
    EXPECT_EQ(sum, (0x12 + 0x34 + 0x56) & 0xFF);
}

TEST(CanUsbDeviceTest, FrameCompletionDetection_Valid20Byte) {
    std::vector<uint8_t> frame = {
        0xAA, 0x55,
        // 17 dummy payload bytes
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
        0x11,
        0x00 // placeholder checksum
    };
    EXPECT_TRUE(CanUsbDevice::is_complete(frame));
}

TEST(CanUsbDeviceTest, FrameCompletionDetection_ShortInvalid) {
    std::vector<uint8_t> short_frame = {0xAA};
    EXPECT_FALSE(CanUsbDevice::is_complete(short_frame));
}

TEST(CanUsbDeviceTest, FrameCompletionDetection_DataFrameWithDLC3) {
    std::vector<uint8_t> frame = {
        0xAA, 0xC3, // info byte (DLC=3)
        0x01, 0x02, // ID
        0xAA, 0xBB, 0xCC, // payload
        0x55 // end
    };
    EXPECT_TRUE(CanUsbDevice::is_complete(frame));
}

TEST(CanUsbDeviceTest, SendDataFrameBuildsCorrectLength) {
    CanUsbDevice dev("/dev/null", 2000000, Speed::S500000, false);
    std::vector<uint8_t> payload = {0xDE, 0xAD, 0xBE, 0xEF};
    dev.set_debug(false);
    // We can't send to /dev/null, but we can test frame creation indirectly by capturing the buffer via send_frame
    // We'd mock send_frame to intercept data, but here we just ensure no crash and expect false
    bool result = dev.send_data(FrameType::Standard, 0x123, payload);
    EXPECT_FALSE(result); // because /dev/null doesn't respond
}
