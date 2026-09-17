# Resource Budget Contract

Firmware-mash targets a constrained embedded device. Features are admitted only when their flash, RAM/PSRAM, persistent-storage and airtime costs are measured and bounded.

## Rule: measure before assigning final capacities

Do not freeze queue lengths, route-table sizes, OTA slot sizes or MessageStore capacity from guesses. Build Phase 1 records the real baseline image and memory footprint first. Capacities are then selected with explicit reserve.

## Flash budget categories

The final partition table must account for, at minimum:

- bootloader / partition metadata required by ESP-IDF;
- durable configuration and identity storage;
- OTA metadata if OTA is enabled;
- application image slot(s);
- bounded internal MessageStore for pending/store-forward messages;
- optional compact crash/health evidence if useful;
- unallocated safety margin for future firmware growth and migrations.

MicroSD is not part of the mandatory capacity calculation.

## OTA rule

Do not enable public OTA until the measured image size allows a safe rollback-capable partition strategy with meaningful growth headroom. If safe dual-image OTA does not fit alongside durable messaging and configuration, prefer USB/web recovery flashing over shrinking safety margins invisibly.

## RAM / PSRAM budget categories

Bound and measure:

- packet pools;
- RX/TX event queues;
- neighbor records;
- route sets and path candidates;
- pending ACK/reliability state;
- dedup/replay windows;
- ESP-NOW hot-peer cache;
- UI/LVGL buffers;
- temporary crypto/fragmentation buffers;
- simulator/device diagnostics where applicable.

Routes, neighbor observations and RF histories are volatile by default and should use RAM/PSRAM rather than flash unless a later ADR proves persistence is beneficial.

## Per-PR resource evidence

Once production code starts, implementation PRs must report where measurable:

```text
application image delta:
static RAM delta:
minimum free heap / high-water mark:
PSRAM impact:
persistent flash allocation delta:
new bounded table/queue capacity:
LoRa airtime/control-traffic impact:
power impact if hardware-measured:
```

A feature that saves radio airtime but causes unbounded memory growth is not an improvement. A feature that fits in RAM but removes safe update/recovery margin is not an improvement.

## Pressure behavior

Every bounded resource needs deterministic exhaustion behavior. Examples:

- route table full -> deterministic stale/low-value candidate eviction;
- message store full -> priority/TTL-aware policy with visible counter/status;
- packet pool full -> explicit drop/backpressure metric, never silent heap expansion;
- ESP-NOW peer cache full -> evict cold peer, never grow unbounded;
- diagnostic buffer full -> drop diagnostics before delivery-critical state.

## Baseline evidence to record

Before finalizing capacities record:

- detected flash capacity and partition table;
- application image size;
- static/internal RAM use;
- PSRAM availability/use;
- runtime free-heap/high-water values where available;
- NVS/config footprint;
- candidate durable MessageStore overhead;
- recovery/OTA margin.

## Promotion gate

STABLE features require:

1. bounded capacity;
2. explicit full/overflow policy;
3. measured resource impact;
4. no microSD dependency;
5. no regression that prevents LoRa-only recovery operation.
