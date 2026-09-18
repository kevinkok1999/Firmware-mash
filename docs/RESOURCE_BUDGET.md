# Resource Budget Contract

Firmware-mash targets a constrained embedded device. Features are admitted only when their flash, RAM/PSRAM, persistent-storage, airtime and power costs are measured and bounded.

## Rule: measure before assigning final capacities

Do not freeze queue lengths, route-table sizes, OTA slot sizes, MessageStore capacity, energy sampling cadence or TX-reservoir thresholds from guesses. Build Phase 1 records the real baseline image and memory footprint first. Capacities and power thresholds are then selected with explicit reserve and, where hardware-specific, measurement.

## Flash budget categories

The final partition table must account for, at minimum:

- bootloader / partition metadata required by ESP-IDF;
- durable configuration and identity storage;
- OTA metadata if OTA is enabled;
- application image slot(s);
- bounded internal MessageStore for pending/store-forward messages;
- optional compact crash/health evidence if useful;
- EnergyManager configuration/version metadata only where needed;
- unallocated safety margin for future firmware growth and migrations.

MicroSD is not part of the mandatory capacity calculation. High-frequency energy/harvest telemetry is not a durable-flash workload.

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
- EnergyManager snapshots/state and provider samples;
- optional RF-harvest mock/provider state;
- simulator/device diagnostics where applicable.

Routes, neighbor observations, RF histories and energy telemetry are volatile by default and should use RAM/PSRAM rather than flash unless a later ADR proves persistence is beneficial.

## Power budget categories

Measure where hardware permits:

- display/backlight active and idle cost;
- GNSS active/sleep duty cycle;
- LoRa RX/TX cost by relevant operating profile;
- ESP-NOW Normal/LR discovery and TX/RX cost;
- background scanning/coexistence overhead;
- EnergyManager sampling overhead;
- sleep/deep-sleep current where the selected foundation supports it;
- relay/multipath incremental cost;
- optional external harvester quiescent/measurement overhead when hardware exists.

Ambient RF Energy Assist must be evaluated on **net useful energy**, not just rectifier output. Provider/PMIC overhead and any RF communication penalty count against the gain.

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
energy sampling/provider overhead if relevant:
```

A feature that saves radio airtime but causes unbounded memory growth is not an improvement. A feature that fits in RAM but removes safe update/recovery margin is not an improvement. A harvester that produces some energy but reduces LoRa sensitivity or consumes comparable overhead is not an improvement.

## Pressure behavior

Every bounded resource needs deterministic exhaustion behavior. Examples:

- route table full -> deterministic stale/low-value candidate eviction;
- message store full -> priority/TTL-aware policy with visible counter/status;
- packet pool full -> explicit drop/backpressure metric, never silent heap expansion;
- ESP-NOW peer cache full -> evict cold peer, never grow unbounded;
- diagnostic buffer full -> drop diagnostics before delivery-critical state;
- EnergyManager provider unavailable -> mark unavailable and continue with stock power policy;
- invalid/stale energy sample -> reject/low-confidence, never trigger uncontrolled policy change.

## Baseline evidence to record

Before finalizing capacities record:

- detected flash capacity and partition table;
- application image size;
- static/internal RAM use;
- PSRAM availability/use;
- runtime free-heap/high-water values where available;
- NVS/config footprint;
- candidate durable MessageStore overhead;
- recovery/OTA margin;
- available stock battery/external-power telemetry hooks;
- baseline idle/active power measurements when hardware access permits.

## Ambient RF hardware evidence

When real harvesting hardware exists, record separately:

```text
harvester/PMIC/BOM revision:
harvesting antenna/front-end configuration:
input RF condition / measurement method:
measured harvested power/energy:
provider/PMIC quiescent overhead:
storage/reservoir behavior:
LoRa sensitivity/insertion-loss comparison:
TX brownout/power-integrity result:
net battery/runtime impact:
```

Do not mix simulator estimates with measured hardware results.

## Promotion gate

STABLE software features require:

1. bounded capacity;
2. explicit full/overflow policy;
3. measured resource impact;
4. no microSD dependency;
5. no RF-harvester dependency;
6. no regression that prevents LoRa-only recovery operation.

Ambient RF Energy Assist hardware remains LAB until its net-energy and RF-impact measurements are repeatable and acceptable.