# Packet and Delivery Contract

## Purpose

This document freezes the logical packet and delivery semantics before implementation. It is transport-independent and applies equally to LoRa, ESP-NOW Normal/LR, IP backhaul and future compatible transports.

## Logical message identity

A user message receives one stable `PacketId` before route selection. That PacketId survives:

- retransmission;
- alternate-path failover;
- transport changes;
- IP/gateway traversal;
- sender-side store-and-forward delay;
- optional relay custody/store-carry-forward;
- reboot/power recovery;
- duplicate arrival through multiple paths.

Transport-local sequence numbers, gateway session IDs and custody-transfer epochs never replace PacketId.

A PacketId is non-repeating within one origin identity. The network-wide logical message key is therefore `(origin identity, PacketId)`. Remote dedup, end-to-end ACK correlation, custody and reboot-safe presentation must use that origin-qualified key (or a foundation-compatible equivalent), never a naked PacketId from an unrelated remote origin.

Conversation identity is independent from transport/path. One contact remains one chat regardless of which transport or gateway delivered each message.

### PacketId generation safety

A reboot must never reset PacketId generation to a simple volatile counter that can collide with an earlier packet from the same identity.

The implementation must use the selected foundation's proven durable counter/message-ID mechanism where compatible. If Firmware-mash provides the generator, it uses durable reserve-ahead state so a value is never issued unless a persistence ceiling covering it is already confirmed durable. Unused reserved values may be skipped after reboot; reuse is forbidden.

Hard rules:

- PacketId generation happens before any transport/path selection;
- no timestamp-only or volatile random-seeded incrementing counter is sufficient for durable logical identity;
- factory reset/identity replacement is treated as a new identity epoch;
- rollback to an older firmware image must not roll PacketId state backward into a collision window;
- PacketId generation is unit/fault tested across reboot and interrupted persistence;
- remote identity comparisons are origin-qualified.

## Delivery states

Normative logical user-message states:

```text
CREATED
READY
SENDING
WAITING_ACK
WAITING_ROUTE
DEFERRED
DELIVERED
EXPIRED
FAILED_PERMANENT
```

`WAITING_ROUTE` means the protected message is retained durably and will be retried when credible connectivity returns. It is not a user-visible permanent failure.

Custody state is separate internal ownership metadata defined by `STORE_CARRY_CUSTODY_CONTRACT.md`; it does not create an alternate user delivery truth.

## End-to-end success

A link-layer TX success, ESP-NOW send callback, LoRa radio completion, TCP/TLS socket write, gateway acceptance or custody acceptance is not sufficient for application delivery.

A message is marked `DELIVERED` only after destination end-to-end delivery evidence/ACK is correlated to the same origin-qualified logical message key.

## Automatic delayed delivery

When no usable route exists, the current responsible holder stores the protected logical message in internal durable MessageStore and enters `WAITING_ROUTE`/equivalent holder state.

The message becomes immediately retry-eligible when credible connectivity evidence indicates that the destination or a useful route may now be reachable. Examples:

```text
NEIGHBOR_UP
LINK_RECOVERED
ROUTE_DISCOVERED
ROUTE_AVAILABLE
TRANSPORT_RECOVERED
GATEWAY_AVAILABLE
IP_BACKHAUL_RECOVERED
CUSTODY_FORWARD_ELIGIBLE
```

There is no distance-based retry trigger. Firmware must never encode a rule such as "retry at 200 m".

## Optional relay custody / store-carry-forward

Sender-side delayed delivery remains the default foundation. Optional custody allows a participating relay to durably accept the same protected logical packet and carry it while disconnected.

Hard rules:

- custody acceptance occurs only after relay durable commit;
- custody acceptance is not `DELIVERED`;
- same origin-qualified message key/end-to-end envelope survives custody transfer;
- relay does not require plaintext user message content;
- replication/ownership is bounded;
- TTL/dedup/resource/energy policy remains authoritative;
- custody can be compiled/disabled without breaking ordinary sender-side delayed delivery.

Full semantics are in `STORE_CARRY_CUSTODY_CONTRACT.md` and ADR 0008.

## Custody/delivery race rule

A destination delivery ACK is terminal truth for that logical message key and dominates any in-flight custody negotiation.

If `DELIVERY_ACK(origin, PacketId)` arrives while a custody offer/accept/reconciliation is pending:

1. ReliabilityManager marks the logical message delivered exactly once;
2. new custody offers for that message key stop;
3. current holder persists the terminal delivery/tombstone state before opportunistic cleanup;
4. any later duplicate custody/control/data event is treated idempotently and may be re-ACKed/reconciled, but never reopens the user message;
5. relay copies are expired/cleaned using bounded completion propagation or normal TTL/tombstone policy.

This prevents a race where one task says Delivered while another task creates a new active custody responsibility.

