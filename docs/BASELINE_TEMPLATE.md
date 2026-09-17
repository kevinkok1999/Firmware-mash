# Baseline Approval Template

Do **not** rename this file to `BASELINE_APPROVED`. Build Phase 1 creates `docs/BASELINE_APPROVED` only after the evidence below is actually collected.

Required machine-readable fields for the future marker:

```text
foundation_repo=owner/repository
foundation_commit=0000000000000000000000000000000000000000
tdeck_build=PASS
baseline_tests=PASS
simulator_tests=PASS
license_review=PASS
no_sd_dependency_review=PASS
```

The approval commit must also add a human-readable baseline report containing:

- audit date/time;
- foundation branch/tag and 40-char commit SHA;
- toolchain/ESP-IDF version;
- exact T-Deck Plus build target/config;
- firmware flash size;
- static/dynamic RAM and PSRAM figures available from the build;
- current partition table and free/growth margin where available;
- test commands and real output summary;
- simulator commands and real output summary;
- LoRa region/config used for baseline;
- source/build review showing that core boot/messaging does not require microSD;
- current persistence behavior for identity/config/message queue;
- whether pending/store-forward messages survive reboot at baseline;
- known failing/skipped tests and why;
- upstream license text/hash/reference;
- upstream limitations relevant to our architecture;
- clean working-tree/reproducibility notes.

## Important distinction

`no_sd_dependency_review=PASS` is a **pre-coding architecture/source gate**, not a substitute for real hardware evidence. It means the pinned foundation does not make microSD a mandatory dependency for core boot/messaging according to the reviewed source/build configuration.

A real T-Deck Plus cold-boot/no-SD/power-cycle/message-recovery test remains mandatory before STABLE release under `STANDALONE_TDECK_REQUIREMENTS.md`, `TESTING.md` and `USER_RELEASE_CONTRACT.md`. The end user is not expected to perform that engineering validation.

The baseline is allowed to record that durable message persistence is not yet implemented; that becomes a required implementation milestone before STABLE. What is not allowed is silently depending on a microSD card or falsely marking volatile behavior as durable.

A baseline with hidden failures must use `FAIL` and may not create the approval marker.
