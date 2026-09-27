// ESP32-C6 UART sniffer for Haier AC <-> WiFi dongle
// - Monitors two TX lines (forked) on RX pins
// - Logs hex bytes with timestamps to USB Serial
// - Decodes transport frames using HaierProtocol library

#include <Arduino.h>
#include <HaierProtocol.h>
#include <cstring>

using namespace haier_protocol;

// Configuration (customize as needed)
const int RX_A_PIN = 14; // line from device A (e.g., AC)
const int RX_B_PIN = 20; // line from device B (e.g., USB dongle)
const unsigned long DEBUG_BAUD = 115200; // USB serial for logs
const unsigned long LINE_BAUD = 9600; // sniffed line baud (user selected)
const bool SHOW_ONLY_CHANGED_FRAMES = true; // Only output frames when they change (avoids repeating status messages)
const bool OMIT_NETWORK_STATUS_FRAMES = true; // Skip REPORT_NETWORK_STATUS frames in output

// ESP32-C6 exposes UART0 and UART1 (no UART2).
HardwareSerial UART_A(1); // UART1
HardwareSerial UART_B(0); // UART0

// Buffers for assembling frames
const size_t MAX_FRAME = 512;
uint8_t bufA[MAX_FRAME];
size_t lenA = 0;
unsigned long lastA = 0;

uint8_t bufB[MAX_FRAME];
size_t lenB = 0;
unsigned long lastB = 0;

// Storage for detecting changed frames (when SHOW_ONLY_CHANGED_FRAMES is enabled)
uint8_t lastFrameA[MAX_FRAME];
size_t lastLenA = 0;
uint8_t lastFrameB[MAX_FRAME];
size_t lastLenB = 0;

// Storage for tracking last payload in each direction (for change highlighting)
uint8_t lastPayloadA[MAX_FRAME];
size_t lastPayloadLenA = 0;
uint8_t lastPayloadB[MAX_FRAME];
size_t lastPayloadLenB = 0;

// inter-byte timeout to flush a frame (ms)
const unsigned long FRAME_TIMEOUT_MS = 12;

class BufferProtocolStream : public ProtocolStream {
public:
	explicit BufferProtocolStream(const uint8_t *data, size_t len) : data_(data), len_(len), pos_(0) {}

	size_t available() noexcept override {
		return (pos_ < len_) ? (len_ - pos_) : 0;
	}

	size_t read_array(uint8_t *dst, size_t req_len) noexcept override {
		size_t remaining = available();
		size_t to_copy = (req_len < remaining) ? req_len : remaining;
		if (to_copy > 0) {
			memcpy(dst, data_ + pos_, to_copy);
			pos_ += to_copy;
		}
		return to_copy;
	}

	void write_array(const uint8_t * /*data*/, size_t /*len*/) noexcept override {
		// Sniffer parse stream is read-only.
	}

private:
	const uint8_t *data_;
	size_t len_;
	size_t pos_;
};

const char *frameTypeName(uint8_t type) {
	switch (static_cast<FrameType>(type)) {
		case FrameType::CONTROL: return "CONTROL";
		case FrameType::STATUS: return "STATUS";
		case FrameType::INVALID: return "INVALID";
		case FrameType::ALARM_STATUS: return "ALARM_STATUS";
		case FrameType::CONFIRM: return "CONFIRM";
		case FrameType::REPORT: return "REPORT";
		case FrameType::GET_DEVICE_VERSION: return "GET_DEVICE_VERSION";
		case FrameType::GET_DEVICE_VERSION_RESPONSE: return "GET_DEVICE_VERSION_RESPONSE";
		case FrameType::GET_DEVICE_ID: return "GET_DEVICE_ID";
		case FrameType::GET_DEVICE_ID_RESPONSE: return "GET_DEVICE_ID_RESPONSE";
		case FrameType::REPORT_NETWORK_STATUS: return "REPORT_NETWORK_STATUS";
		default: return "UNKNOWN";
	}
}

void printPayload(const uint8_t *data, size_t len) {
	Serial.print("  payload=");
	if (len == 0 || data == nullptr) {
		Serial.println("(empty)");
		return;
	}
	for (size_t i = 0; i < len; ++i) {
		if (i > 0) Serial.print(" ");
		Serial.printf("%02X", data[i]);
	}
	Serial.println();
}

void printPayloadDiff(const char *dir, const uint8_t *data, size_t len) {
	// Get reference to last payload for this direction
	uint8_t *lastPayload = nullptr;
	size_t *lastPayloadLen = nullptr;
	
	if (strcmp(dir, "A->B") == 0) {
		lastPayload = lastPayloadA;
		lastPayloadLen = &lastPayloadLenA;
	} else if (strcmp(dir, "B->A") == 0) {
		lastPayload = lastPayloadB;
		lastPayloadLen = &lastPayloadLenB;
	} else {
		return;  // Unknown direction
	}
	
	// Print diff line: show changed values, ".." for unchanged
	Serial.print("  changed=");
	size_t maxLen = (len > *lastPayloadLen) ? len : *lastPayloadLen;
	
	for (size_t i = 0; i < maxLen; ++i) {
		if (i > 0) Serial.print(" ");
		
		uint8_t oldVal = (i < *lastPayloadLen) ? lastPayload[i] : 0;
		uint8_t newVal = (i < len) ? data[i] : 0;
		
		if (i >= *lastPayloadLen || i >= len || oldVal != newVal) {
			// Changed or new/removed
			Serial.printf("%02X", newVal);
		} else {
			// Unchanged
			Serial.print("..");
		}
	}
	Serial.println();
	
	// Update stored payload
	if (len > 0) {
		memcpy(lastPayload, data, len);
	}
	*lastPayloadLen = len;
}

