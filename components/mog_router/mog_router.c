#include "mog_router.h"

static bool node_valid(mog_node_id_t node)
{
    return node != 0U;
}

static bool candidate_valid(const mog_route_candidate_t *candidate)
{
    return candidate != NULL && candidate->valid && candidate->path_id != 0U &&
           node_valid(candidate->destination) && node_valid(candidate->next_hop) &&
           candidate->first_link >= MOG_LINK_LORA &&
           candidate->first_link <= MOG_LINK_BACKSCATTER && candidate->hop_count != 0U;
}

int mog_hybrid_router_init(mog_hybrid_router_t *router,
                           mog_route_candidate_t *storage,
                           size_t capacity,
                           size_t max_paths_per_destination,
                           uint16_t switch_margin)
{
    size_t i;

    if (router == NULL || storage == NULL || capacity == 0U ||
        max_paths_per_destination == 0U || max_paths_per_destination > capacity) {
        return MOG_ROUTER_ERR_ARG;
    }

    router->storage = storage;
    router->capacity = capacity;
    router->count = 0U;
    router->max_paths_per_destination = max_paths_per_destination;
    router->switch_margin = switch_margin;

    for (i = 0U; i < capacity; ++i) {
        storage[i].valid = false;
    }

    return MOG_ROUTER_OK;
}

size_t mog_hybrid_router_count_for_destination(const mog_hybrid_router_t *router,
                                                mog_node_id_t destination)
{
    size_t i;
    size_t count = 0U;

    if (router == NULL || router->storage == NULL || !node_valid(destination)) {
        return 0U;
    }

    for (i = 0U; i < router->capacity; ++i) {
        if (router->storage[i].valid && router->storage[i].destination == destination) {
            ++count;
        }
    }

    return count;
}

int mog_hybrid_router_install(mog_hybrid_router_t *router,
                              const mog_route_candidate_t *candidate)
{
    size_t i;
    size_t free_index;
    bool have_free = false;

    if (router == NULL || router->storage == NULL || !candidate_valid(candidate)) {
        return MOG_ROUTER_ERR_ARG;
    }

    for (i = 0U; i < router->capacity; ++i) {
        if (router->storage[i].valid && router->storage[i].path_id == candidate->path_id) {
            if (router->storage[i].destination != candidate->destination) {
                return MOG_ROUTER_ERR_CONFLICT;
            }
            router->storage[i] = *candidate;
            return MOG_ROUTER_OK;
        }
        if (!router->storage[i].valid && !have_free) {
            free_index = i;
            have_free = true;
        }
    }

    if (mog_hybrid_router_count_for_destination(router, candidate->destination) >=
        router->max_paths_per_destination) {
        return MOG_ROUTER_ERR_FULL;
    }
    if (!have_free || router->count >= router->capacity) {
        return MOG_ROUTER_ERR_FULL;
    }

    router->storage[free_index] = *candidate;
    ++router->count;
    return MOG_ROUTER_OK;
}

int mog_hybrid_router_invalidate(mog_hybrid_router_t *router, uint32_t path_id)
{
    size_t i;

    if (router == NULL || router->storage == NULL || path_id == 0U) {
        return MOG_ROUTER_ERR_ARG;
    }

    for (i = 0U; i < router->capacity; ++i) {
        if (router->storage[i].valid && router->storage[i].path_id == path_id) {
            router->storage[i].valid = false;
            if (router->count > 0U) {
                --router->count;
            }
            return MOG_ROUTER_OK;
        }
    }

    return MOG_ROUTER_ERR_NOT_FOUND;
}

int mog_hybrid_router_select_primary(const mog_hybrid_router_t *router,
                                     mog_node_id_t destination,
                                     uint32_t current_path_id,
                                     mog_route_candidate_t *out)
{
    size_t i;
    const mog_route_candidate_t *best = NULL;
    const mog_route_candidate_t *current = NULL;

    if (router == NULL || router->storage == NULL || out == NULL ||
        !node_valid(destination)) {
        return MOG_ROUTER_ERR_ARG;
    }

    for (i = 0U; i < router->capacity; ++i) {
        const mog_route_candidate_t *candidate = &router->storage[i];
        if (!candidate->valid || candidate->destination != destination) {
            continue;
        }
        if (best == NULL || candidate->score > best->score ||
            (candidate->score == best->score && candidate->confidence > best->confidence) ||
            (candidate->score == best->score && candidate->confidence == best->confidence &&
             candidate->path_id < best->path_id)) {
            best = candidate;
        }
        if (current_path_id != 0U && candidate->path_id == current_path_id) {
            current = candidate;
        }
    }

    if (best == NULL) {
        return MOG_ROUTER_ERR_NOT_FOUND;
    }

    if (current != NULL && best != current) {
        const uint32_t required = (uint32_t)current->score + (uint32_t)router->switch_margin;
        if ((uint32_t)best->score <= required) {
            best = current;
        }
    }

    *out = *best;
    return MOG_ROUTER_OK;
}
