# Conversation Identity and Ordering Contract

## Purpose

Ensure that one contact really remains one chat when packets arrive over different transports, out of order, after reboot, or through a gateway/custody relay.

## Conversation identity

A conversation is keyed by stable authenticated contact identity from the selected foundation, not by:

- LoRa next-hop/node address alone;
- ESP-NOW MAC address;
- IP address;
- gateway ID;
- current route;
- transport type.

Exact key/identity representation is baseline-dependent, but transport addresses are only reachability attributes.

If the foundation supports authenticated identity/key rotation, Firmware-mash may preserve the conversation through that verified rotation. An unverified new identity is never silently merged into an existing contact.

## Message identity

`PacketId` is the idempotency/dedup key for one logical message. Conversation ordering is a separate concern.

Where the selected foundation does not already provide equivalent ordering metadata, each sender maintains a durable per-conversation or per-sender logical sequence/epoch suitable for ordering that sender's messages without relying on wall-clock accuracy.

The sequence state must not roll backward after reboot in a way that makes new messages indistinguishable from old ordering positions. Exact representation is chosen after baseline/storage review.

## Arrival order is not display truth

A message that arrives first over the fastest route is not automatically earlier in the conversation.

Example:

```text
A: message #41 -> slow LoRa path
A: message #42 -> fast IP path

receiver sees #42 first
```

The UI/message service uses ordering metadata to place messages consistently where possible.

## Bounded reorder policy

The receiver may hold a small bounded reorder window for missing immediately-previous messages, but it may not block the conversation indefinitely.

Rules:

- reorder buffer has fixed capacity/time bound;
- if an earlier message never arrives, later valid messages become visible after the reorder bound;
- a late earlier message may be inserted into its logical position or annotated according to UI policy without duplicating newer messages;
- PacketId dedup remains authoritative;
- reboot reconstructs ordering from durable conversation records rather than a RAM-only reorder buffer;
- ordering metadata cannot be used by relays to read message plaintext.

## Two-way ordering

There is no requirement for a single perfectly synchronized global ordering between two independent senders. The normal chat timeline may use durable local receipt/presentation metadata plus each sender's sequence while preserving causally obvious local sends.

Wall-clock timestamps are display metadata and are not the only correctness mechanism.

## Local send durability

When the user presses Send:

1. allocate PacketId/order metadata;
2. durably create the outgoing message record before reporting it as safely queued;
3. show the chat bubble immediately from local durable/app state;
4. networking changes delivery state in-place for that same record.

A reboot after pressing Send must not create a second local bubble or forget a record that was already reported as queued.

## Contact reachability changes

A contact may accumulate current reachability hints such as LoRa node mapping, ESP-NOW peer address, gateway route or IP capability. Those hints expire/change independently of conversation identity.

A route/address change therefore updates how the contact is reached, not which chat it belongs to.

## Required tests

```text
CONV-001 same contact over LoRa then IP remains one conversation
CONV-002 message N+1 arrives before N; bounded reorder gives deterministic timeline
CONV-003 missing N does not block N+1 forever
CONV-004 late N inserts/reconciles without duplicate bubbles
CONV-005 reboot preserves outgoing local bubble/order/delivery state
CONV-006 changing ESP-NOW MAC/IP/gateway route does not create new chat
CONV-007 unverified identity replacement is not silently merged
CONV-008 verified foundation-supported identity rotation preserves chat when explicitly supported
```

## Promotion rule

The simple UI may hide sequence/order details. These are internal correctness mechanisms so the user experiences one ordinary continuous conversation.