void printParsedFrame(const char *dir, const HaierFrame &frame) {
	const uint8_t type = frame.get_frame_type();
	
	// Filter out network status frames if configured
	if (OMIT_NETWORK_STATUS_FRAMES && type == static_cast<uint8_t>(FrameType::REPORT_NETWORK_STATUS)) {
		return;  // Skip this frame
	}
	
	const uint8_t data_size = frame.get_data_size();
	const uint8_t *data = frame.get_data();

	Serial.printf(
		"[%10lu] %s PARSED type=0x%02X (%s) data=%u crc=%s chk=0x%02X",
		millis(),
		dir,
		type,
		frameTypeName(type),
		(unsigned)data_size,
		frame.get_use_crc() ? "on" : "off",
		frame.get_checksum());

	if (frame.get_use_crc()) {
		Serial.printf(" crc16=0x%04X", frame.get_crc());
	}
	Serial.println();

	if (data_size >= 2) {
		uint16_t subcommand = (static_cast<uint16_t>(data[0]) << 8) | data[1];
		Serial.printf("  subcommand=0x%04X\n", subcommand);
	}

	printPayload(data, data_size);
	printPayloadDiff(dir, data, data_size);
}

void setup() {
	Serial.begin(DEBUG_BAUD);
	while (!Serial) {}
	delay(3000);
	Serial.println("ESP32-C6 UART sniffer starting...");

	// Initialize UART A and B as RX only (TX pin -1)
	UART_A.begin(LINE_BAUD, SERIAL_8N1, RX_A_PIN, -1);
	UART_B.begin(LINE_BAUD, SERIAL_8N1, RX_B_PIN, -1);

	Serial.printf("Listening on RX pins A=%d (UART1) B=%d (UART0) at %lu bps\n", RX_A_PIN, RX_B_PIN, LINE_BAUD);
}

// Print one byte with timestamp and direction
// void printByteLog(const char *dir, uint8_t b) {
// 	unsigned long t = millis();
// 	char buf[64];
// 	snprintf(buf, sizeof(buf), "[%10lu] %s %02X", t, dir, b);
// 	Serial.println(buf);
// }

// Hex dump for a frame
void printFrameHex(const char *dir, const uint8_t *data, size_t len) {
	Serial.printf("[%10lu] %s FRAME len=%u: ", millis(), dir, (unsigned)len);
	for (size_t i = 0; i < len; ++i) Serial.printf("%02X", data[i]);
	Serial.println();
}

void parseHaierFrames(const char *dir, const uint8_t *data, size_t len) {
	if (len == 0) return;

	BufferProtocolStream stream(data, len);
	TransportLevelHandler transport(stream, len + 16);

	bool found_any = false;
	for (;;) {
		size_t read_count = transport.read_data();
		transport.process_data();

		TimestampedFrame parsed;
		while (transport.pop(parsed)) {
			found_any = true;
			printParsedFrame(dir, parsed.frame);
		}

		if (read_count == 0) {
			break;
		}
	}

	if (!found_any) {
		Serial.printf("[%10lu] %s PARSED no valid Haier frame in chunk\n", millis(), dir);
	}
}

// Compare two frames for equality
bool framesEqual(const uint8_t *frame1, size_t len1, const uint8_t *frame2, size_t len2) {
	if (len1 != len2) return false;
	return (memcmp(frame1, frame2, len1) == 0);
}

// Update last frame buffer and return true if frame changed
bool frameChanged(uint8_t *currentFrame, size_t currentLen, uint8_t *lastFrame, size_t &lastLen) {
	bool changed = !framesEqual(currentFrame, currentLen, lastFrame, lastLen);
	if (changed) {
		memcpy(lastFrame, currentFrame, currentLen);
		lastLen = currentLen;
	}
	return changed;
}

void flushA() {
	if (lenA == 0) return;
	
	// Check if frame changed (when filter is enabled)
	if (SHOW_ONLY_CHANGED_FRAMES && !frameChanged(bufA, lenA, lastFrameA, lastLenA)) {
		lenA = 0;
		return;  // Frame hasn't changed, skip output
	}
	
	printFrameHex("A->B", bufA, lenA);
	parseHaierFrames("A->B", bufA, lenA);
	lenA = 0;
}

void flushB() {
	if (lenB == 0) return;
	
	// Check if frame changed (when filter is enabled)
	if (SHOW_ONLY_CHANGED_FRAMES && !frameChanged(bufB, lenB, lastFrameB, lastLenB)) {
		lenB = 0;
		return;  // Frame hasn't changed, skip output
	}
	
	printFrameHex("B->A", bufB, lenB);
	parseHaierFrames("B->A", bufB, lenB);
	lenB = 0;
}

void loop() {
	// Read from UART A (traffic from device A headed to B)
	while (UART_A.available()) {
		int c = UART_A.read();
		if (c < 0) break;
		if (lenA < MAX_FRAME) bufA[lenA++] = (uint8_t)c;
		lastA = millis();
		// printByteLog("A->B", (uint8_t)c);
	}

	// Read from UART B (traffic from device B headed to A)
	while (UART_B.available()) {
		int c = UART_B.read();
		if (c < 0) break;
		if (lenB < MAX_FRAME) bufB[lenB++] = (uint8_t)c;
		lastB = millis();
		// printByteLog("B->A", (uint8_t)c);
	}

	unsigned long now = millis();
	if (lenA && (now - lastA) > FRAME_TIMEOUT_MS) flushA();
	if (lenB && (now - lastB) > FRAME_TIMEOUT_MS) flushB();

	// small yield to avoid starving background tasks
	delay(1);
}

