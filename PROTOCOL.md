# Field Bus Protocol v0.1

Shared **CAN-FD** language for MetaField physical field nodes.

One protocol. Many ESP32s. No protocol drift.

```
┌──────────────────────────┐
│ Application              │  optical / sensor / actuator / compute
├──────────────────────────┤
│ Node API                 │
├──────────────────────────┤
│ Field Bus Protocol       │  ← this repository (identical on every node)
├──────────────────────────┤
│ CAN-FD driver            │  (MCP2518FD / TCAN4550 / etc.)
├──────────────────────────┤
│ ESP32 + CAN-FD transceiver│
└──────────────────────────┘
```

**CAN-FD is the native mode.** Classic CAN (8-byte) is only a degraded fallback for early bring-up.

---

## 1. Node Identity

Every node has a unique 8-bit ID. Suggested assignment for a six-node system:

| ID   | Role              |
|------|-------------------|
| 0x01 | Host / Coordinator|
| 0x02 | Optical Node      |
| 0x03 | Sensor Node       |
| 0x04 | Actuator Node     |
| 0x05 | Compute Node      |
| 0x06 | Expansion Node    |
| 0x00 | Broadcast / any   |
| 0xFF | Reserved          |

**Do not hard-code role into the firmware permanently.**  
A node announces its capabilities at boot via `NODE_HELLO`.

```
NODE_HELLO
  node_id:        0x03
  protocol_ver:   1
  firmware_ver:   42
  capabilities:   SENSOR | ADC | GPIO
```

The coordinator discovers the network from these announcements.

### Capability flags (bitmask)

| Bit | Name        | Meaning                    |
|-----|-------------|----------------------------|
| 0   | SENSOR      | Produces sensor data       |
| 1   | ADC         | Has analog acquisition     |
| 2   | GPIO        | General digital I/O        |
| 3   | ACTUATOR    | Can drive outputs          |
| 4   | OPTICAL     | Laser / photodiode body    |
| 5   | COMPUTE     | Runs local inference / HMC |
| 6   | STORAGE     | Has FRAM / SD archive      |
| 7   | TIME_MASTER | Can be time source         |

---

## 2. CAN Identifier Scheme

We use **29-bit extended IDs**. The identifier itself carries routing and priority so the CAN arbitration hardware does the right thing.

```
Bit 28 … 26   Priority          (0 = highest)
Bit 25 … 18   Message Type      (see can_message_types.h)
Bit 17 … 10   Source Node ID
Bit  9 …  2   Target Node ID    (0x00 = broadcast)
Bit  1 …  0   Reserved / flags
```

### Priority bands (lower number = higher priority)

| Priority | Class              | Examples                     |
|----------|--------------------|------------------------------|
| 0        | Emergency          | EMERGENCY_STOP               |
| 1        | Actuator / Control | ACTUATOR_SET, GPIO_SET       |
| 2        | Time & Config      | TIME_SYNC, CONFIG_SET        |
| 3        | Sensor events      | SENSOR_EVENT, FIELD_OBS      |
| 4        | Telemetry          | NODE_STATUS, SENSOR_DATA     |
| 5        | Discovery          | NODE_HELLO, NODE_ANNOUNCE    |
| 6        | Debug / Log        | DEBUG_PRINT                  |
| 7        | Reserved           | —                            |

Critical traffic wins arbitration automatically.

---

## 3. Common Payload Header (CAN-FD)

Every frame carries a short, predictable header followed by type-specific payload. Designed for **CAN-FD** (up to 64 bytes total).

```c
typedef struct __attribute__((packed)) {
    uint8_t  version;     // protocol version (currently 1)
    uint8_t  type;        // message type (see can_message_types.h)
    uint8_t  source;      // source node ID
    uint8_t  target;      // target node ID (0 = broadcast)
    uint16_t sequence;    // monotonic per-source sequence number
    uint16_t length;      // payload bytes that follow
    // uint8_t payload[]; // variable, up to 56 bytes in one FD frame
} FieldBusHeader;
```

Total header = 8 bytes. Remaining bytes (up to 56 in a 64-byte FD frame) are pure payload.

This is enough for:
- Full NODE_STATUS (with temperature + supply)
- Compact FIELD_OBSERVATION summaries
- SENSOR_DATA with several channels
- ACK / NACK with context

