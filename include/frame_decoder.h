#ifndef FRAME_DECODER_H
#define FRAME_DECODER_H

#include <cstddef>
#include <cstdint>

// A decoded SoupBinTCP frame extracted from the 
// receive buffer
// Returned by FrameDecoder::next_frame()
struct Frame {
    uint8_t     type;
    const char* payload;
    size_t      payload_len;
};

// Frame Decoder
// Accumulates raw bytes from recv_bytes() into an internal buffer
// and extracts complete SoupBinTCP frames one at a time.
//
// Typical usage in the event loop:
// char recv_buf[4096];
// int n = socket.recv_bytes(recv_buf, sizeof(recv_buf));
// if (n > 0) decoder.feed(recv_buf, n);
//
// Frame frame;
// while (decoder.next_frame(frame)) {
//      session.on_frame(frame);
// }
class FrameDecoder {
public:
    static const size_t BUFFER_SIZE = 65536; // 64KB

    FrameDecoder();

    // Feed raw bytes from recv_bytes() into the internal buffer.
    // Returns false if the buffer is full (should never happen in normal use).
    bool feed(const char* data, size_t length);

    // Extracts the next complete frame from the buffer into frame_out.
    // Returns true if a coplete frame was extracted.
    // Returns false if there are not enough bytes yet - call feed() first.
    //
    // The payload pointer inside frame_out points into the internal buffer.
    // It is valid until the next call to feed() or next_frame().
    bool next_frame(Frame& frame_out);

    // Resets the buffer
    // called on dissconnect
    // or session reset
    void reset();

    // Returns how many bytes are currently waiting in the buffer.
    size_t bytes_available() const;

private:
    // Shift unprocessed bytes to the front of the buffer after extraction.
    // Keep the buffer compact so we never run out of space
    void consume(size_t num_bytes);

    char buffer[BUFFER_SIZE];   // internal receive buffer
    size_t buf_len;             // number of valid bytes currently in buffer
};

#endif
