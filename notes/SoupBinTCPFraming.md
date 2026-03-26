# SoupBinTCP Framing
## What is SoupBinTCP?
SoupBinTCP is a lightweight session protocol designed by Nasdaq. It sits between TCP and your OUCH messages. Its job is three things:
1. Framing - tells the receiver exactly how many bytes each messages is
2. Session - handles login, logout, heartbeat
3. Sequencing - every server message has a sequence number so you can detect gaps

## Why do we need it?
TCP is a raw bytes stream. If you send two OUCH messages back to back:
```sh
[Enter Order 48 bytes][Cancel Order 9 bytes]
```

The receiver might get them like this:
```sh
recv() call 1 -> [Enter Order 48 bytes][Cancel Or]
recv() call 2 -> [der 9 bytes]
```
There is no boundaries. The receiver has no idea where one message ends and the next begins.

SoupBinTCP fixes this with a simple 3-part envelope around every message:

```sh
|-----------------------------------|
| 2 bytes | 1 byte | N bytes        |
| length  | type   | payload        |
|-----------------------------------|
```
The length field tells the receiver exactly how many bytes to read next. Always. No guessing.

### The Length Field
The 2-byte length field counts `type + payload` but does NOT count itself.

```sh
Total packet size on wire = 2 (length field) + length value
```

Example - a heartbeat has no payload:
```sh
length  = 1 (just the type byte)
wire    = [0x000][0x01][0x52] = 3 bytes total
```

Example - a login request (payload is 46 bytes):
```sh 
length  = 47 (1 type byte + 46 payload bytes)
wire    = [0x000][0x2F][0x2B][...46 bytes...] = 49 bytes total
```

#### Byte Order
The Length field is big-endian (network byte order). This means the most significant byte comes first.

```sh
length = 49 = 0x0031

big-endian or wire: [0x000][0x31]   ← correct
little-endian:      [0x31][0x000]   ← wrong
```
CPU(x86/ARM) stores numbers in little-endian. So we must always convert with `htons()

#### Packet Types
Client → Server (frame_encoder handles these)

| Char      | Hex       | Name              | When                                      |
| :-------- | :-------- | :---------------- | :---------------------------------------- |
| +         | 0x2B      | Login Request     | On connect, before anything else          |
| U         | 0x55      | Unsequenced Data  | Wraps every OUCH message you send         |
| R         | 0x52      | Client Heartbeat  | Every 1 second when no other packet sent  |
| O         | 0x4F      | Login Request     | When desconnecting cleanly                |

Server → Client (frame_decoder handles these)
| Char      | Hex       | Name              | When                                                  |
| :-------- | :-------- | :---------------- | :---------------------------------------------------- |
| A         | 0x41      | Login Accepted    | Server accepts login                                  |
| J         | 0x4A      | Login Rejected    | Server rejects login                                  |
| S         | 0x53      | Sequenced Data    | Wraps every OUCH response (accepted, executed etc.)   |
| H         | 0x48      | Server Heartbeat  | Server sends every 1 seconds when idle                |
| Z         | 0x5A      | End of Session    | Server is shutting down                               |

#### Packet Exmples on the Wire
Client Heartbeat (Smallest possible packet)
```sh
Byte 0: 0x000 --| length = 1
Byte 1: 0x01  --|
Byte 2: 0x52      type = 'R'

Total: 3 bytes
```

Login Request
```sh
Byte 0: 0x00 --| length = 47
Byte 1: 0x2F --|
Byte 2: 0x2B     type = '+'
Byte 3-8:   username        (6 bytes, space padded)
Byte 9-22:  password        (10 bytes, space padded)
Byte 23-32: session         (10 bytes, space padded)
Byte 33-42: sequence_number (10 bytes, ASCII digits)

Total: 49 bytes
```

Unsequenced Data (Wrapping an OUCH Enter Order)
```sh
Byte 0: 0x00  --| length = 49
Byte 1: 0x31  --|
Byte 2: 0x55      type = 'U'
Byte 3+:          OUCH Enter Order payload (48 bytes)

Total: 51 bytes
```

#### The Flow - From Order to Wire
```sh
soup_client                 frame_encoder                 socket
    |                           |                           |
    |                           |                           |
    |encode(U, ouch_bytes)      |                           |
    |-------------------------->|                           |
    |                           |                           |
    |                           | build packet:             |
    |                           | [len][U][ouch_bytes]      |
    |                           |                           |
    |                           |-------------------------->|
    |                           |                           |
    |                           |                           | bytes on wire
```

*** In Short
SoupBinTCP wraps every message with a 2-byte length and 1-byte type so the receiver always know exactly how many bytes to read - solving TCP's lack of message boundaries.
