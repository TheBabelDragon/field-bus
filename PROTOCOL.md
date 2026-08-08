# Field Bus Protocol v0.1

Shared **CAN-FD** language for MetaField physical field nodes.

```
┌──────────────────────────┐
│ Application              │  optical / sensor / actuator / compute
├──────────────────────────┤
│ Node API                 │
├──────────────────────────┤
│ Field Bus Protocol       │  ← this repository
├──────────────────────────┤
│ MCP2518FD (SPI)          │  CAN-FD controller + transceiver
├──────────────────────────┤
│ CANH / CANL twisted pair │
└──────────────────────────┘
         ▲
         │  DSD TECH SH-C31G (USB-CAN-FD host)
```

## Hardware (this generation)

| Role | Hardware |
|------|----------|
| Nodes (×6) | ESP32-S3 + **MCP2518FD** module (SPI, CAN-FD, up to 8 Mbps data) |
| Host PC | **DSD TECH SH-C31G** (Canable 2.0 Pro, isolated) — **CAN-FD mode** |

Classic SN65HVD230 / TWAI is not the target path.

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

Nodes announce capabilities at boot via `NODE_HELLO`.

### Capability flags

| Bit | Name |
|-----|------|
| 0 | SENSOR |
| 1 | ADC |
| 2 | GPIO |
| 3 | ACTUATOR |
| 4 | OPTICAL |
| 5 | COMPUTE |
| 6 | STORAGE |
| 7 | TIME_MASTER |

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

| Prio | Class | Examples |
|------|-------|----------|
| 0 | Emergency | EMERGENCY_STOP |
| 1 | Control | ACTUATOR_SET, GPIO_SET |
| 2 | Time/Config | TIME_SYNC, CONFIG_SET |
| 3 | Sensor event | SENSOR_EVENT, FIELD_OBS |
| 4 | Telemetry | NODE_STATUS, SENSOR_DATA |
| 5 | Discovery | NODE_HELLO |
| 6 | Debug | DEBUG |

---

## 3. Frame layout (CAN-FD, up to 64 data bytes)

Every frame starts with an 8-byte header, then type-specific payload:

```c
typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint8_t  type;
    uint8_t  source;
    uint8_t  target;
    uint16_t sequence;
    uint16_t length;      // payload bytes that follow
    // uint8_t payload[];
} FieldBusHeader;
```

Header = 8 bytes → up to 56 bytes payload in a single FD frame.

### Example payloads

**NODE_HELLO** — node_id, protocol_ver, firmware_ver, capabilities  
**NODE_STATUS** — node_id, state, error_flags, uptime_ms, temp, supply  
**TIME_SYNC** — network_time_us, sync_sequence  
**ACK** — ref_sequence, result  

---

## 4. Node Lifecycle

```
POWER ON → init hardware → init MCP2518FD (SPI)
        → NODE_HELLO (broadcast)
        → TIME_SYNC (from host)
        → READY
        → HEARTBEAT every ~500 ms
        → normal operation
```

---

## 5. Bit rates (starting point)

| Phase | Rate |
|-------|------|
| Arbitration (nominal) | 500 kbit/s |
| Data (FD) | 2 Mbit/s (raise toward 8 M later) |

All nodes + SH-C31G must match.

---

## 6. Wiring notes

**MCP2518FD ↔ ESP32 (SPI)**  
SCK, MOSI, MISO, CS, INT — pin map is board-specific; set via build flags.

**Bus**  
CANH / CANL twisted pair. 120 Ω termination at both physical ends.

**Host**  
SH-C31G on the same pair, FD mode, same bit rates.

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

---

*Field Bus v0.1 — CAN-FD on MCP2518FD + SH-C31G.*
