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
```

The approval commit must also add a human-readable baseline report containing:

- audit date/time;
- foundation branch/tag and 40-char commit SHA;
- toolchain/ESP-IDF version;
- exact T-Deck Plus build target/config;
- firmware flash size;
- static/dynamic RAM figures available from the build;
- test commands and real output summary;
- simulator commands and real output summary;
- LoRa region/config used for baseline;
- known failing/skipped tests and why;
- upstream license text/hash/reference;
- upstream limitations relevant to our architecture;
- clean working-tree/reproducibility notes.

A baseline with hidden failures must use `FAIL` and may not create the approval marker.
