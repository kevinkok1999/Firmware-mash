# Phase 1 Storage Durability Design

## Purpose

Phase 1 makes message durability strong enough that later routing, custody and delayed-delivery code can rely on it. The goal is not merely to keep a file readable after reboot; it is to ensure the firmware never claims committed message state that can disappear or be silently corrupted by a power cut.

## Baseline finding

The pinned Bramble foundation already provides internal SPIFFS message persistence and can recover a torn trailing append. Two durability hazards remain for Firmware-mash requirements:

1. rollover/compaction truncates and rewrites the active file in place;
2. status/message updates rewrite existing records in place, so a power loss during an update can corrupt an otherwise size-valid record.

Firmware-mash therefore does not use in-place mutation as its final durable-state model.

## Required model

Durable message state is composed of two mechanisms.

### 1. Append-only journal for normal mutations

Normal durable changes are represented as monotonically sequenced journal operations:

- `PUT` stores the complete durable state of one message UID;
- `DELETE` removes a UID only after the higher-level lifecycle contract permits deletion.

Each entry has an explicitly serialized little-endian header, sequence, UID, payload CRC, header CRC and commit marker. The writer first writes header + payload and fsyncs, then publishes the committed header and fsyncs again. An unfinished append therefore cannot be mistaken for a committed entry.

Recovery validates entries strictly in ascending sequence order and stops at the first torn, corrupt or non-monotonic entry. It never skips bad bytes and resumes later. If a damaged tail is found, startup must truncate the journal to the reported validated `valid_bytes` before any new append. Otherwise later valid events could be placed behind unreachable corrupt bytes.

A committed snapshot stores a sequence watermark. Journal replay validates older entries but only reapplies entries newer than that watermark, preventing duplicate application after compaction.

### 2. Transactional snapshot for compaction

When journal growth requires compaction:

1. reconstruct current state using the valid committed snapshot plus valid journal prefix;
2. write a complete new snapshot to the inactive/shadow slot with a higher generation;
3. fsync payload;
4. publish the committed self-validating snapshot header;
5. fsync commit;
6. validate and read the new snapshot back;
7. only after successful validation may the old generation and already-checkpointed journal prefix become reclaimable;
8. a power cut before that point leaves the previous committed generation recoverable.

Two valid snapshot generations may coexist temporarily. Boot selects the newest fully valid generation. Generation number never substitutes for CRC/format validation.

## Boot recovery

Recovery order:

1. validate both snapshot slots;
2. select the newest valid committed generation;
3. load it completely; never expose a partial snapshot;
4. replay the journal using the snapshot sequence watermark;
5. stop at the first torn/corrupt/non-monotonic journal entry;
6. if replay reports partial recovery, truncate exactly to its validated `valid_bytes` before allowing another append;
7. reconstruct bounded in-memory MessageStore state;
8. establish the next sequence strictly above every committed/replayed sequence;
9. publish storage health counters/diagnostics;
10. continue boot even when message persistence is unavailable, without erasing identity/config.

If neither snapshot is valid but a legacy Bramble store exists during a supported migration, migration logic must be explicit and transactional. Otherwise storage enters a degraded/recovery state; it must not silently reinterpret unknown bytes.

## Ownership boundaries

- identity/keys/config remain isolated from message persistence;
- route/neighbour/RF metrics stay volatile;
- `mog_message_store` owns durability mechanics;
- ReliabilityManager owns delivery lifecycle policy, not filesystem mechanics;
- CustodyManager may request durable responsibility changes but cannot bypass MessageStore commit acknowledgement;
- UI never reads raw persistence files directly.

## Commit semantics

A caller may treat a durable operation as committed only after the required journal/snapshot fsync and integrity steps succeed.

`Delivered` is unrelated to storage commit: only destination end-to-end acknowledgement can mark a message delivered.

## Boundedness

The durable system must have explicit limits for:

- maximum active messages;
- message/record size;
- journal size/entry threshold that triggers compaction;
- retained snapshot generations;
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
- journal payload/header CRC corruption;
- non-monotonic journal sequence;
- startup tail truncation before subsequent append;
- snapshot watermark excludes already-compacted events from replay;
- power loss before/after both journal fsync points;
- power loss at each snapshot compaction phase;
- repeated compaction without leaked files/storage;
- reboot with full store;
- no identity/config loss when message storage cannot recover.

Physical power-cut testing on a real T-Deck Plus remains a STABLE promotion requirement even after host fault injection passes.

## Phase 1 exit condition

Phase 1 is not complete merely because `mog_store_snapshot` and `mog_store_journal` compile. It exits only when:

- host tests pass with warnings-as-errors;
- the pinned foundation sync is reproducible;
- the journal + snapshot model is integrated over the Bramble message-store adapter;
- legacy migration behavior is defined/tested if required;
- target T-Deck build succeeds;
- source/simulator no-SD behavior is preserved;
- hardware-only release evidence remains clearly marked unverified rather than assumed.