#pragma once

/**
 * Field Bus — common structures and constants
 */

#include <stdint.h>
#include "can_message_types.h"
#include "can_ids.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FB_PROTOCOL_VERSION   1

/* Capability bitmask (NODE_HELLO) */
#define FB_CAP_SENSOR      (1u << 0)
#define FB_CAP_ADC         (1u << 1)
#define FB_CAP_GPIO        (1u << 2)
#define FB_CAP_ACTUATOR    (1u << 3)
#define FB_CAP_OPTICAL     (1u << 4)
#define FB_CAP_COMPUTE     (1u << 5)
#define FB_CAP_STORAGE     (1u << 6)
#define FB_CAP_TIME_MASTER (1u << 7)

/* Node states (NODE_STATUS) */
#define FB_STATE_BOOT      0
#define FB_STATE_READY     1
#define FB_STATE_DEGRADED  2
#define FB_STATE_ERROR     3
#define FB_STATE_OFFLINE   4

/**
 * Every frame starts with this 8-byte header.
 * On CAN-FD the remaining bytes are payload.
 * On classic CAN you may need multi-frame for larger payloads.
 */
typedef struct __attribute__((packed)) {
    uint8_t  version;     /* FB_PROTOCOL_VERSION */
    uint8_t  type;        /* FB_MSG_* */
    uint8_t  source;
    uint8_t  target;      /* 0 = broadcast */
    uint16_t sequence;    /* per-source monotonic */
    uint16_t length;      /* payload bytes following this header */
} FieldBusHeader;

/* ---- Specific payload shapes (examples) ---- */

typedef struct __attribute__((packed)) {
    uint8_t  node_id;
    uint8_t  protocol_ver;
    uint16_t firmware_ver;
    uint8_t  capabilities;   /* FB_CAP_* bitmask */
    uint8_t  reserved[3];
} FbNodeHello;

typedef struct __attribute__((packed)) {
    uint8_t  node_id;
    uint8_t  state;          /* FB_STATE_* */
    uint16_t error_flags;
    uint32_t uptime_ms;
    int16_t  temperature_c;  /* 0.1 °C units, or 0 if unknown */
    uint16_t supply_mv;      /* millivolts, or 0 if unknown */
} FbNodeStatus;

typedef struct __attribute__((packed)) {
    uint32_t network_time_us;
    uint16_t sync_sequence;
    uint16_t reserved;
} FbTimeSync;

typedef struct __attribute__((packed)) {
    uint8_t  channel;
    uint8_t  reserved;
    int16_t  value;          /* application-defined scaling */
} FbActuatorSet;

typedef struct __attribute__((packed)) {
    uint8_t  pin;
    uint8_t  state;          /* 0/1 */
    uint16_t reserved;
} FbGpioSet;

typedef struct __attribute__((packed)) {
    uint16_t ref_sequence;   /* sequence of the command being acked */
    uint8_t  result;         /* 0 = OK, non-zero = error code */
    uint8_t  reserved;
} FbAck;

#ifdef __cplusplus
}
#endif
