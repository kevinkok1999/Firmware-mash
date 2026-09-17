#include "mog_airtime.h"

static bool kind_valid(mog_airtime_kind_t kind) {
    return kind >= MOG_AIRTIME_DATA && kind < MOG_AIRTIME_KIND_COUNT;
}

int mog_airtime_manager_init(mog_airtime_manager_t *manager,
                             const mog_airtime_backend_ops_t *ops,
                             void *ctx,
                             size_t max_wire_len) {
    if (!manager || !ops || !ops->send || !ops->can_send || !ops->cost_ms ||
        max_wire_len == 0) {
        return MOG_AIRTIME_ERR_ARG;
    }

    manager->ops = *ops;
    manager->ctx = ctx;
    manager->max_wire_len = max_wire_len;
    manager->tx_ok = 0;
    manager->tx_denied = 0;
    manager->tx_busy = 0;
    manager->tx_radio_error = 0;
    manager->initialized = true;
    return MOG_AIRTIME_OK;
}

int mog_airtime_send(mog_airtime_manager_t *manager,
                     const uint8_t *data,
                     size_t len,
                     mog_airtime_kind_t kind) {
    if (!manager || !manager->initialized) {
        return MOG_AIRTIME_ERR_NOT_READY;
    }
    if (!data || len == 0 || !kind_valid(kind)) {
        return MOG_AIRTIME_ERR_ARG;
    }
    if (len > manager->max_wire_len) {
        return MOG_AIRTIME_ERR_TOO_LARGE;
    }

    const int rc = manager->ops.send(manager->ctx, data, len, kind);
    switch (rc) {
        case MOG_AIRTIME_BACKEND_OK:
            manager->tx_ok++;
            return MOG_AIRTIME_OK;
        case MOG_AIRTIME_BACKEND_DENIED:
            manager->tx_denied++;
            return MOG_AIRTIME_ERR_DENIED;
        case MOG_AIRTIME_BACKEND_BUSY:
            manager->tx_busy++;
            return MOG_AIRTIME_ERR_BUSY;
        case MOG_AIRTIME_BACKEND_RADIO_ERROR:
        default:
            manager->tx_radio_error++;
            return MOG_AIRTIME_ERR_RADIO;
    }
}

bool mog_airtime_can_send(mog_airtime_manager_t *manager,
                          size_t wire_len,
                          mog_airtime_kind_t kind) {
    if (!manager || !manager->initialized || wire_len == 0 ||
        wire_len > manager->max_wire_len || !kind_valid(kind)) {
        return false;
    }
    return manager->ops.can_send(manager->ctx, wire_len, kind);
}

uint32_t mog_airtime_cost_ms(mog_airtime_manager_t *manager, size_t wire_len) {
    if (!manager || !manager->initialized || wire_len == 0 ||
        wire_len > manager->max_wire_len) {
        return 0;
    }
    return manager->ops.cost_ms(manager->ctx, wire_len);
}
