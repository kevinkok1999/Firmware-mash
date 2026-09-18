# Internal MessageStore Design

## Purpose

Define the code contract for durable pending/offline messages on a stock T-Deck Plus without requiring microSD. The same storage boundary may later hold accepted relay-custody records, but custody remains optional and bounded.

## Storage separation

Use three separate logical domains:

- **identity/config:** small durable values, identity/key references, region and preferences;
- **message store:** higher-churn queued/pending protected messages and optional accepted custody records;
- **volatile runtime:** routes, neighbors, RSSI/SNR history, route scores, gateway scores and hot peer/dedup caches in RAM/PSRAM.

MessageStore corruption or compaction failure must not erase identity/configuration.

## Backend selection gate

Do not freeze LittleFS versus a purpose-built append-only journal until the pinned baseline is built and both candidates can be measured. Compare power-loss recovery, flash overhead/write amplification, compaction behavior, code size, RAM use, corruption containment and implementation/test complexity.

The public MessageStore API remains backend-independent so the choice does not leak into routing/reliability/custody code.

## Logical record model

Conceptual append/journal operations:

```text
PUT_MESSAGE
STATE_UPDATE
CUSTODY_UPDATE        # optional capability
ACK_DELIVERED
EXPIRE
DELETE/TOMBSTONE
CHECKPOINT/COMPACTION_MARKER
```

A committed record includes version, type, length, PacketId where applicable, logical sequence, payload metadata and integrity check. Incomplete tail records after a power cut are ignored/quarantined during recovery.

## Message metadata

Persist only what is needed to resume safely:

```text
PacketId
destination
protected envelope/payload
creation/expiry semantics
delivery class/priority
logical delivery state
bounded retry epoch/counters
optional custody state/transfer epoch/holder metadata
record integrity/version metadata
```

Do not persist route candidates, RSSI/SNR windows, peer-cache state, gateway scores or route scores on every change.

## API contract

Conceptual operations:

```text
init_and_recover()
put(message)
set_state(PacketId, state)
set_custody(PacketId, custody_state, metadata)   # optional
mark_delivered(PacketId)
mark_expired(PacketId)
next_retry_eligible(now, reason)
iterate_pending()
usage()
compact_if_needed()
health()
```

All operations return explicit errors; silent data loss is forbidden.

## Custody commit rule

When custody is enabled, a relay may emit `CUSTODY_ACCEPTED` only after the protected packet and required ownership metadata are durably committed. A failed or incomplete commit means custody is rejected/not accepted.

Custody state is not application delivery state. Marking custody accepted must never call `mark_delivered()`.

## Full-store policy

Storage is bounded. When full:

1. reclaim delivered/expired tombstoned records;
2. compact if policy permits;
3. reject new custody offers before falsely promising responsibility;
4. apply deterministic TTL/priority eviction only according to defined policy;
5. never evict identity/configuration because it is a different domain;
6. expose user/service-visible storage pressure before severe exhaustion where possible.

No unbounded retry loop may occur because storage is full.

## Recovery algorithm

At boot:

1. mount/open the message-store domain;
2. recover only committed valid records;
3. ignore/quarantine incomplete/corrupt tail records;
4. rebuild a bounded RAM/PSRAM index;
5. restore sender-owned pending messages to WAITING_ROUTE/appropriate state;
6. restore accepted custody records to carrying/forward-eligible state as defined by `STORE_CARRY_CUSTODY_CONTRACT.md`;
7. emit recovery/health metrics;
8. continue boot when an isolated recoverable queue record is bad.

A damaged queue must not cause a boot loop.

## Power-loss rules

Required fault-injection points include:

- during PUT_MESSAGE;
- during STATE_UPDATE;
- during CUSTODY_UPDATE when enabled;
- immediately before/after custody commit evidence;
- during ACK_DELIVERED/tombstone;
- during compaction copy;
- immediately before/after commit marker;
- first boot following a firmware update.

Expected result: last fully committed state is recoverable; at worst the incomplete operation is discarded/reconciled. Identity/config remains intact.

## Delayed-delivery integration

MessageStore itself does not discover routes. ReliabilityManager/HybridRouter notify it when a pending or custody-held message becomes retry-eligible because credible route/link/gateway evidence occurred. There is no distance threshold.

A recovered/waiting/custody-held message preserves the same PacketId across every retry, transport and gateway transition.

## Wear policy

- no persistent write per RSSI/SNR sample;
- no persistent write per route/gateway-score update;
- coalesce state updates where safe;
- append rather than rewrite large structures;
- compact according to measured thresholds, not every deletion;
- expose append/erase/compaction counters where practical.

## Acceptance tests

- offline destination -> message persists;
- reboot sender -> message remains pending;
- connectivity returns -> automatic delivery without user resend;
- ACK lost after receive -> destination displays once after retry;
- corrupt final record -> boot succeeds and earlier records recover;
- power cut during compaction -> recoverable committed store;
- near-full store -> deterministic bounded behavior;
- no microSD installed for every test above;
- custody-enabled builds additionally pass CUS-001..CUS-010.
