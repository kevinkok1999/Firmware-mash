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

Conversation identity is independent from transport/path. One contact remains one chat regardless of which transport or gateway delivered each message.

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

A message is marked `DELIVERED` only after destination end-to-end delivery evidence/ACK is correlated to the same PacketId.

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
- same PacketId/end-to-end envelope survives custody transfer;
- relay does not require plaintext user message content;
- replication/ownership is bounded;
- TTL/dedup/resource/energy policy remains authoritative;
- custody can be compiled/disabled without breaking ordinary sender-side delayed delivery.

Full semantics are in `STORE_CARRY_CUSTODY_CONTRACT.md` and ADR 0008.

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

Network delivery is at-least-once internally because retries, failover, gateway traversal or custody reconciliation may produce duplicates. Application presentation is exactly-once within the dedup retention policy:

- duplicates with the same PacketId are collapsed;
- destination may re-ACK a duplicate when needed;
- duplicate arrival may update link/path evidence;
- the chat/UI displays the logical message once.

## Durable retry metadata

The durable store retains enough information to resume safely after reboot without persisting high-churn routing state. At minimum:

- PacketId;
- destination identity;
- protected payload/envelope;
- creation/TTL semantics;
- delivery class/priority;
- retry epoch/state needed to prevent uncontrolled replay;
- integrity/version metadata;
- optional custody ownership metadata when that capability is enabled.

Routes, RSSI history, neighbor tables and gateway scores are reconstructed after boot rather than persisted with every update.

## Expiry

Messages do not live forever accidentally. TTL/expiry policy is deterministic and versioned. Expired messages are removed from retry/custody eligibility and surfaced clearly where relevant.

## Required tests

Before the corresponding tier is promoted:

1. Send while destination is unreachable -> WAITING_ROUTE.
2. Restore direct LoRa reachability -> automatic delivery without second Send press.
3. Restore ESP-NOW reachability -> automatic delivery.
4. Restore only an alternate multi-hop route -> automatic delivery.
5. Restore IP/gateway route -> automatic delivery with same PacketId/chat.
6. Reboot responsible holder while waiting -> message survives and later delivers.
7. Duplicate route/gateway-up events -> bounded retries, no storm.
8. Duplicate packet arrivals -> one application message.
9. ACK lost after destination receives -> retry may occur, destination still displays once.
10. Expired queued message -> deterministic expiry, no later delivery.
11. When custody is enabled, run CUS-001..CUS-010 from `TEST_TRACEABILITY.md`.
