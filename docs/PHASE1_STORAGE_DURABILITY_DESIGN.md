# Phase 1 Storage Durability Design

## Purpose

Phase 1 makes message durability strong enough that later routing, custody and delayed-delivery code can rely on it. The goal is not merely to keep a file readable after reboot; it is to ensure the firmware never claims committed message state that can disappear or be silently corrupted by a power cut.

## Baseline finding

The pinned Bramble foundation already provides internal SPIFFS message persistence and can recover a torn trailing append. Two durability hazards remain for Firmware-mash requirements:

1. rollover/compaction truncates and rewrites the active file in place;
2. status/message updates rewrite existing records in place, so a power loss during an update can corrupt an otherwise size-valid record.

Firmware-mash therefore does not use in-place mutation as its final durable-state model.

## Implemented Phase-1 model

Durable message state is now composed of three layers:

1. `mog_store_journal` — append-only mutation log;
2. `mog_store_snapshot` — dual-slot transactional checkpoints;
3. `mog_store_state` — host-testable recovery/state engine combining snapshot + journal.

Bramble keeps its existing public `msg_store_spiffs_*` API. The Firmware-mash overlay replaces only the persistence backend beneath that API, so the rest of the pinned foundation does not need a parallel message-store implementation.

### 1. Append-only journal for normal mutations

Normal durable changes are represented as monotonically sequenced journal operations:

- `PUT` stores the complete durable state of one message UID;
- `DELETE` removes a UID only after the higher-level lifecycle contract permits deletion.

Each journal entry uses a fixed little-endian header with explicit format version, sequence, UID, payload size, payload CRC, header CRC and commit marker. The writer first writes an uncommitted header + payload and fsyncs them, then publishes the committed header and fsyncs again. An unfinished append therefore cannot be mistaken for a committed entry.

Recovery validates entries strictly in ascending sequence order and stops at the first torn, corrupt or non-monotonic entry. It never skips bad bytes and resumes later. If a damaged tail is found, startup truncates the journal to the reported validated `valid_bytes` before any new append.

### 2. Transactional snapshots

Snapshot format v2 uses a fixed little-endian on-disk header rather than writing a native C structure. Each committed snapshot records:

- generation;
- record size/count;
- highest included journal sequence (watermark);
- payload CRC;
- header CRC;
- explicit commit marker.

Compaction writes only to the inactive slot. The previous committed slot is left untouched until the new payload and commit header are durable and validate correctly. Boot chooses the newest fully valid generation, using the sequence watermark as the tie-breaker.

The snapshot watermark prevents already-checkpointed journal events from being applied twice after reboot.

### 3. State/recovery engine

`mog_store_state` reconstructs bounded current state from:

1. newest valid snapshot;
2. journal entries newer than the snapshot watermark.

It rejects invalid/duplicate zero keys, records the next sequence/generation and exposes the validated journal prefix for tail repair.

## Durable payload schema guard

The journal/snapshot container format is explicitly serialized, but the current Phase-1 payload is the approved Bramble `stored_msg_t` record layout. That payload must never drift silently across a firmware update.

`overlay/bramble/components/msg_store/mog_msg_store_layout_guard.c` is therefore compiled as part of the real Bramble `msg_store` component and contains compile-time assertions for:

- `MSG_TEXT_MAX` and `MSG_ROUTE_MAX_HOPS`;
- enum widths;
- every persisted field offset;
- final `sizeof(stored_msg_t)` (716 bytes for the approved foundation).

The host gate compiles the same guard against the mirrored Bramble test layout. The real T-Deck build compiles it against the pinned upstream header. If upstream or future Firmware-mash code changes the record layout, the build fails closed and a reviewed storage-schema migration/version bump is required before release. Record-size equality alone is not accepted as sufficient compatibility evidence.

## Bramble adapter behavior

`overlay/bramble/components/msg_store/msg_store_spiffs.c` preserves Bramble's API while changing persistence semantics:

- `save()` -> durable journal `PUT` keyed by UID;
- `update()` -> durable journal `PUT` on the same UID, never an in-place record rewrite;
- `load_recent()` -> recovered snapshot+journal state;
- `rollover()` -> retained recent state is checkpointed transactionally to the inactive snapshot slot;
- `clear()` -> message-persistence files only; identity/config storage is untouched.

