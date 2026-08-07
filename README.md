# field-bus

**Shared CAN protocol for MetaField physical field nodes.**

## Current hardware

- **6×** Waveshare **SN65HVD230** (classic CAN transceiver, 3.3 V) + ESP32 TWAI
- **1×** DSD TECH **SH-C31G** isolated USB-CAN (host) — classic mode on this bus

Classic CAN, 500 kbit/s, 29-bit IDs, 8-byte payloads. Type/source/target live in the ID.

## Quick start

1. Read **[PROTOCOL.md](PROTOCOL.md)**
2. Copy `protocol/` into each ESP32 project
3. Wire SN65HVD230: VCC→3.3V, TXD→TWAI TX, RXD→TWAI RX, RS→GND, CANH/CANL to the bus
4. Terminate the two ends of the bus with 120 Ω
5. Boot → `NODE_HELLO` → heartbeat every 500 ms

## Node IDs

| ID   | Role               |
|------|--------------------|
| 0x01 | Host / Coordinator |
| 0x02 | Optical Node       |
| 0x03 | Sensor Node        |
| 0x04 | Actuator Node      |
| 0x05 | Compute Node       |
| 0x06 | Expansion Node     |

---

*Part of the MetaField physical-field substrate.*
