# Bramble Foundation Candidate Notes

Audit snapshot: 2026-09-17

Candidate upstream commit observed during preparation:

`23854fd883fd29da14d8c0876f6eca4e14fbb938`

This file is a reproducibility aid, not the final baseline pin. Build Phase 1 must re-check upstream immediately before import and record the exact selected commit in `docs/BASELINE_APPROVED` only after all gates pass.

## Why Bramble is currently the leading foundation candidate

At the audited commit Bramble already contains:

- an explicit LILYGO T-Deck Plus board profile (`main/boards/tdeck_plus.h`);
- ESP32-S3 + SX1262 support;
- a dedicated `sdkconfig.defaults.tdeck_plus`;
- LVGL v9 T-Deck Plus GUI support;
- T-Deck keyboard, trackball, touch/display integration;
- a board-aware build/flash wrapper;
- host tests;
- simulator/emulator infrastructure;
- routing/reliability/security/store-forward components in a modular ESP-IDF layout;
- a web-flasher/release-artifact path.

These capabilities must still be reproduced locally/CI before the foundation is approved.

## Exact upstream T-Deck Plus configuration observed

`main/boards/tdeck_plus.h` identifies the board as:

`LILYGO T-Deck Plus`

`sdkconfig.defaults.tdeck_plus` includes the T-Deck board selection and 240 MHz ESP32-S3 CPU default.

## ESP-IDF pin

Bramble's build documentation pins:

`ESP-IDF v5.4.1`

The exact tag, not a moving `v5.4` branch, is the intended source of truth for the audited upstream build.

Typical host setup from upstream documentation:

```bash
git clone --depth 1 -b v5.4.1 https://github.com/espressif/esp-idf.git "$HOME/esp/esp-idf"
cd "$HOME/esp/esp-idf"
git submodule update --init --recursive --depth 1
./install.sh esp32s3

export IDF_PATH="$HOME/esp/esp-idf"
source "$IDF_PATH/export.sh"
```

Build Phase 1 must verify the upstream `.esp-idf-version` still says the same thing before using this pin.

## Preferred T-Deck Plus build command

From the audited Bramble build documentation:

```bash
bash scripts/flash.sh local tdeck-plus build
```

Expected per-board build directory:

```text
build-tdeck-plus/
```

Expected board-specific SDK config:

```text
sdkconfig.tdeck-plus
```

Expected main firmware artifact:

```text
build-tdeck-plus/bramble.bin
```

## Flash command for later hardware validation

Do not flash during the baseline source-import step unless hardware validation is explicitly being performed.

Upstream command:

```bash
bash scripts/flash.sh local tdeck-plus flash /dev/ttyACM0
```

The actual serial device must be discovered rather than assumed.

## Containerized reproducible build

Bramble also documents a CI-equivalent Docker build route. This is preferred for our first reproducibility attempt because it reduces host-toolchain drift.

Build image:

```bash
docker build -t bramble/idf-node:v5.4.1 docker/firmware-builder
```

Build T-Deck Plus firmware:

```bash
docker run --rm \
  -v "$PWD":/workspace -w /workspace \
  --user "$(id -u):$(id -g)" \
  bramble/idf-node:v5.4.1 \
  bash -lc 'source $IDF_PATH/export.sh >/dev/null && bash scripts/flash.sh local tdeck-plus build'
```

The `--user` argument matters: upstream warns that otherwise the container can leave root-owned build files and signing-key material in the checkout.

## Host-side test entrypoint

At the audited commit:

```bash
bash test/run_all_tests.sh
```

Webapp test entrypoint where relevant:

```bash
cd webapp
npm ci
npm test
```

Build Phase 1 should additionally inspect simulator/emulator documentation and execute the routing/security/reliability suites that exercise the same component sources used by the device build.

## Evidence to capture

For the selected baseline commit record:

- exact 40-character upstream SHA;
- ESP-IDF/toolchain versions;
- exact build command;
- build exit status;
- firmware/image sizes;
- warnings;
- host-test pass/fail/skip counts;
- simulator pass/fail/skip counts;
- T-Deck Plus board config used;
- EU868 configuration path;
- license hash/reference;
- known upstream defects/limitations;
- whether hardware was actually flashed and tested.

## Important design lessons from current upstream

A recent Bramble fix at the observed candidate commit reinforces two rules already adopted by Firmware-mash:

1. A route error must only invalidate a route/pending delivery when the reported failed hop is actually relevant to that path.
2. Long-lived handshake/pending states require stale-state recovery; one lost response must not park communication forever.

Those behaviors become explicit regression tests before our multipath layer is promoted.

## Baseline decision

Current status:

**PROMISING — NOT YET APPROVED**

Approval requires issue #1 and `docs/PREBUILD_CHECKLIST.md` to pass. Until then, CI intentionally blocks production source directories.
