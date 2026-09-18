# Wire Protocol Contract

## Purpose

Freeze the protocol layering rules before implementation without inventing a new incompatible security protocol before the selected foundation baseline is pinned.

## Primary compatibility rule

The first implementation must preserve the selected foundation's proven identity/security/message envelope wherever practical. Firmware-mash extends routing/transport behavior around that envelope rather than rewriting cryptography or changing on-air message semantics merely for architectural neatness.

Any incompatible wire-format change requires an ADR, versioning plan, migration/interoperability test and explicit reason.

## Layering

Conceptual layering:

```text
Physical/link framing
  -> transport adapter framing (LoRa / ESP-NOW / future)
  -> versioned logical network/message envelope
  -> end-to-end protected payload/session data
```

Transport framing may differ by link. The logical message identity and end-to-end protected content remain transport-independent.

## Logical message identity

One stable logical PacketId is assigned before route selection and survives:

- LoRa versus ESP-NOW transport changes;
- ESP-NOW Normal versus Long Range mode changes;
- retries;
- fragmentation/reassembly;
- multipath/failover;
- durable WAITING_ROUTE storage;
- gateway/IP traversal;
- custody/store-carry-forward;
- reboot recovery.

PacketId is non-repeating within one origin identity. The network-wide logical message key is `(origin identity, PacketId)`. Wire/control structures that need globally meaningful ACK/dedup/custody identity must carry or unambiguously bind both parts (or a foundation-compatible equivalent). A transport-local sequence/frame ID may exist for link mechanics but never becomes the application identity.

## Versioning

Every Firmware-mash-owned extension that appears on the wire uses an explicit protocol/version field or a foundation-compatible equivalent.

Unknown incompatible versions fail closed/cleanly rather than being interpreted as another structure.

Do not reuse reserved bits/fields from upstream without confirming their contract at the pinned foundation commit.

## Security boundary

- no custom encryption primitive;
- no transport bridge secret treated as end-to-end security;
- relays do not need plaintext user message content merely to forward;
- ESP-NOW link security, if used, is additive and does not replace the end-to-end layer;
- durable MessageStore retains the protected logical envelope rather than unnecessarily storing plaintext message bodies.

Exact security/session types are inherited/adapted from the pinned foundation after baseline review.

## Routing metadata

Only routing/forwarding metadata necessary for the selected protocol is exposed outside the end-to-end protected content. Do not duplicate entire route tables inside user messages.

Route metadata is bounded and versioned. Hop/TTL rules must prevent indefinite circulation.

## Fragmentation

Fragmentation/reassembly ownership must remain single and explicit. Do not independently fragment the same logical message in multiple layers unless the foundation contract requires it.

Rules:

- preserve the stable origin-qualified logical message key across fragments;
- fragment count/size bounded;
- incomplete assemblies expire;
- duplicate fragments are tolerated/deduplicated;
- transport MTU differences are handled below logical delivery semantics;
- a transport change during a retry must not create a second application message.

## ACK semantics

Link-level ACK/result and end-to-end delivery ACK remain separate.

A destination ACK references the origin-qualified logical identity required by ReliabilityManager. Loss of the ACK may cause retransmission, but destination dedup prevents a second UI message.

## ESP-NOW framing

ESP-NOW Normal and ESP-NOW Long Range use the same logical Firmware-mash message/network envelope. LR is a link operating capability, not a second message protocol.

The adapter may add bounded local framing/sequence information required for peer/link delivery, but it must hand the same logical packet upward/downward to HybridRouter/ReliabilityManager.

## LoRa framing

The initial LoRa path should remain as close as possible to the pinned foundation's proven on-air framing. HybridRouter insertion must not force an incompatible LoRa protocol change during the first seam/refactor phase.

Where the pinned Bramble wire header still uses its existing 32-bit packet ID, Firmware-mash must not silently reinterpret that field as the complete new logical identity. A compatibility seam must bind the upstream wire packet to the Firmware-mash origin-qualified message key without breaking legacy LoRa framing. Any on-air extension requires explicit versioning/interoperability evidence.

## Store-and-forward

The durable store records the logical protected envelope plus the minimum delivery metadata needed for retry. It does not serialize ephemeral route candidates as if they were part of the message.

When connectivity returns, the same stored logical message may be re-framed for whatever current valid transport/path is selected.

## Interoperability rule

For each wire-format-affecting change, test at minimum:

- same-version direct communication;
- fragmentation/reassembly if applicable;
- retry with lost ACK;
- route/transport change preserving logical message identity;
- same numeric PacketId from two different origins remains distinct;
- malformed/unknown version rejection;
- mixed old/new version behavior when backward compatibility is claimed.

## What remains baseline-dependent

Only the following are intentionally not frozen before the baseline build/audit:

- exact upstream header structure;
- exact NodeId/PacketId bit width if compatibility requires a different representation;
- exact end-to-end crypto/session framing;
- exact LoRa payload MTU after the pinned foundation/config is measured;
- exact extension-byte layout.

Those values must be filled from real pinned-source evidence, not guessed during one-shot implementation.