## Anti-storm behavior

Immediate retry eligibility does not mean uncontrolled repeated transmission. ReliabilityManager applies:

- bounded retry count per opportunity;
- jitter/backoff;
- suppression of duplicate route/gateway/custody events;
- airtime/regulatory gating;
- path health/hysteresis;
- bounded custody offer/reconciliation attempts.

The goal is fast recovery without retry storms.

## Route and transport independence

The same pending message may be delivered after recovery through:

- direct LoRa;
- multi-hop LoRa;
- ESP-NOW Normal/LR;
- mixed ESP-NOW/LoRa path;
- Wi-Fi/IP backhaul;
- LoRa -> gateway -> IP -> gateway -> LoRa;
- selected cellular/IP provider when compatible hardware exists;
- optional store-carry-forward relay custody;
- cached alternate path discovered after the original send failed.

The user does not resend or choose a route per message.

## Exactly-once application presentation

Network delivery is at-least-once internally because retries, failover, gateway traversal or custody reconciliation may produce duplicates. Application presentation is exactly-once within the dedup/presentation-retention policy:

- duplicates with the same `(origin, PacketId)` are collapsed;
- destination may re-ACK a duplicate when needed;
- duplicate arrival may update link/path evidence;
- the chat/UI displays the logical message once.

### Reboot-safe presentation

A volatile RAM dedup cache alone is insufficient. Otherwise the destination could reboot and then display a late retry of an already-presented message key as a new message.

The conversation/message persistence layer therefore performs an idempotent insert keyed by the origin-qualified logical message identity. A duplicate after reboot resolves to the existing message/presentation record and may refresh ACK state, but cannot append a second chat bubble.

A bounded durable recent-delivery/presentation index or equivalent indexed conversation store is required. It must not become an unbounded flash log.

## Durable time / TTL semantics

Durable message records must not store a raw absolute deadline derived solely from volatile boot-relative monotonic milliseconds, because that clock restarts after reboot.

The implementation uses two time domains explicitly:

- **runtime monotonic time:** retries, route ageing, backoff, session timers and in-boot deadlines;
- **durable message lifetime:** persisted TTL budget/age metadata that remains meaningful across reboot.

When a trusted wall-clock source is available, it may be used to account for powered-off elapsed time. When no trusted wall clock/RTC is available, powered-off duration must not be guessed from an untrusted clock; the documented safe policy is to resume from the persisted remaining TTL budget and continue consuming it while running. This may extend real-world wall time but never causes a fresh reboot to instantly expire or resurrect a message because of clock reset.

No durable field named like `expires_at_ms` may contain a boot-relative absolute deadline unless it also carries an epoch/domain that makes comparison valid after reboot.

## Durable retry metadata

The durable store retains enough information to resume safely after reboot without persisting high-churn routing state. At minimum:

- origin identity + PacketId;
- destination identity;
- protected payload/envelope;
- durable creation/TTL budget semantics;
- delivery class/priority;
- retry epoch/state needed to prevent uncontrolled replay;
- integrity/version metadata;
- optional custody ownership metadata when that capability is enabled;
- terminal presentation/delivery identity sufficient for reboot-safe idempotency where this node is the destination.

Routes, RSSI history, neighbor tables and gateway scores are reconstructed after boot rather than persisted with every update.

## Expiry

Messages do not live forever accidentally. TTL/expiry policy is deterministic and versioned. Expired messages are removed from retry/custody eligibility and surfaced clearly where relevant.

## Required tests

Before the corresponding tier is promoted:

1. Send while destination is unreachable -> WAITING_ROUTE.
2. Restore direct LoRa reachability -> automatic delivery without second Send press.
3. Restore ESP-NOW reachability -> automatic delivery.
4. Restore only an alternate multi-hop route -> automatic delivery.
5. Restore IP/gateway route -> automatic delivery with same logical message key/chat.
6. Reboot responsible holder while waiting -> message survives and later delivers.
7. Duplicate route/gateway-up events -> bounded retries, no storm.
8. Duplicate packet arrivals from the same origin/message key -> one application message.
9. Two different origins use the same numeric PacketId -> both messages remain distinct.
10. ACK lost after destination receives -> retry may occur, destination still displays once.
11. Expired queued message -> deterministic expiry, no later delivery.
12. Destination receives message, reboots, then receives duplicate retry -> existing chat message reused, no second bubble.
13. Reboot with pending TTL -> durable lifetime remains valid; boot-relative timer reset cannot instantly expire/resurrect it.
14. Generate PacketIds across repeated reboot/power-cut/rollback simulation -> no reuse for the same origin within the defined safety model.
15. Delivery ACK racing custody acceptance -> terminal delivery wins and no custody state reopens the message.
16. When custody is enabled, run CUS-001..CUS-010 from `TEST_TRACEABILITY.md`.
