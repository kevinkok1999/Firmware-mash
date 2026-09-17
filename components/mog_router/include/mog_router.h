#ifndef MOG_ROUTER_H
#define MOG_ROUTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mog_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t path_id;
    mog_node_id_t destination;
    mog_node_id_t next_hop;
    mog_link_type_t first_link;
    uint8_t hop_count;
    uint16_t score;
    uint16_t confidence;
    bool valid;
} mog_route_candidate_t;

typedef struct {
    mog_route_candidate_t *storage;
    size_t capacity;
    size_t count;
    size_t max_paths_per_destination;
    uint16_t switch_margin;
} mog_hybrid_router_t;

typedef enum {
    MOG_ROUTER_OK = 0,
    MOG_ROUTER_ERR_ARG = -1,
    MOG_ROUTER_ERR_FULL = -2,
    MOG_ROUTER_ERR_CONFLICT = -3,
    MOG_ROUTER_ERR_NOT_FOUND = -4,
} mog_router_result_t;

/*
 * This is the one logical route-table authority. Transport adapters and
 * gateway managers may offer candidates, but only this component installs,
 * invalidates and selects them.
 *
 * `score` is intentionally an input. Firmware-mash does not freeze an
 * unvalidated route-score formula here; simulator/field evidence will own
 * that later policy. `switch_margin` only prevents route flapping.
 */
int mog_hybrid_router_init(mog_hybrid_router_t *router,
                           mog_route_candidate_t *storage,
                           size_t capacity,
                           size_t max_paths_per_destination,
                           uint16_t switch_margin);

int mog_hybrid_router_install(mog_hybrid_router_t *router,
                              const mog_route_candidate_t *candidate);

int mog_hybrid_router_invalidate(mog_hybrid_router_t *router, uint32_t path_id);

int mog_hybrid_router_select_primary(const mog_hybrid_router_t *router,
                                     mog_node_id_t destination,
                                     uint32_t current_path_id,
                                     mog_route_candidate_t *out);

size_t mog_hybrid_router_count_for_destination(const mog_hybrid_router_t *router,
                                                mog_node_id_t destination);

#ifdef __cplusplus
}
#endif

#endif /* MOG_ROUTER_H */
