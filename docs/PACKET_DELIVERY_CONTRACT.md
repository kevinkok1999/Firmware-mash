# Packet and Delivery Contract

## Purpose

This document freezes the logical packet and delivery semantics before implementation. It is transport-independent and applies equally to LoRa, ESP-NOW Normal, ESP-NOW Long Range and future compatible transports.

## Logical message identity

A user message receives one stable `PacketId` before route selection. That PacketId survives:

- retransmission;
- alternate-path failover;
- transport changes;
- store-and-forward delay;
- reboot/power recovery;
- duplicate arrival through multiple paths.

Transport-local sequence numbers never replace PacketId.

## Delivery states

Normative logical states:

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

`WAITING_ROUTE` means the message is retained durably and will be retried when credible connectivity returns. It is not a user-visible failure.

## End-to-end success

A link-layer TX success, ESP-NOW send callback or LoRa radio completion is not sufficient for application delivery. A message is marked DELIVERED only after the destination's end-to-end delivery evidence/ACK is correlated to the same PacketId.

## Automatic delayed delivery

When no usable route exists, the sender stores the encrypted logical message in the internal MessageStore and enters WAITING_ROUTE.

The message becomes immediately retry-eligible when a credible connectivity event indicates that the destination or a route may be reachable again. Examples:

```text
NEIGHBOR_UP
LINK_RECOVERED
ROUTE_DISCOVERED
ROUTE_AVAILABLE
TRANSPORT_RECOVERED
```

There is no distance-based retry trigger. Firmware must never encode a rule such as "retry at 200 m". A user moving from 5 km away back into usable radio range receives the pending message as soon as a valid route exists and normal bounded scheduling permits.

## Anti-storm behavior

Immediate retry eligibility does not mean uncontrolled repeated transmission. ReliabilityManager applies:

- bounded retry count per opportunity;
- jitter/backoff;
- suppression of duplicate route-up events;
- airtime/regulatory gating;
- path health/hysteresis.

The goal is fast recovery without retry storms.

## Route and transport independence

The same pending message may be delivered after recovery through:

- direct LoRa;
- multi-hop LoRa;
- direct ESP-NOW Normal;
- direct ESP-NOW Long Range;
- mixed ESP-NOW/LoRa path;
- alternate cached route discovered after the original send failed.

The user does not need to resend or choose a transport.

## Exactly-once application presentation

Network delivery is at-least-once internally because retries and multipath may produce duplicates. Application presentation is exactly-once within the dedup retention window:

- duplicates with the same PacketId are collapsed;
- the destination may re-ACK a duplicate when needed;
- duplicate arrival may update path/link metrics;
- the chat/UI displays the logical message once.

## Durable retry metadata

The durable store must retain enough information to resume delivery safely after reboot without persisting high-churn routing state. At minimum:

- PacketId;
- destination identity;
- encrypted payload/envelope;
- creation time/TTL semantics;
- delivery class/priority;
- retry epoch/state needed to prevent uncontrolled replay;
- integrity/checksum metadata.

Routes, RSSI history and neighbor tables are reconstructed after boot rather than persisted with every update.

## Expiry

Messages do not live forever by accident. TTL/expiry policy is deterministic and versioned. Expired messages are removed from retry eligibility and surfaced clearly to the user if that state matters to the UI.

## Required tests

Before STABLE promotion:

1. Send while destination is unreachable -> WAITING_ROUTE.
2. Restore direct LoRa reachability -> automatic delivery without second Send press.
3. Restore ESP-NOW reachability -> automatic delivery without second Send press.
4. Restore only an alternate multi-hop route -> automatic delivery.
5. Reboot sender while waiting -> pending message survives and later delivers.
6. Duplicate route/neighbor-up events -> bounded retries, no storm.
7. Duplicate packet arrivals -> one application message.
8. ACK lost after destination receives -> retry may occur, destination still displays once.
9. Expired queued message -> deterministic expiry, no later delivery.