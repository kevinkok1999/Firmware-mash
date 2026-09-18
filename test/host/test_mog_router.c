#include <assert.h>
#include <stdio.h>

#include "mog_router.h"

static mog_route_candidate_t route(uint32_t id, mog_node_id_t destination,
                                   mog_node_id_t next_hop, uint16_t score)
{
    mog_route_candidate_t value = {
        .path_id = id,
        .destination = destination,
        .next_hop = next_hop,
        .first_link = MOG_LINK_LORA,
        .hop_count = 1U,
        .score = score,
        .confidence = 100U,
        .valid = true,
    };
    return value;
}

int main(void)
{
    mog_route_candidate_t storage[4];
    mog_hybrid_router_t router;
    mog_route_candidate_t out;
    mog_route_candidate_t a = route(10U, 100U, 2U, 100U);
    mog_route_candidate_t b = route(11U, 100U, 3U, 105U);
    mog_route_candidate_t c = route(12U, 100U, 4U, 120U);
    mog_route_candidate_t other = route(20U, 200U, 5U, 90U);
    mog_route_candidate_t conflict = route(10U, 300U, 6U, 200U);

    assert(mog_hybrid_router_init(&router, storage, 4U, 2U, 10U) == MOG_ROUTER_OK);
    assert(router.count == 0U);

    assert(mog_hybrid_router_install(&router, &a) == MOG_ROUTER_OK);
    assert(mog_hybrid_router_install(&router, &b) == MOG_ROUTER_OK);
    assert(mog_hybrid_router_count_for_destination(&router, 100U) == 2U);
    assert(mog_hybrid_router_install(&router, &c) == MOG_ROUTER_ERR_FULL);
    assert(mog_hybrid_router_install(&router, &conflict) == MOG_ROUTER_ERR_CONFLICT);

    assert(mog_hybrid_router_select_primary(&router, 100U, 10U, &out) == MOG_ROUTER_OK);
    assert(out.path_id == 10U); /* +5 does not cross the +10 switch margin. */

    b.score = 111U;
    assert(mog_hybrid_router_install(&router, &b) == MOG_ROUTER_OK);
    assert(router.count == 2U); /* replacement must not inflate occupancy. */
    assert(mog_hybrid_router_select_primary(&router, 100U, 10U, &out) == MOG_ROUTER_OK);
    assert(out.path_id == 11U);

    assert(mog_hybrid_router_install(&router, &other) == MOG_ROUTER_OK);
    assert(mog_hybrid_router_invalidate(&router, 10U) == MOG_ROUTER_OK);
    assert(router.count == 2U);
    assert(mog_hybrid_router_install(&router, &c) == MOG_ROUTER_OK);
    assert(mog_hybrid_router_select_primary(&router, 100U, 0U, &out) == MOG_ROUTER_OK);
    assert(out.path_id == 12U);

    assert(mog_hybrid_router_invalidate(&router, 999U) == MOG_ROUTER_ERR_NOT_FOUND);
    assert(mog_hybrid_router_select_primary(&router, 999U, 0U, &out) == MOG_ROUTER_ERR_NOT_FOUND);

    puts("mog_router: PASS");
    return 0;
}
