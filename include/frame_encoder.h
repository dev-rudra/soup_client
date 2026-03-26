#ifndef FRAME_ENCODER_H
#define FRAME_ENCODER_H

#include <cstddef>
#include <cstdint>

class FrameEncoder {
public:
    
    // SoupBinTCP packet type constants
    // client --> server
    static const uint8_t TYPE_LOGIN_REQUEST     = '+';
    static const uint8_t TYPE_UNSEQUENCED_DATA  = 'U';
    static const uint8_t TYPE_CLIENT_HEARTBEAT  = 'R';
    static const uint8_t TYPE_LOGOUT_REQUEST    = 'O';

    // Login request packet.
    // Returns bytes written, or -1 if buf is too small.
    static int encode_login(char*  buf, size_t buf_len, const char* username,
                            const char* password, const char* session, const char* seq_number);

    // Unsequenced data packet
    // Wraps any outbound protocol messages (OUCH,DROP etc)
    // The encoder does not inspect the payload - it just frames it.
    static int encode_data(char* buf, size_t buf_len, const char* payload, size_t payload_len);

    // Client Heartbeat packet.
    static int encode_heartbeat(char* buf, size_t buf_len);

    // Login request packet
    // Sent when disconnecting cleanly. Has no payload.
    static int encode_logout(char* buf, size_t buf_len);

};

#endif

