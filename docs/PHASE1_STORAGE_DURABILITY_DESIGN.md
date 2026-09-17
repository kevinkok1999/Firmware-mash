# Phase 1 Storage Durability Design

## Purpose

Phase 1 makes message durability strong enough that later routing, custody and delayed-delivery code can rely on it. The goal is not merely to keep a file readable after reboot; it is to ensure the firmware never claims committed message state that can disappear or be silently corrupted by a power cut.

## Baseline finding

The pinned Bramble foundation already provides internal SPIFFS message persistence and can recover a torn trailing append. Two durability hazards remain for Firmware-mash requirements:

1. rollover/compaction truncates and rewrites the active file in place;
2. status/message updates rewrite existing records in place, so a power loss during an update can corrupt an otherwise size-valid record.

Firmware-mash therefore does not use in-place mutation as its final durable-state model.

## Required model

Durable message state is composed of two mechanisms:

### 1. Append-only journal for normal mutations

Normal durable changes are represented as monotonically sequenced journal events:

- UPSERT a complete durable message record/state;
- DELETE a durable record only after the higher-level contract permits deletion.

Each event contains a self-validating header, payload CRC and monotonically increasing sequence number. An event is visible to recovery only when its complete header and payload validate. A torn/corrupt trailing event is ignored and may be truncated by repair; recovery never skips corruption and then accepts later bytes.

This removes normal in-place status mutation from the durability path.

### 2. Transactional snapshot for compaction

When journal growth requires compaction:

1. materialize current state in memory using the committed snapshot + valid journal;
2. write a complete new snapshot to the inactive/shadow slot with a higher generation;
3. fsync payload;
4. publish the committed self-validating snapshot header;
5. fsync commit;
6. validate the new snapshot by reading it back;
7. only after successful validation may the old generation/journal become reclaimable;
8. a power cut at any point before step 6 leaves the previous committed generation recoverable.

Two valid snapshot generations may coexist temporarily. Boot selects the newest fully valid generation. Generation number never substitutes for CRC/format validation.

## Boot recovery

Recovery order:

1. validate both snapshot slots;
2. select the newest valid committed generation;
3. load it completely; never expose a partial snapshot;
4. replay journal entries with sequence values newer than the snapshot checkpoint;
5. stop at the first torn/corrupt/non-monotonic journal entry;
6. repair only the invalid trailing suffix;
7. reconstruct bounded in-memory MessageStore state;
8. publish storage health counters/diagnostics;
9. continue boot even when message persistence is unavailable, without erasing identity/config.

If neither snapshot is valid but a legacy Bramble store exists during a supported migration, migration logic must be explicit and transactional. Otherwise storage enters a degraded/recovery state; it must not silently reinterpret unknown bytes.

## Ownership boundaries

- identity/keys/config remain isolated from message persistence;
- route/neighbour/RF metrics stay volatile;
- `mog_message_store` owns durability mechanics;
- ReliabilityManager owns delivery lifecycle policy, not filesystem mechanics;
- CustodyManager may request durable responsibility changes but cannot bypass MessageStore commit acknowledgement;
- UI never reads raw persistence files directly.

## Commit semantics

A caller may treat a durable operation as committed only after the journal/snapshot function returns success following `fsync` and integrity validation required by that operation.

`Delivered` is unrelated to storage commit: only destination end-to-end acknowledgement can mark a message delivered.

## Boundedness

The durable system must have explicit limits for:

- maximum active messages;
- maximum record size;
- maximum journal payload size;
- journal size/entry threshold that triggers compaction;
- retained generations;
- compaction scratch memory;
- write-amplification/health counters.

No unbounded journal growth is permitted in STABLE builds.

## Fault-injection requirements

Host/simulator tests must eventually cover at least:

- torn snapshot payload;
- torn snapshot commit header;
- corrupted snapshot payload CRC;
- wrong record schema/version;
- both snapshot slots present, newest invalid;
- journal torn in header;
- journal torn in payload;
- journal CRC corruption;
- non-monotonic journal sequence;
- power loss before/after journal `fsync`;
- power loss at each snapshot compaction phase;
- repeated compaction without leaked files/storage;
- reboot with full store;
- no identity/config loss when message storage cannot recover.

Physical power-cut testing on a real T-Deck Plus remains a STABLE promotion requirement even after host fault injection passes.

## Phase 1 exit condition

Phase 1 is not complete merely because `mog_store_snapshot` and `mog_store_journal` compile. It exits only when:

- the host tests pass with warnings-as-errors;
- the pinned foundation sync is reproducible;
- the journal + snapshot model is integrated over the Bramble message-store adapter;
- legacy migration behavior is defined/tested if needed;
- target T-Deck build succeeds;
- source/simulator no-SD behavior is preserved;
- hardware-only release evidence remains clearly marked unverified rather than assumed.