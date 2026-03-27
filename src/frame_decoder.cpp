#include "frame_decoder.h"

#include <cstring>
#include <arpa/inet.h>

// Constructor
// Start with an empty
// buffer
FrameDecoder::FrameDecoder() : buf_len(0) {
    memset(buffer, 0, sizeof(buffer));
}

// Clear the buffer
// called on disconnect or session reset
void FrameDecoder::reset() {
    memset(buffer, 0, sizeof(buffer));
    buf_len = 0;
}

// Available Bytes
// Returns how many bytes are currently waiting
// in the buffer.
size_t FrameDecoder::bytes_available() const {
    return buf_len;
}

// Feed
// Appends incoming bytes from recv_bytes() into the internal buffer
// Returns false if there is not enough space - caller should disconnect
bool FrameDecoder::feed(const char* data, size_t length) {
    // Check the available space before copying anything
    if (buf_len + length > BUFFER_SIZE) {
        return false;
    }

    // append new bytes
    // after the existing
    // buffered bytes
    memcpy(buffer + buf_len, data, length);
    buf_len += length;

    return true;
}

// Consume
// Removes num_bytes from the front of the buffer by shifting the
// remaining bytes to the front. Called after a complete frame has 
// been extracted.
void FrameDecoder::consume(size_t num_bytes) {
    if (num_bytes >= buf_len) {
        buf_len = 0;
        return;
    }

    // shift remaining bytes to the front
    memmove(buffer, buffer + num_bytes, buf_len - num_bytes);
    buf_len -= num_bytes;
}

// Next Frame
//
// Tries to extract one complete SoupBinTCP frame from the buffer.
//
// A complete frame requires:
//   - At least 2 bytes for the length field
//   - At least 2 + length bytes total
//
// If successful:
//   - Fills frame_out with type, payload pointer, and payload length
//   - Removes the frame bytes from the buffer via consume()
//   - Returns true
//
// If not enough bytes yet:
//   - Returns false — caller should call feed() then try again
bool FrameDecoder::next_frame(Frame& frame_out) {
    // Need at least 2-bytes to read the length field
    if (buf_len < 2) {
        return false;
    }

    // Read the 2-byte length field - stored in network byte order (big-endian)
    // ntohs() converts from network byte order to host byte order
    uint16_t raw_length;
    memcpy(&raw_length, buffer, 2);
    uint16_t frame_length = ntohs(raw_length);

    // Total bytes needed = 2 (length field) + frame_length (type + payload)
    size_t total_bytes = 2 + frame_length;

    // Check for the complete frame in the 
    // buffer
    if (buf_len < total_bytes) {
        return false;
    }

    // Extract the type byte - sits immediately after the 2-byte length field
    frame_out.type = static_cast<uint8_t>(buffer[2]);

    // Payload starts after the length field (2 bytes) and type byte (1 byte)
    // Payload length = frame_length - 1 (subtract the type byte)
    if (frame_length > 1) {
        frame_out.payload       = buffer + 3;
        frame_out.payload_len   = frame_length - 1;
    } else {
        // No payload - heartbeats and logout have frame_length == 1
        frame_out.payload = 0;
        frame_out.payload_len = 0;
    }

    // Remove the
    // consumed frame from 
    // the front of the buffer
    consume(total_bytes);

    return true;
}
