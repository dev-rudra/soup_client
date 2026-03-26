## What problem does frame decoder solve?
As described in the framing notes - TCP is a raw bytes stream. When the exchange sends a message to client, they arrive as a continuous flow of bytes with no natural boundraries.

Let's say the exchange sends three messages back to back:
```sh
[Order Accepted 65 bytes][Order Executed 30 bytes][Heartbeat 3 bytes]
```
`recv_bytes()` call might give them as bellow:

```sh
call 1 -> [Order Accepted 65 bytes][Order Exec]   <- 75 bytes
call 2 -> [uted 30 bytes][Heartbeat 3 bytes]      <- 23 bytes (partial!) (wait)
```
The bytes arrive in arbitary chunks, We never know how many bytes one `recv_bytes()` call will return. Could be 1 byte. Could be 500 bytes spanning multiple messages.

`frame_encoder` solves this by accumulating bytes into a buffer and extracting complete frames one at a time.

## How frame encoder works vs frame decoder
It helps to see them as exact opposites:

```sh
Frame Encoder                           Frame Decoder
Input:  type + payload                  Input: raw bytes from recv_bytes()
Output: complete packet bytes           Output: complete frames one at a time

[type][payload]                         [messy byte stream]
     ↓                                          ↓
[len][type][payload]                    [frame1][frame2][frame3]
     ↓
socket.send_bytes()
```

*** frame_encoder: is simple, we give it a message, it wraps it, done.
*** frame_decoder: is harder - bytes trickle in from the network in unpredictable chunks and we must reassemble complete frames from them.

### The Core Challenge - Three Scenarios
Every time `recv_bytes()` returns bytes, one of three things has happened:

1. Scenario 1 - Partial frame
```sh
Buffer: [00 01]         <- only got the length field, type byte missing
```
Not enough bytes yet. Must wait for more

2. Scenario 2 - Exact Frame
```sh
Buffer: [00 01 52]      <- complete heartbeat packet
```
One complete frame. Extract it, buffer is now empty.

3. Scenario 3 - Multiple frames
```sh
Buffer: [00 01 52][00 01 4F][00 06 55 48 45 4C 4C 4F]
```
Three complete frames in one `recv_bytes()` call. Must extract them one by one.

4. Scenario 4 - Frame and a half
```sh
Buffer: [00 01 52][00 06 55 48 45]    ← heartbeat complete, data packet partial
```
Extract the heartbeat, leave the partial data packet in the buffer for next time.

### How frame decoder handles this
It maintains an internal receive buffer - a fixed-size byte array that accumulates incoming bytes. The logic on every `recv_bytes()` call is:

```sh
1. Append new bytes to the end of the receive buffer

2. Loop:
   a. Do we have at least 2 bytes?  → No  → stop, wait for more
   b. Read the length field (2 bytes big-endian)
   c. Do we have 2 + length bytes?  → No  → stop, wait for more
   d. We have a complete frame — extract it
   e. Shift remaining bytes to the front of the buffer
   f. Go back to step 2
```

Visually:
```sh
recv buffer
┌──────────────────────────────────────────┐
│ 00 01 52 │ 00 06 55 48 45 4C 4C 4F │ 00 │
└──────────────────────────────────────────┘
     ▲              ▲                    ▲
  frame 1        frame 2              partial
  (complete)     (complete)           (wait)

Step 1: extract frame 1 → [00 01 52]   → on_packet('R', payload=none)
Step 2: extract frame 2 → [00 06 55 …] → on_packet('U', payload="HELLO")
Step 3: only [00] left  → not enough   → stop, wait for more bytes
```

### What frame decoder produces
When it extracts a complete frame it gives:
```sh
packet_type   →  single byte  e.g. 'S' sequenced data, 'H' heartbeat, 'A' login accepted
payload       →  pointer to the payload bytes inside the buffer
payload_len   →  number of payload bytes
```
The `session` layer then looks at `packet_type` and decides what to do:

```sh
'A'  → login was accepted    → move to active state
'J'  → login was rejected    → log error, disconnect
'H'  → server heartbeat      → reset heartbeat timer
'Z'  → end of session        → disconnect cleanly
'S'  → sequenced data        → pass payload to message_handler (OUCH response)
```

### The Receive Buffer Size
The buffer needs to be large enough to hold at least the largest possible message. The largest OUCH outbound message is Order Accepted at 65 bytes. But in practice you size it much larger — 4096 or 8192 bytes — to handle bursts of multiple messages arriving together.

### In Short
`frame_encoder` accumulates raw bytes from `recv_bytes()` into an internal buffer.
When the buffer contains a complete SoupBinTCP frame it extracts it and delivers the type and payload to the session layer.
It handles partial frames, multiple frames, and everything in between.
