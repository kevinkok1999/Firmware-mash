#include "msg_store.h"

#include <stddef.h>

/*
 * Firmware-mash Phase-1 persistence currently stores stored_msg_t records as
 * the payload of a versioned/CRC-protected snapshot+journal format. The disk
 * container is portable, but the record payload intentionally remains tied to
 * the approved Bramble stored_msg_t layout.
 *
 * These assertions make that contract fail closed. If upstream changes field
 * order, enum width, padding, route capacity or text capacity, the target build
 * stops here instead of silently interpreting old durable bytes with a new
 * layout. A reviewed storage-schema migration/version bump is then required.
 */
_Static_assert(MSG_TEXT_MAX == 640, "durable schema drift: MSG_TEXT_MAX");
_Static_assert(MSG_ROUTE_MAX_HOPS == 10, "durable schema drift: MSG_ROUTE_MAX_HOPS");
_Static_assert(sizeof(msg_direction_t) == 4, "durable schema drift: direction enum width");
_Static_assert(sizeof(msg_status_t) == 4, "durable schema drift: status enum width");

_Static_assert(offsetof(stored_msg_t, peer_addr) == 0, "durable schema drift: peer_addr");
_Static_assert(offsetof(stored_msg_t, uid) == 4, "durable schema drift: uid");
_Static_assert(offsetof(stored_msg_t, direction) == 8, "durable schema drift: direction");
_Static_assert(offsetof(stored_msg_t, status) == 12, "durable schema drift: status");
_Static_assert(offsetof(stored_msg_t, packet_id) == 16, "durable schema drift: packet_id");
_Static_assert(offsetof(stored_msg_t, timestamp_s) == 20, "durable schema drift: timestamp_s");
_Static_assert(offsetof(stored_msg_t, rssi) == 24, "durable schema drift: rssi");
_Static_assert(offsetof(stored_msg_t, snr) == 25, "durable schema drift: snr");
_Static_assert(offsetof(stored_msg_t, channel_index) == 26, "durable schema drift: channel_index");
_Static_assert(offsetof(stored_msg_t, route_hop_count) == 28, "durable schema drift: route_hop_count");
_Static_assert(offsetof(stored_msg_t, route_hops) == 32, "durable schema drift: route_hops");
_Static_assert(offsetof(stored_msg_t, text_len) == 72, "durable schema drift: text_len");
_Static_assert(offsetof(stored_msg_t, text) == 74, "durable schema drift: text");
_Static_assert(sizeof(stored_msg_t) == 716, "durable schema drift: stored_msg_t size");