The adapter preserves the original durable timestamp during later status/route updates. Bramble deliberately zeros restored RAM timestamps because old uptime values are meaningless after reboot; that zero must not overwrite the durable record.

If the durable store is already full after a reboot (for example power loss after the final append but before caller-side rollover), the next save first performs a proactive transactional rollover. This prevents a full-store deadlock.

## Bounded journal growth

Logical message count alone is not enough to trigger compaction because delivery/status updates can generate many journal events without adding messages.

The adapter therefore performs a full checkpoint after a bounded number of journal mutations (`MOG_JOURNAL_CHECKPOINT_OPS`, currently 256). A failed cleanup does not invalidate an already durable journal operation; recovery re-derives authoritative state and can safely remove a journal containing only events already covered by the committed snapshot watermark.

No unbounded update-only journal growth is permitted in STABLE builds.

## Boot recovery

Recovery order:

1. validate both snapshot slots;
2. select the newest valid committed generation;
3. load it completely; never expose a partial snapshot;
4. replay the journal using the snapshot sequence watermark;
5. stop at the first torn/corrupt/non-monotonic journal entry;
6. if replay reports partial recovery, truncate exactly to its validated `valid_bytes` before allowing another append;
7. reconstruct bounded in-memory durable state;
8. establish the next sequence strictly above every committed/replayed sequence;
9. if the journal only contains entries already covered by the snapshot watermark, truncate that stale journal safely;
10. continue device boot even when message persistence is unavailable, without erasing identity/config.

## Legacy Bramble persistence policy

There is no prior public Firmware-mash release whose message schema must be migrated in Phase 1. Therefore the development overlay does **not** silently reinterpret or import the legacy Bramble `/spiffs/messages.bin` file.

Policy:

- if a legacy Bramble file exists while the new Firmware-mash store is empty, it is left untouched and a warning is logged;
- no bytes are silently reinterpreted as the new journal/snapshot format;
- an explicit user message-store clear may remove the legacy message file as part of clearing message history;
- if a future supported migration is required, it must be separately versioned, transactional and tested before release.

Once Firmware-mash has a public persistent schema, normal Firmware-mash updates must preserve/migrate that schema according to the update/recovery contracts.

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

The durable system has or must retain explicit limits for:

- maximum active/persisted messages;
- message/record size;
- journal mutation threshold for checkpointing;
- retained snapshot generations (two slots);
- compaction scratch memory;
- write-amplification/storage-health counters.

T-Deck compaction scratch/state allocations prefer PSRAM and fall back to default heap only when necessary.

## Host fault/integration tests

The Phase-1 host suite now covers the storage primitives/state engine plus the real Bramble persistence adapter compiled through ESP/Bramble host shims.

Covered scenarios include:

- CRC32 reference/incremental behavior;
- torn/invalid snapshot fallback;
- wrong record schema;
- snapshot read capacity protection;
- journal torn/corrupt tail;
- journal sequence/commit validation;
- startup tail truncation before subsequent append;
- snapshot watermark excluding already-checkpointed events;
- snapshot + journal recovery;
- compile-time durable payload layout guard;
- adapter timestamp preservation across a process restart;
- full-store proactive rollover;
- adapter reboot after rollover;
- adapter torn-journal recovery followed by a new append and another reboot.

Still required before STABLE promotion:

- physical no-SD boot;
- real power-cut testing at multiple commit/compaction points;
- flash wear/health measurement under realistic message/update load;
- real T-Deck target build evidence for the current overlay;
- release/update migration tests once a public Firmware-mash schema exists.

## Phase 1 exit condition

Phase 1 is not complete merely because the storage components exist. It exits only when:

- host tests pass with warnings-as-errors;
- the durable payload layout guard compiles;
- the real Bramble adapter integration test passes;
- pinned foundation sync + overlay application are reproducible and fail closed on source drift;
- the journal + snapshot model is integrated under Bramble's existing message-store API;
- target T-Deck Plus build succeeds with the overlay;
- source/simulator no-SD behavior is preserved;
- hardware-only release evidence remains clearly marked unverified rather than assumed.

Current infrastructure note: GitHub's `phase1-foundation` and `preflight` jobs are presently queued before step execution in this repository. That runner/scheduling condition is not counted as a firmware PASS or FAIL; target-build evidence remains open until a runner actually executes the gate.