# field-bus

**Shared CAN-FD protocol for MetaField physical field nodes.**

## Hardware

- **Nodes:** ESP32-S3 + **MCP2518FD** (SPI, CAN-FD)
- **Host:** DSD TECH **SH-C31G** (CAN-FD mode)

## Quick start

1. Read **[PROTOCOL.md](PROTOCOL.md)**
2. Copy `protocol/` into each ESP32 project
3. Wire MCP2518FD over SPI; CANH/CANL to the bus; terminate both ends
4. Boot → `NODE_HELLO` → heartbeat every 500 ms

## Node IDs

| ID | Role |
|----|------|
| 0x01 | Host / Coordinator |
| 0x02 | Optical Node |
| 0x03 | Sensor Node |
| 0x04 | Actuator Node |
| 0x05 | Compute Node |
| 0x06 | Expansion Node |

---

*Part of the MetaField physical-field substrate.*
