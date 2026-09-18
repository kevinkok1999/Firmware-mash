#ifndef MOG_AIRTIME_H
#define MOG_AIRTIME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOG_AIRTIME_DATA = 0,
    MOG_AIRTIME_DATA_RETRY,
    MOG_AIRTIME_FORWARD,
    MOG_AIRTIME_STORED_FORWARD,
    MOG_AIRTIME_BEACON,
    MOG_AIRTIME_ROUTING,
    MOG_AIRTIME_ACK,
    MOG_AIRTIME_RECEIPT,
    MOG_AIRTIME_PROBE,
    MOG_AIRTIME_PROBE_REPLY,
    MOG_AIRTIME_KIND_COUNT,
} mog_airtime_kind_t;

typedef enum {
    MOG_AIRTIME_BACKEND_OK = 0,
    MOG_AIRTIME_BACKEND_RADIO_ERROR = -1,
    MOG_AIRTIME_BACKEND_DENIED = -2,
    MOG_AIRTIME_BACKEND_BUSY = -3,
} mog_airtime_backend_result_t;

typedef struct {
    int (*send)(void *ctx, const uint8_t *data, size_t len, mog_airtime_kind_t kind);
    bool (*can_send)(void *ctx, size_t wire_len, mog_airtime_kind_t kind);
    uint32_t (*cost_ms)(void *ctx, size_t wire_len);
} mog_airtime_backend_ops_t;

typedef struct {
    mog_airtime_backend_ops_t ops;
    void *ctx;
    size_t max_wire_len;
    uint32_t tx_ok;
    uint32_t tx_denied;
    uint32_t tx_busy;
    uint32_t tx_radio_error;
    bool initialized;
} mog_airtime_manager_t;

typedef enum {
    MOG_AIRTIME_OK = 0,
    MOG_AIRTIME_ERR_ARG = -10,
    MOG_AIRTIME_ERR_NOT_READY = -11,
    MOG_AIRTIME_ERR_TOO_LARGE = -12,
    MOG_AIRTIME_ERR_DENIED = -13,
    MOG_AIRTIME_ERR_BUSY = -14,
    MOG_AIRTIME_ERR_RADIO = -15,
} mog_airtime_result_t;

/*
 * Firmware-mash does not implement a second LoRa duty-cycle budget here.
 * The target backend MUST bind to the pinned foundation's single TX gate
 * (`tx_gate`) or a future explicitly approved replacement. That backend owns
 * real ToA calculation, LBT and regional duty-cycle enforcement.
 */
int mog_airtime_manager_init(mog_airtime_manager_t *manager,
                             const mog_airtime_backend_ops_t *ops,
                             void *ctx,
                             size_t max_wire_len);

int mog_airtime_send(mog_airtime_manager_t *manager,
                     const uint8_t *data,
                     size_t len,
                     mog_airtime_kind_t kind);

bool mog_airtime_can_send(mog_airtime_manager_t *manager,
                          size_t wire_len,
                          mog_airtime_kind_t kind);

uint32_t mog_airtime_cost_ms(mog_airtime_manager_t *manager, size_t wire_len);

#ifdef __cplusplus
}
#endif

#endif /* MOG_AIRTIME_H */
