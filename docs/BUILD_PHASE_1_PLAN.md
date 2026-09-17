# Build Phase 1 Runbook — Foundation Baseline

This runbook is the final **pre-implementation** gate. It is intentionally narrow: prove and pin the foundation before generating Firmware-mash production source around it.

## Objective

Produce a reproducible T-Deck Plus baseline and enough evidence to decide whether Bramble remains the correct foundation. No MessageStore/HybridRouter/multipath/ESP-NOW production work starts before this gate passes.

## Step 1 — Revalidate upstream immediately before import

1. Fetch the current Bramble default branch and releases/tags.
2. Re-check T-Deck Plus board support and build instructions in source.
3. Re-check `LICENSE` at the exact candidate commit.
4. Inspect changes since snapshot `23854fd883fd29da14d8c0876f6eca4e14fbb938`.
5. Re-check open issues/PRs touching T-Deck, routing, security, SX1262, simulator and build breakage.
6. If a newer candidate is selected, record why.

Do the same at a lighter level for MeshCore/LoRaMesher/Meshtastic/Pyxis/MeshCom/Reticulum so a newly merged feature does not invalidate our assumptions.

## Step 2 — Decide import strategy

Choose one and record it in an ADR:

- **Vendor/fork baseline:** import the selected foundation source and preserve upstream history/provenance as practical.
- **Component extraction:** only if the architecture proves a smaller set of MIT components can be reused cleanly without losing simulator/test value.

Default preference is to keep Bramble's component/test/simulator coherence rather than manually copying isolated files.

## Step 3 — Pin toolchain

Record exact versions for:

- ESP-IDF;
- Python;
- CMake/Ninja;
- compiler/toolchain;
- Go for simulator/gosim tests;
- Node/npm only if web/simulator UI requires it;
- any package manager lockfiles.

A second checkout should be able to reproduce the build from repository instructions.

## Step 4 — Build untouched upstream baseline

Before local architectural changes:

- build the exact T-Deck Plus target;
- save build logs as CI artifacts where practical;
- record flash image/application sizes;
- record static RAM/heap/PSRAM information exposed by the build;
- record warnings;
- do not hide warnings that might become errors later.

If the T-Deck target does not build cleanly, stop and diagnose rather than patching around it silently.

## Step 5 — Run upstream tests

Run every practical host/unit suite relevant to routing, packet, security, reliability, mailbox/store-forward and board-independent logic. Run simulator suites using the same selected commit.

Minimum baseline commands when supported by the pinned candidate:

```text
bash test/run_all_tests.sh
bash simulator/test-e2e.sh
cd simulator/gosim && go build ./... && go test -count=1 ./...
```

Record exact commands and pass/fail/skip counts.

## Step 6 — Baseline behavior/dependency matrix

Confirm from build/source or explicitly mark hardware-unverified:

- T-Deck board config/display/keyboard/trackball/touch paths;
- SX1262 LoRa initialization path;
- EU868 config path;
- ACK/retry semantics;
- route discovery/forwarding;
- store-forward/mailbox behavior;
- current persistence/reboot behavior;
- simulator parity claims;
- **core boot/messaging has no mandatory microSD dependency in the reviewed source/build configuration**.

Real boot/display/input/radio and no-SD behavior remain HARDWARE-UNVERIFIED until actual T-Deck hardware evidence exists. That hardware evidence is a release gate, not a reason to make the end user a development tester.

## Step 7 — Baseline fault review

Before approval, inspect known upstream failure areas we already identified:

- RERR must not cancel unrelated/direct routes;
- handshake/pending states need stale-state recovery;
- route timers/deadlines must be wrap-safe;
- queues/state machines must not stall indefinitely;
- control traffic and retry ladders must remain bounded.

If upstream already fixes these at the pin, note the exact source/tests. If not, create scoped follow-up issues after baseline approval; do not mix them into baseline reproduction.

## Step 8 — Create local provenance record

Add a baseline provenance document listing every imported upstream file/tree and its license source. Preserve MIT notices. Do not bring GPL/custom-license reference source into the tree just for convenience.

## Step 9 — Approve or reject foundation

Use the following gate:

**GO** if T-Deck support, build/test reproducibility, component boundaries, simulator/test leverage, security foundation, no-SD dependency review and licensing are all acceptable.

**CONDITIONAL GO** if there are bounded, understood defects with explicit issues and no architectural blocker.

**NO-GO** if the selected foundation cannot be reproduced, lacks reliable T-Deck support, creates unacceptable licensing/security constraints, has a mandatory microSD dependency for core operation, or requires more replacement than reuse.

## Step 10 — Only after GO

Create `docs/BASELINE_APPROVED` from `BASELINE_TEMPLATE.md` with real evidence fields, add the full human-readable report, and let CI unlock production-source directories.

Then the production implementation sequence begins with GitHub issue **#10** (standalone internal MessageStore/no-SD durability), followed by issue **#2** (HybridRouter LoRa-only seam), then the remaining roadmap/runbook stages.

## Explicit non-goals of the baseline gate

Do not yet implement:

- durable Firmware-mash MessageStore;
- HybridRouter changes;
- multipath;
- ESP-NOW;
- NAN;
- RF assist;
- backscatter;
- TDMA/regional routing;
- new crypto;
- OTA redesign;
- smartphone UI redesign.

The fastest path to strong firmware is a proven baseline, not maximum code volume before the foundation is measured.
