# ADR 0009 — Pinned foundation with Firmware-mash overlay

Status: Accepted

## Context

Firmware-mash needs to retain Bramble's coherent T-Deck Plus board support, ESP-IDF build, simulator/tests, security envelope and radio integration while adding project-owned behavior. Copying selected files ad hoc would destroy provenance and make upstream updates difficult. A cosmetic full rename would add risk without product value.

The approved foundation is:

```text
repository=https://github.com/justinlindh/bramble
commit=23854fd883fd29da14d8c0876f6eca4e14fbb938
```

## Decision

Use a **pinned foundation + overlay** integration model.

1. `tools/foundation_sync.py` materializes the exact pinned Bramble commit into a generated/local foundation workspace.
2. The sync step verifies the resolved 40-character commit and refuses a moving/unexpected revision.
3. Bramble source keeps its upstream names, structure and MIT provenance.
4. Firmware-mash-owned reusable code lives in this repository under `components/mog_*`.
5. Small unavoidable upstream integration changes are represented as explicit, reviewable overlay/patch operations rather than undocumented manual edits.
6. Build tooling assembles the pinned foundation plus Firmware-mash overlay into one build workspace.
7. Generated/materialized foundation workspaces and build output are not committed as source-of-truth.
8. A release manifest records both Firmware-mash SHA and foundation SHA.
9. Updating the foundation pin requires a deliberate review, baseline regression and superseding/updated evidence; no floating branch is accepted.

## Consequences

### Positive

- preserves upstream board/test/simulator coherence;
- makes provenance obvious;
- reduces duplicate/fork drift;
- project-owned modules remain clearly separated;
- upstream upgrades can be evaluated as one pin change plus overlay compatibility;
- deterministic release metadata can identify both source layers.

### Costs

- a clean build needs the exact foundation materialized before target compilation;
- offline builders need a cached/exported copy of the pinned foundation;
- overlay integration must be tested against the exact pin;
- release CI must fail closed if the foundation SHA cannot be verified.

## Build safety rules

- never build from `main`, `master`, `latest` or another moving upstream ref;
- never silently apply an overlay to a different foundation SHA;
- foundation sync may use a local cache, but the cache must resolve to the exact pin;
- a failed patch/integration check aborts before compiling;
- no generated signing keys, credentials or materialized upstream checkout are committed accidentally;
- license/notice provenance remains part of release evidence.

## Phase 1A implication

The first storage work adapts the existing Bramble `msg_store`/SPIFFS persistence. Firmware-mash adds a transaction-safe generation/rollover layer and fault-injection tests rather than creating a second competing durable message store.

## Evidence / constraints

- baseline approval: `docs/BASELINE_APPROVED`;
- baseline report: `docs/BASELINE_EVIDENCE_2026-09-17.md`;
- provenance: `docs/BASELINE_PROVENANCE_2026-09-17.md`;
- repository layout: `docs/REPOSITORY_LAYOUT.md`.

## Supersedes / superseded by

None.