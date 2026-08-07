# field-bus

**Shared CAN / CAN-FD protocol for MetaField physical field nodes.**

One language. Every ESP32 speaks it.

```
Optical Node  ─┐
Sensor Node   ─┤
Actuator Node ─┼── all include the same protocol/ headers
Compute Node  ─┤
Host / Coord  ─┘
```

## Quick start

1. Read **[PROTOCOL.md](PROTOCOL.md)** — the full specification.
2. Copy or submodule the `protocol/` folder into each ESP32 project.
3. Include the headers:

```c
#include "can_protocol.h"
#include "can_ids.h"
#include "can_codec.h"
```

4. On every node:
   - Assign a unique `node_id`
   - Emit `NODE_HELLO` at boot
   - Emit `NODE_STATUS` every ~500 ms
   - Honour `TIME_SYNC` from the coordinator

## Layout

```
protocol/
├── can_ids.h              29-bit ID helpers + priority bands
├── can_message_types.h    FB_MSG_* constants
├── can_protocol.h         Header + payload structs, capabilities, states
├── can_codec.h            Pack / unpack helpers
PROTOCOL.md                Human-readable specification
README.md                  This file
```

## Design principles

- **Dictated, not interactive** — protocol is fixed at compile time across the fleet.
- **Priority in the CAN ID** — emergency and control traffic wins arbitration for free.
- **Discovery by announcement** — nodes declare capabilities; the coordinator does not hard-code roles.
- **Time is first-class** — required for correlating laser / photodiode / MetaField observations.
- **CAN-FD native** — classic CAN still works for the 8-byte header + small payloads.

## Node ID suggestions (6-node starter)

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