Larger observations can still use multi-frame later if needed.

---

## 4. Node Lifecycle

Every node follows the same boot sequence:

```
POWER ON
   ↓
Initialize hardware (GPIO, ADC, lasers, FRAM …)
   ↓
Initialize CAN-FD
   ↓
Listen briefly for existing coordinator
   ↓
NODE_HELLO  (broadcast)
   ↓
Receive CONFIG / capability acknowledgement (optional)
   ↓
TIME_SYNC
   ↓
READY
   ↓
HEARTBEAT every ~500 ms
   ↓
Normal operation
```

If the coordinator disappears, the node enters a defined **degraded** state (continue local sensing if possible, stop accepting remote actuator commands, keep heartbeating).

---

## 5. Heartbeat / Node Status

Every node emits at ~500 ms:

```
NODE_STATUS
  node_id
  uptime_ms
  state          // BOOT, READY, DEGRADED, ERROR, …
  error_flags
  temperature_c  // optional, 0 if unknown
  supply_mv      // optional
```

The coordinator maintains a live table:

```
Node 0x01  ONLINE   184 ms
Node 0x02  ONLINE   203 ms
Node 0x03  ONLINE   187 ms
Node 0x04  OFFLINE    2.8 s
…
```

This is the single most useful debugging tool on a multi-node bus.

---

## 6. Commands vs Telemetry

**Commands** (coordinator → node) expect a reply:

| Command            | Reply          |
|--------------------|----------------|
| ACTUATOR_SET       | ACK / NACK     |
| GPIO_SET           | ACK / NACK     |
| CONFIG_SET         | ACK / NACK     |
| CALIBRATION_START  | ACK / NACK     |
| EMERGENCY_STOP     | ACK (best effort) |

**Telemetry / events** (node → anyone) are fire-and-forget or optionally acknowledged by higher layers:

- SENSOR_DATA
- FIELD_OBSERVATION (optical body packets)
- SENSOR_EVENT (LM393 edges, etc.)
- NODE_STATUS
- DEBUG

Transactional pattern:

```
Coordinator                Node 0x04
     │                         │
     │  ACTUATOR_SET           │
     │  seq=912  ch=2  val=731 │
     │────────────────────────►│
     │                         │
     │  ACK  seq=912           │
     │◄────────────────────────│
```

---

## 7. Time Synchronisation

First-class feature for the photonic / sensing work.

Coordinator periodically broadcasts:

```
TIME_SYNC
  network_time_us
  sync_sequence
```

Nodes timestamp measurements against this common timebase so that:

```
laser excitation
      ↓
photodiode response
      ↓
ESP32 measurement
      ↓
CAN-FD transmission
      ↓
MetaField observation
```

can be correlated without every node having an independent clock.

---

## 8. Message Type Summary

See `can_message_types.h` for the authoritative list. High-level groups:

| Range     | Group            |
|-----------|------------------|
| 0x00–0x0F | System / lifecycle |
| 0x10–0x1F | Time & config    |
| 0x20–0x2F | Commands         |
| 0x30–0x3F | Acknowledgements |
| 0x40–0x4F | Sensor / field data |
| 0x50–0x5F | Events           |
| 0xF0–0xFF | Debug / reserved |

---

## 9. Shared Headers

This repository is the single source of truth:

```
protocol/
├── can_ids.h
├── can_message_types.h
├── can_protocol.h
├── can_codec.h
└── PROTOCOL.md          (this file)
```

Every node firmware includes these files (copy, submodule, or PlatformIO lib).

---

## 10. Implementation Notes

- **CAN-FD is required** for full-sized frames (header + payload in one shot).
- ESP32 built-in TWAI is classic CAN only — use an external CAN-FD controller (MCP2518FD, TCAN4550, etc.) via SPI for production nodes.
- Sequence numbers are per-source and wrap at 16 bits.
- Target 0x00 = broadcast. Nodes ignore frames whose target is neither themselves nor 0x00 (except discovery messages).
- Keep the application layer free of CAN bit-twiddling. All packing/unpacking lives in `can_codec`.

---

*Field Bus v0.1 — the common language for the physical field substrate. CAN-FD native.*
