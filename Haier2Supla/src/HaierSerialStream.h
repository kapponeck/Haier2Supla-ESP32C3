#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <HaierProtocol.h>
#include <utils/protocol_stream.h>

// Simple logging shim for migrating Serial.* calls to SUPLA_LOG
// All existing logs are treated as debug-level logs.
enum SuplaLogLevel { SUPLA_LOG_LEVEL_DEBUG = 0 };

#ifndef SUPLA_LOG
#define SUPLA_LOG(level, msg)                     \
  do {                                           \
    (void)level; /* level currently unused beyond debug */ \
    Serial.print("[DEBUG] ");                   \
    Serial.println(msg);                          \
  } while (0)
#endif

// Simple adapter between Arduino HardwareSerial and haier_protocol::ProtocolStream
class HaierSerialStream : public haier_protocol::ProtocolStream {
 public:
  explicit HaierSerialStream(HardwareSerial &serial) : serial_(serial) {}

  size_t available() noexcept override {
    return static_cast<size_t>(serial_.available());
  }

  size_t read_array(uint8_t *data, size_t len) noexcept override {
    size_t count = 0;
    while (count < len && serial_.available() > 0) {
      int b = serial_.read();
      if (b < 0) {
        break;
      }
      data[count++] = static_cast<uint8_t>(b);
    }
    return count;
  }

  void write_array(const uint8_t *data, size_t len) noexcept override {
    serial_.write(data, len);
  }

 private:
  HardwareSerial &serial_;
};
