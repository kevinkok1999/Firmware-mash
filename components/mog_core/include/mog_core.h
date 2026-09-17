#ifndef MOG_CORE_H
#define MOG_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t mog_node_id_t;
typedef uint64_t mog_packet_id_t;

typedef enum {
    MOG_LINK_LORA = 1,
    MOG_LINK_ESPNOW,
    MOG_LINK_IP,
    MOG_LINK_NAN,
    MOG_LINK_BACKSCATTER,
} mog_link_type_t;

typedef struct {
    mog_node_id_t origin;
    mog_packet_id_t packet_id;
} mog_message_key_t;

typedef enum {
    MOG_MSG_CREATED = 0,
    MOG_MSG_READY,
    MOG_MSG_SENDING,
    MOG_MSG_WAITING_ACK,
    MOG_MSG_WAITING_ROUTE,
    MOG_MSG_DEFERRED,
    MOG_MSG_DELIVERED,
    MOG_MSG_EXPIRED,
    MOG_MSG_FAILED_PERMANENT,
} mog_message_state_t;

typedef enum {
    MOG_CORE_OK = 0,
    MOG_CORE_ERR_ARG = -1,
    MOG_CORE_ERR_STORE = -2,
    MOG_CORE_ERR_EXHAUSTED = -3,
    MOG_CORE_ERR_STATE = -4,
} mog_core_result_t;

typedef int (*mog_counter_read_fn)(uint64_t *ceiling_out, void *ctx);
typedef int (*mog_counter_write_fn)(uint64_t ceiling, void *ctx);

typedef struct {
    uint64_t next;
    uint64_t durable_ceiling;
    uint64_t reserve;
    mog_counter_read_fn read;
    mog_counter_write_fn write;
    void *ctx;
    bool ready;
} mog_packet_id_generator_t;

int mog_packet_id_generator_init(mog_packet_id_generator_t *gen,
                                 uint64_t reserve,
                                 mog_counter_read_fn read_fn,
                                 mog_counter_write_fn write_fn,
                                 void *ctx);

int mog_packet_id_next(mog_packet_id_generator_t *gen, mog_packet_id_t *out);

bool mog_message_key_is_valid(mog_message_key_t key);
bool mog_message_key_equal(mog_message_key_t a, mog_message_key_t b);

bool mog_message_state_is_terminal(mog_message_state_t state);
bool mog_message_state_can_transition(mog_message_state_t from,
                                      mog_message_state_t to);

uint32_t mog_time_elapsed32(uint32_t now, uint32_t then);
bool mog_time_reached32(uint32_t now, uint32_t deadline);

#ifdef __cplusplus
}
#endif

#endif /* MOG_CORE_H */
