# Store-Carry-Forward and Custody Contract

## Purpose

Define opportunistic relay custody without changing the user conversation, PacketId, end-to-end protection or delivery truth.

Sender-side delayed delivery and relay custody are different mechanisms:

- **sender delayed delivery:** sender retains a protected message in `WAITING_ROUTE` until a usable route returns;
- **relay custody:** a participating relay durably accepts responsibility for carrying that same protected logical packet while disconnected and forwarding it later.

Custody is a BETA capability until repeated real T-Deck hardware evidence exists. Normal sender-side delayed delivery remains available when custody is disabled.

## Core invariants

1. One logical `PacketId` survives every offer, acceptance, reboot, transport change and final delivery.
2. One user conversation remains one conversation; custody never creates a second chat or cloned user-visible message.
3. Relays store the end-to-end protected logical envelope and do not require plaintext message content.
4. Custody acceptance is **not** end-to-end delivery. The UI cannot show `Delivered` until destination delivery evidence reaches ReliabilityManager.
5. A relay may emit custody-accepted evidence only after the protected packet and required ownership metadata are durably committed locally.
6. Custody storage, offers, retries, tables and replication are bounded.
7. TTL/expiry and dedup remain authoritative. No immortal packets.
8. AirtimeManager, ReliabilityManager and EnergyManager policy remain authoritative; custody cannot bypass them.
9. A relay with insufficient durable storage, critical energy or incompatible capability rejects custody deterministically.
10. Disabling custody cannot break ordinary LoRa/ESP-NOW/IP routing or sender-side `WAITING_ROUTE` behavior.

## Initial ownership model

The initial implementation uses **single active custody ownership** for one logical packet. Bounded fallback copies may exist only while ownership transfer is unresolved; they must not become uncontrolled replication.

Conceptual states:

```text
NO_CUSTODY
OFFERING
AWAITING_ACCEPT
CUSTODY_TRANSFERRED
CARRYING
FORWARDING
DESTINATION_DELIVERED
EXPIRED
REJECTED
```

The exact enum may be adapted to the foundation, but the semantics above are normative.

## Transfer sequence

```text
A has PacketId P in WAITING_ROUTE
  -> A discovers eligible relay B
  -> A sends bounded custody offer for P
  -> B validates capability/policy/TTL/dedup/storage/energy
  -> B durably commits protected packet + custody metadata
  -> only then B returns authenticated custody-accepted evidence
  -> A records transferred responsibility according to ReliabilityManager policy
  -> B may move while disconnected
  -> B later finds destination/route/gateway
  -> B forwards same PacketId P
  -> destination E2E ACK proves DELIVERED
```

A transport-local ACK or custody acceptance never substitutes for the destination ACK.

## Failure semantics

### Acceptance evidence lost

If B committed the packet but A did not receive the acceptance evidence, bounded re-offer/reconciliation may occur. Dedup and stable PacketId prevent duplicate application delivery. This must not trigger flooding.

### Relay reboot

After accepted custody, B must recover the packet and custody metadata from internal durable storage. A reboot must not silently lose accepted responsibility.

### Relay storage pressure

If storage is full or below reserved safety margin, B rejects new custody before promising responsibility. Existing committed custody records follow deterministic priority/TTL policy.

### Relay energy pressure

`CRITICAL`/`SURVIVAL` policy may reject new custody or defer forwarding. It cannot claim delivery and cannot discard a committed non-expired custody packet outside defined storage/expiry policy.

### Relay disappears permanently

The initial protocol must not pretend this failure can be made impossible. Transfer/reconciliation policy therefore keeps acceptance evidence, TTL and deterministic sender/previous-holder recovery semantics. Exact recovery timers are measured/simulator-tuned rather than guessed.

## Security and privacy

- No custom cryptography.
- Custody control evidence is authenticated using the selected foundation/security layer or a reviewed compatible mechanism.
- Relay does not need message plaintext.
- Do not leak exact route history or precise GPS merely to support custody.
- Replay/duplicate custody offers are idempotent for the same PacketId and transfer epoch.

## MessageStore integration

Relay custody extends MessageStore records with the minimum required ownership metadata, such as:

```text
PacketId
protected envelope
destination
creation/expiry
priority/delivery class
custody state
custody transfer epoch / previous holder reference where required
integrity/version metadata
```

High-churn route/RSSI/location history is not persisted with every custody record.

## Events

Conceptual events:

```text
CUSTODY_CANDIDATE
CUSTODY_OFFER
CUSTODY_ACCEPTED
CUSTODY_REJECTED
CUSTODY_RECOVERED
CUSTODY_FORWARD_ELIGIBLE
CUSTODY_EXPIRED
```

Names may adapt to the implementation, but event handling remains bounded and goes through the normal network-state owner.

## Required tests

```text
CUS-001 durable commit precedes custody acceptance
CUS-002 sender can power off after accepted custody; relay later delivers
CUS-003 relay reboot preserves accepted custody
CUS-004 duplicate offer/accept does not duplicate app message
CUS-005 full-store/critical-energy relay rejects deterministically
CUS-006 lost custody evidence stays bounded and deduplicated
CUS-007 TTL expiry removes custody deterministically
CUS-008 PacketId survives LoRa/ESP-NOW/IP transitions after custody
CUS-009 custody disabled -> normal sender WAITING_ROUTE still works
CUS-010 malformed/unauthenticated custody control fails closed
```

## Promotion rule

Host/simulator evidence can validate state-machine behavior. Real custody promotion beyond BETA requires repeatable T-Deck hardware tests including reboot, movement/disconnection, storage pressure, transport changes and final exactly-once presentation.