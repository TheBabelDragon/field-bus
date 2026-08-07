# Field Bus Protocol v0.1

Shared CAN language for MetaField physical field nodes.

One protocol. Many ESP32s. No protocol drift.

```
┌──────────────────────────┐
│ Application              │  optical / sensor / actuator / compute
├──────────────────────────┤
│ Node API                 │
├──────────────────────────┤
│ Field Bus Protocol       │  ← this repository
├──────────────────────────┤
│ ESP32 TWAI               │
├──────────────────────────┤
│ SN65HVD230 transceiver   │  Waveshare boards (×6)
├──────────────────────────┤
│ CANH / CANL twisted pair │
└──────────────────────────┘
         ▲
         │  SH-C31G USB-CAN (host / coordinator)
```

## Hardware (current generation)

| Role | Hardware |
|------|----------|
| 6 nodes | ESP32-S3 + **Waveshare SN65HVD230** (classic CAN, 3.3 V) |
| Host PC | **DSD TECH SH-C31G** (Canable 2.0 Pro, isolated) — run in **classic CAN** mode on this bus |

**SN65HVD230 is classic CAN only.** The six-node network therefore runs **classic CAN** (ISO 11898-2, max 8 data bytes per frame). The SH-C31G supports FD after a firmware flash, but must stay in classic mode while these transceivers are on the bus.

CAN-FD remains a future option if the node transceivers are upgraded.

---

## 1. Node Identity

| ID   | Role               |
|------|--------------------|
| 0x01 | Host / Coordinator |
| 0x02 | Optical Node       |
| 0x03 | Sensor Node        |
| 0x04 | Actuator Node      |
| 0x05 | Compute Node       |
| 0x06 | Expansion Node     |
| 0x00 | Broadcast          |

Nodes announce capabilities at boot via `NODE_HELLO` — do not hard-code roles permanently.

### Capability flags

| Bit | Name        |
|-----|-------------|
| 0   | SENSOR      |
| 1   | ADC         |
| 2   | GPIO        |
| 3   | ACTUATOR    |
| 4   | OPTICAL     |
| 5   | COMPUTE     |
| 6   | STORAGE     |
| 7   | TIME_MASTER |

---

## 2. CAN Identifier Scheme (29-bit extended)

```
Bit 28 … 26   Priority     (0 = highest)
Bit 25 … 18   Message Type
Bit 17 … 10   Source Node
Bit  9 …  2   Target Node  (0x00 = broadcast)
Bit  1 …  0   Reserved
```

### Priority bands

| Prio | Class        | Examples                |
|------|--------------|-------------------------|
| 0    | Emergency    | EMERGENCY_STOP          |
| 1    | Control      | ACTUATOR_SET, GPIO_SET  |
| 2    | Time/Config  | TIME_SYNC, CONFIG_SET   |
| 3    | Sensor event | SENSOR_EVENT            |
| 4    | Telemetry    | NODE_STATUS, SENSOR_DATA|
| 5    | Discovery    | NODE_HELLO              |
| 6    | Debug        | DEBUG                   |

---

## 3. Frame layout (classic CAN, 8 data bytes)

Type, source, and target already live in the **29-bit ID**.  
The 8 data bytes are pure payload — no redundant header in the data field.

This keeps every message in a single classic frame.

### NODE_HELLO (8 bytes)

```
[0]     node_id
[1]     protocol_ver
[2..3]  firmware_ver (LE)
[4]     capabilities
[5..7]  reserved = 0
```

### NODE_STATUS (8 bytes)

```
[0]     node_id
[1]     state
[2..3]  error_flags (LE)
[4..7]  uptime_ms (LE)
```

### TIME_SYNC (8 bytes)

```
[0..3]  network_time_us (LE)
[4..5]  sync_sequence (LE)
[6..7]  reserved = 0
```

### ACK (4–8 bytes)

```
[0..1]  ref_sequence (LE)
[2]     result (0 = OK)
[3..7]  optional context
```

Richer sensor / FieldObservation data can use multiple frames later (sequence continuity) or wait for a CAN-FD transceiver upgrade.

---

## 4. Node Lifecycle

```
POWER ON → init hardware → init TWAI + SN65HVD230
        → NODE_HELLO (broadcast)
        → TIME_SYNC (from host)
        → READY
        → HEARTBEAT every ~500 ms
        → normal operation
```

Degraded mode if the host disappears: keep sensing locally, stop accepting remote actuator commands, continue heartbeating.

---

## 5. Wiring (Waveshare SN65HVD230 ↔ ESP32)

| SN65HVD230 | ESP32            | Notes                          |
|------------|------------------|--------------------------------|
| VCC        | 3.3 V            | Do **not** use 5 V             |
| GND        | GND              | Common ground                  |
| TXD (D)    | TWAI TX GPIO     | Default GPIO 1 (overridable)   |
| RXD (R)    | TWAI RX GPIO     | Default GPIO 2 (overridable)   |
| CANH       | CAN bus CANH     | Twisted pair                   |
| CANL       | CAN bus CANL     | Twisted pair                   |
| RS         | GND              | High-speed / normal mode       |

**Termination:** 120 Ω between CANH and CANL at **both physical ends** of the bus. Many Waveshare boards have an onboard terminator — enable only on the two end nodes.

**Host:** SH-C31G CANH/CANL on the same pair, classic CAN @ 500 kbit/s (match the nodes).

---

## 6. Bit rate

Default: **500 kbit/s** (good compromise for short/medium runs with SN65HVD230).  
All nodes and the SH-C31G must use the same rate.

---

## 7. Shared headers

```
protocol/
├── can_ids.h
├── can_message_types.h
├── can_protocol.h
├── can_codec.h
PROTOCOL.md
```

Every ESP32 includes the same definitions.

---

*Field Bus v0.1 — classic CAN on SN65HVD230, ready for the six-node fleet.*
