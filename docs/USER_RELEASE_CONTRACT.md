# User Release Contract — One-Flash Stable Experience

## User requirement

The intended end user must not be required to perform engineering validation such as packet-loss measurements, route-failover experiments, RF tuning, peer-table debugging, flash-wear testing, storage-corruption testing or power-electronics tuning before the firmware is useful.

The expected user flow is:

1. obtain one approved T-Deck Plus release package;
2. flash it once using the supported flasher;
3. complete only normal first-run configuration;
4. use messaging, multipath failover, store-and-forward, EnergyManager and enabled hybrid transports without engineering intervention.

## Release principle

Testing responsibility belongs to the project before a build is labelled STABLE. A user should not be used as the primary test harness.

This does **not** mean real-world RF or power behavior can be guaranteed in every environment. Range, interference, antenna orientation, terrain, buildings, battery condition, ambient RF availability and legal regional constraints remain physical variables. STABLE means the software behavior has passed the defined evidence gates under documented conditions.

## Required pre-release evidence

A STABLE candidate must pass all applicable automated and hardware gates before publication:

- reproducible T-Deck Plus build from a pinned foundation/toolchain;
- no-microSD boot and normal operation;
- internal MessageStore durability and reboot recovery;
- power-loss/corrupt-tail recovery tests;
- LoRa-only regression tests;
- multipath primary/backup route tests;
- cached failover before broad rediscovery;
- duplicate suppression and end-to-end ACK correlation;
- bounded high-loss/retry behavior;
- queue/pool exhaustion behavior;
- simulator congestion and route-flapping scenarios;
- EnergyManager state/hysteresis and low-energy-policy tests;
- stable build with all RF-harvesting providers disabled/absent;
- ESP-NOW direct and hybrid forwarding hardware tests before ESP-NOW is enabled in STABLE;
- 2.4 GHz coexistence tests for every enabled Wi-Fi/ESP-NOW/BLE combination;
- flash/RAM/PSRAM high-water measurements;
- battery/power impact measurements for stable power-management behavior where hardware telemetry/testing permits;
- internal-storage wear/health review;
- recovery flashing procedure;
- firmware integrity/release hashes;
- regulatory/region configuration review.

Ambient RF harvesting is **not** a STABLE release requirement. If advertised as a hardware feature, it needs its own LAB/BETA evidence package covering measured harvested energy, hardware compatibility and communication impact.

## Hardware-in-the-loop rule

Simulator success alone is not enough for a hardware-facing feature to be called STABLE.

For features such as LoRa, ESP-NOW, display/input, battery behavior and internal persistent storage, at least one repeatable hardware test on real T-Deck Plus units is required before stable promotion.

For Ambient RF Energy Assist, real rectenna/harvester-PMIC/storage hardware is additionally required. Firmware-only or mock-provider success cannot be presented as proof that a stock T-Deck harvests ambient RF.

If project maintainers do not have access to the necessary hardware test setup, that feature remains BETA/LAB rather than shifting the engineering burden to the end user.

## Release package

The final normal-user release should be a single coherent package containing at minimum:

- T-Deck Plus merged firmware image;
- manifest/version information;
- SHA-256 hashes;
- supported region/board declaration;
- release notes with evidence tier;
- supported one-click/web flashing path where practical;
- recovery instructions.

The user must not have to manually combine separate LoRa, routing, ESP-NOW, energy-management or storage binaries.

## Safe defaults

The default release configuration must favor reliability over maximum experimental capability.

- LoRa backbone: enabled.
- validated multipath/failover: enabled once proven.
- internal durable MessageStore: enabled.
- EnergyManager: enabled with validated stock-board power telemetry/policy.
- microSD: optional only.
- RF-harvest provider: disabled/unavailable unless a compatible validated accessory is explicitly supported.
- ESP-NOW: enabled in STABLE only after repeatable T-Deck hardware validation; otherwise BETA build only.
- NAN/RIS/backscatter: disabled in STABLE until separately proven.
- debug logging: bounded and non-disruptive.

## Self-diagnostics

Because normal users are not expected to perform network or power engineering, the firmware should expose simple health states such as:

- Network OK;
- Weak link;
- Searching for route;
- Message queued;
- Delivered;
- Storage warning;
- Radio degraded;
- Energy saving;
- Survival mode;
- Energy assist active only when compatible detected hardware provides credible measurements;
- Recovery required.

Advanced metrics may exist behind a diagnostics screen but must not be necessary for basic operation.

## No hidden tuning requirement

A STABLE release must not depend on users manually selecting:

- LoRa vs ESP-NOW;
- route number;
- next hop;
- retry counts;
- routing weights;
- peer cache entries;
- packet-loss thresholds;
- EnergyManager thresholds;
- rectifier/MPPT parameters;
- supercapacitor/reservoir thresholds.

Those choices belong to firmware policy, validated defaults or hardware-specific engineering profiles.

## Optional energy accessory rule

A normal STABLE T-Deck remains a complete product without an RF-energy-harvesting accessory. If an accessory is later supported, the same firmware architecture may detect/use it through the EnergyManager provider interface, but removal/failure of that accessory must safely fall back to ordinary battery operation.

## Final release gate

A release may be called STABLE only when its evidence package shows that the user can flash the approved T-Deck Plus image and use the advertised stable features without performing developer-level validation themselves.

Ambient RF Energy Assist may only be advertised at the evidence tier actually achieved by real compatible hardware.