#ifndef TEST_BRAMBLE_MSG_STORE_H
#define TEST_BRAMBLE_MSG_STORE_H

#include <stdint.h>

#define MSG_TEXT_MAX 640
#define MSG_ROUTE_MAX_HOPS 10

typedef enum {
    MSG_DIR_INCOMING = 0,
    MSG_DIR_OUTGOING = 1,
    MSG_DIR_BROADCAST_IN = 2,
    MSG_DIR_BROADCAST_OUT = 3,
} msg_direction_t;

typedef enum {
    MSG_STATUS_NONE = 0,
    MSG_STATUS_SENT = 1,
    MSG_STATUS_DELIVERED = 2,
    MSG_STATUS_FAILED = 3,
    MSG_STATUS_QUEUED = 4,
} msg_status_t;

typedef struct {
    uint32_t peer_addr;
    uint32_t uid;
    msg_direction_t direction;
    msg_status_t status;
    uint32_t packet_id;
    uint32_t timestamp_s;
    int8_t rssi;
    int8_t snr;
    int16_t channel_index;
    uint8_t route_hop_count;
    uint32_t route_hops[MSG_ROUTE_MAX_HOPS];
    uint16_t text_len;
    char text[MSG_TEXT_MAX];
} stored_msg_t;

#endif
