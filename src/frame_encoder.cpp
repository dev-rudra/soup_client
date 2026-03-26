#include "frame_encoder.h"

#include <cstring>
#include <arpa/inet.h>

// Writes the 2-byte length and 1-byte type into buffer
// call at the start of every encode function
// length = number of bytes that follow the 2-byte length field
// (type byte + payload bytes)
static void write_header(char* buf, uint16_t length, uint8_t type) {
    uint16_t network_length = htons(length);

    // copy the 2-byte length 
    // into the first 2 bytes of the 
    // buffer
    memcpy(buf, &network_length, 2);

    // Write the type byte immediately
    // after the length
    buf[2] = static_cast<char>(type);
}

// Write padded field
// Writes a fixed-width alpha field into buffer,
// space-padded on the right.
// Used for username, password, session, seq_number in the login
// packet.
// src: the string to write (does not need to be null-terminated)
// buf: destination buffer
// width: exact number of bytes to write
static void write_padded_field(char* buf, const char* src, size_t width) {
    // Fill the entire field with space first
    memset(buf, ' ', width);

    // Copy as many bytes from src
    // as will fit, without overflowing
    size_t src_len = strlen(src);
    if (src_len > width) src_len = width;
    memcpy(buf, src, src_len);
}

// Encode Heartbeat
// Builds a client heartbeat packet - 3 bytes total:
// [0x000][0x01]['R']
// Send every 1 seconds when no other packet has been sent
int FrameEncoder::encode_heartbeat(char* buf, size_t buf_len) {
    if (buf_len < 3) return -1;

    // Length = 1
    write_header(buf, 1, TYPE_CLIENT_HEARTBEAT);

    return 3;
}

// Encode Logout
// Sent when disconnecting cleanly from the 
// Exchange
int FrameEncoder::encode_logout(char* buf, size_t buf_len) {
    // Need exactly 3 bytes: 2 length + 1 type
    if (buf_len < 3) return -1;

    write_header(buf, 1, TYPE_LOGOUT_REQUEST);

    return 3;
}

// Encode Data
// Builds an unsequenced data packet - wraps any outbound protocol message.
// Layout:
// [2 bytes: length][1 byte: 'U'][N bytes: payload]
// The encoder does not inspect the payload - it just frames it.
// payload can be an OUCH message, DROP message, or any other protocol.
int FrameEncoder::encode_data(char* buf, size_t buf_len, const char* payload, size_t payload_len) {
    // Total bytes needed: 2 (length) + 1 (type) + payload
    size_t total = 3 + payload_len;
    if (buf_len < total) return -1;

    // length field = type byte (1) + payload bytes
    write_header(buf, static_cast<uint16_t>(1 + payload_len), TYPE_UNSEQUENCED_DATA);

    // Copy payload bytes as-is - do not modify them
    memcpy(buf + 3, payload, payload_len);

    return static_cast<int>(total);
}

// Login
// Layout:
// [2 bytes: length][1 byte: '+'][6 bytes: usrename][10 bytes: password]
// [10 bytes: session][10 bytes: seq_number]
int FrameEncoder::encode_login(char* buf, size_t buf_len, const char* username,
                               const char* password, const char* session, const char* seq_number) {

    // Field widths as defined in the SoupBinTCP specification
    static const size_t USERNAME_LEN    = 6;
    static const size_t PASSWORD_LEN    = 10;
    static const size_t SESSION_LEN     = 10;
    static const size_t SEQ_NUMBER_LEN  = 10;

    // Total payload = all four fields = 36
    static const size_t PAYLOAD_LEN = USERNAME_LEN + PASSWORD_LEN + SESSION_LEN + SEQ_NUMBER_LEN;

    // Total packet = 2 (length) + 1 (type) + 36 (payload) = 39 bytes
    static const size_t TOTAL_LEN = 3 + PAYLOAD_LEN;

    if (buf_len < TOTAL_LEN) return -1;

    // Write the 2-byte length and type byte
    // Length = 1 (type) + 36 (payload) = 37
    write_header(buf, static_cast<uint16_t>(1 + PAYLOAD_LEN), TYPE_LOGIN_REQUEST);

    // Write each field at its
    // correct offset, space-padded to 
    // exact width
    size_t offset = 3;
    write_padded_field(buf + offset, username,   USERNAME_LEN);   offset += USERNAME_LEN;
    write_padded_field(buf + offset, password,   PASSWORD_LEN);   offset += PASSWORD_LEN;
    write_padded_field(buf + offset, session,    SESSION_LEN);    offset += SESSION_LEN;
    write_padded_field(buf + offset, seq_number, SEQ_NUMBER_LEN); offset += SEQ_NUMBER_LEN;

    return static_cast<int>(TOTAL_LEN);
}
