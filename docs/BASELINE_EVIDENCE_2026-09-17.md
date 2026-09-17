# Foundation Baseline Evidence — 2026-09-17

## Verdict

**GO — approved foundation with tracked limitations.**

This approval pins an untouched Bramble baseline for Firmware-mash implementation. It does not claim that Firmware-mash production code or real T-Deck field validation already exists.

## Foundation pin

```text
repository=justinlindh/bramble
commit=23854fd883fd29da14d8c0876f6eca4e14fbb938
esp_idf=v5.4.1
board=LILYGO T-Deck Plus
flash=16 MB
license=MIT
```

At approval time the upstream default branch still resolves to the selected commit, so the pin is not knowingly behind a newer upstream head.

## Reproducible build/test evidence

The exact pinned commit has a successful upstream GitHub Actions Quality run (`34956733144`) from a clean GitHub-hosted environment. Evidence observed for that exact SHA includes:

- T-Deck Plus board build: PASS;
- host/unit tests: PASS;
- gosim build/tests: PASS;
- parser fuzzing: PASS;
- full emulator suite: PASS;
- Docker simulator build: PASS.

Previously inspected output for the exact run recorded the T-Deck application image at approximately `0x17f780` bytes (~1.50 MiB) and substantial remaining app-partition margin. Final Firmware-mash release sizing is re-measured after each implementation layer; this baseline value is not reused as a future release claim.

Independent execution inside the current ChatGPT working container was not possible because Docker is unavailable and outbound Git clone/network access is restricted. Firmware-mash's own GitHub Actions jobs are also currently queued before runner assignment. These are recorded as infrastructure limitations, not silently relabelled as local PASS. The accepted build/test evidence is the exact upstream clean CI run at the pinned commit.

## Toolchain and target

The pinned source declares ESP-IDF `v5.4.1` in `.esp-idf-version`. The T-Deck Plus board configuration selects ESP32-S3, 16 MB flash and PSRAM.

The partition table is:

```text
nvs      0x009000 size 0x005000
otadata  0x00e000 size 0x002000
app0     0x010000 size 0x3c0000
app1     0x3d0000 size 0x3c0000
spiffs   0x7d0000 size 0x820000
```

This gives two OTA application slots plus a large internal SPIFFS data partition. Exact future Firmware-mash partition sizes remain evidence-driven and may change through a reviewed migration/ADR.

## EU868 / airtime path

The pinned source contains an explicit `BRAMBLE_REGION_EU868` selection and `FREQ_REGION_EU868` frequency plan. The EU868 plan identifies ETSI EN 300.220 and the upstream TX/airtime path contains duty-cycle enforcement. Firmware-mash still requires every future LoRa transmit path to remain behind the centralized AirtimeManager/TX gate.

## No-microSD dependency review

**PASS as a source/architecture gate.**

The T-Deck initialization attempts SD-card setup, but failure/absence produces a warning and boot continues. Core persistence reviewed below is on internal SPIFFS, not on removable microSD.

This is not a claim that physical no-SD cold boot and power-cycle recovery have already been tested on the user's T-Decks. Those remain mandatory hardware evidence before STABLE release.

## Existing internal persistence

The pinned T-Deck configuration already enables Bramble message persistence. The implementation uses internal SPIFFS and includes:

- bounded persistent records;
- record CRC/integrity checking;
- append + `fsync` behavior;
- recovery that can ignore/truncate an incomplete trailing record;
- bounded retention behavior.

Therefore Firmware-mash Phase 1A will **adapt and harden** this existing store rather than introduce a redundant second persistence engine.

## Tracked foundation limitation — compaction/rollover power loss

Source review found one important limitation in the current SPIFFS backend: rollover/compaction rewrites the active persistence file in-place after truncation. A power loss during that window can lose more committed history than Firmware-mash's durability contract permits.

This is bounded and understood, not an architecture blocker. It becomes the first Phase 1A production hardening task:

1. make rollover/compaction transactional using a shadow/journal/commit strategy appropriate to the measured filesystem;
2. preserve the last fully committed state across interruption;
3. add fault injection around append, rollover, commit and recovery;
4. keep identity/config isolated from message-store failure;
5. verify no-SD operation remains intact.

No higher-layer routing feature may be used to hide this defect.

## Security / routing baseline observations

The pinned commit includes upstream fixes for:

- path-relevant RERR fail-fast behavior;
- recovery of stale DM handshakes;
- wrap-safe elapsed-time handling in the affected state machine.

Firmware-mash keeps its own regression tests for these failure classes rather than relying permanently on upstream assumptions.

## Licensing

The pinned Bramble source is MIT-licensed. Required copyright/license notices will be preserved for reused/adapted upstream code. Reference-only GPL/custom-license projects are not copied into the MIT production tree.

## What this approval does not prove

Still hardware/evidence-gated:

- real T-Deck cold boot without microSD;
- physical LoRa range/reliability;
- power-cut behavior on real flash;
- ESP-NOW Normal/LR coexistence and range;
- Wi-Fi/IP gateway federation;
- cellular modem/eSIM integration;
- custody carry/reboot behavior;
- RF-harvest hardware;
- final battery life;
- final one-click flasher/recovery acceptance.

## Foundation decision

The baseline satisfies the Phase 0 purpose: a pinned, build-tested T-Deck Plus foundation with acceptable license/component boundaries, simulator leverage, EU868 support and no mandatory microSD dependency. Known persistence rollover risk is explicitly tracked as the first Phase 1A hardening task.

**Production implementation may now begin bottom-up from this pin.**