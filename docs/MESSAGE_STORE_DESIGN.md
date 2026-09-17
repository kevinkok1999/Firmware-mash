# Internal MessageStore Design

## Purpose

Define the code contract for durable pending/offline messages on a stock T-Deck Plus without requiring microSD.

## Storage separation

Use three separate logical domains:

- **identity/config:** small durable values, identity/key references, region and preferences;
- **message store:** higher-churn queued/pending encrypted messages;
- **volatile runtime:** routes, neighbors, RSSI/SNR history, route scores and hot peer/dedup caches in RAM/PSRAM.

MessageStore corruption or compaction failure must not erase identity/configuration.

## Backend selection gate

Do not freeze LittleFS versus a purpose-built append-only journal until the pinned baseline is built and both candidates can be measured. Compare:

- power-loss recovery;
- flash overhead and write amplification;
- compaction behavior;
- code size;
- RAM use;
- corruption containment;
- implementation/test complexity.

The public MessageStore API remains backend-independent so the choice does not leak into routing/reliability code.

## Logical record model

Conceptual append/journal operations:

```text
PUT_MESSAGE
STATE_UPDATE
ACK_DELIVERED
EXPIRE
DELETE/TOMBSTONE
CHECKPOINT/COMPACTION_MARKER
```

A committed record includes version, type, length, PacketId where applicable, monotonic/logical sequence, payload metadata and integrity check. Incomplete tail records after a power cut are ignored during recovery.

## Message metadata

Persist only what is needed to resume safely:

```text
PacketId
destination
encrypted envelope/payload
creation/expiry semantics
delivery class/priority
logical delivery state
bounded retry epoch/counters needed for safe recovery
record integrity/version metadata
```

Do not persist route candidates, RSSI/SNR windows, peer-cache state or route scores on every change.

## API contract

Conceptual operations:

```text
init_and_recover()
put(message)
set_state(PacketId, state)
mark_delivered(PacketId)
mark_expired(PacketId)
next_retry_eligible(now, reason)
iterate_pending()
usage()
compact_if_needed()
health()
```

All operations return explicit errors; silent data loss is forbidden.

## Full-store policy

Storage is bounded. When full:

1. reclaim committed delivered/expired tombstoned records;
2. compact if policy permits;
3. apply deterministic TTL/priority eviction only if still required;
4. never evict identity/configuration because it is in a different domain;
5. expose a user-visible storage warning before severe pressure where possible.

No unbounded retry loop may occur because storage is full.

## Recovery algorithm

At boot:

1. mount/open the message-store domain;
2. scan/recover only committed valid records;
3. ignore or quarantine an incomplete/corrupt final record;
4. rebuild a bounded RAM/PSRAM index;
5. restore pending logical messages to WAITING_ROUTE/appropriate state;
6. emit recovery/health metrics;
7. continue boot even when one recoverable queue record is bad.

A damaged queue must not cause a boot loop.

## Power-loss rules

Required fault-injection points:

- during PUT_MESSAGE;
- during STATE_UPDATE;
- during ACK_DELIVERED/tombstone;
- during compaction copy;
- immediately before/after commit marker;
- first boot following a firmware update.

Expected result: last fully committed state is recoverable; at worst the incomplete operation is discarded. Identity/config remains intact.

## Delayed-delivery integration

MessageStore itself does not discover routes. ReliabilityManager/HybridRouter notify it when a pending message becomes retry-eligible because a credible route/link event occurred. There is no distance threshold.

A recovered or waiting message preserves its PacketId across every retry and transport change.

## Wear policy

- no persistent write per RSSI/SNR sample;
- no persistent write per route-score update;
- coalesce state updates where safe;
- append rather than rewrite large structures;
- compact according to measured thresholds, not on every deletion;
- expose append/erase/compaction counters where practical.

## Acceptance tests

- offline destination -> message persists;
- reboot sender -> message remains pending;
- connectivity returns -> automatic delivery without user resend;
- ACK lost after receive -> destination displays once after retry;
- corrupt final record -> boot succeeds and earlier records recover;
- power cut during compaction -> recoverable committed store;
- near-full store -> deterministic bounded behavior;
- no microSD installed for every test above.