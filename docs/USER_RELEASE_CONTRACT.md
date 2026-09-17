# User Release Contract — One-Flash Stable Experience

## User requirement

The intended end user must not be required to perform engineering validation such as packet-loss measurements, route-failover experiments, RF tuning, peer-table debugging, flash-wear testing or storage-corruption testing before the firmware is useful.

The expected user flow is:

1. obtain one approved T-Deck Plus release package;
2. flash it once using the supported flasher;
3. complete only normal first-run configuration;
4. use messaging, multipath failover, store-and-forward and enabled hybrid transports without engineering intervention.

## Release principle

Testing responsibility belongs to the project before a build is labelled STABLE. A user should not be used as the primary test harness.

This does **not** mean real-world RF behavior can be guaranteed in every environment. Range, interference, antenna orientation, terrain, buildings and legal regional constraints remain physical variables. STABLE means the software behavior has passed the defined evidence gates under documented conditions.

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
- ESP-NOW direct and hybrid forwarding hardware tests before ESP-NOW is enabled in STABLE;
- 2.4 GHz coexistence tests for every enabled Wi-Fi/ESP-NOW/BLE combination;
- flash/RAM/PSRAM high-water measurements;
- internal-storage wear/health review;
- recovery flashing procedure;
- firmware integrity/release hashes;
- regulatory/region configuration review.

## Hardware-in-the-loop rule

Simulator success alone is not enough for a hardware-facing feature to be called STABLE.

For features such as LoRa, ESP-NOW, display/input, battery behavior and internal persistent storage, at least one repeatable hardware test on real T-Deck Plus units is required before stable promotion.

If project maintainers do not have access to the necessary hardware test setup, that feature remains BETA/EXPERIMENTAL rather than shifting the engineering burden to the end user.

## Release package

The final normal-user release should be a single coherent package containing at minimum:

- T-Deck Plus merged firmware image;
- manifest/version information;
- SHA-256 hashes;
- supported region/board declaration;
- release notes with evidence tier;
- supported one-click/web flashing path where practical;
- recovery instructions.

The user must not have to manually combine separate LoRa, routing, ESP-NOW or storage binaries.

## Safe defaults

The default release configuration must favor reliability over maximum experimental capability.

- LoRa backbone: enabled.
- validated multipath/failover: enabled once proven.
- internal durable MessageStore: enabled.
- microSD: optional only.
- ESP-NOW: enabled in STABLE only after repeatable T-Deck hardware validation; otherwise BETA build only.
- NAN/RIS/backscatter: disabled in STABLE until separately proven.
- debug logging: bounded and non-disruptive.

## Self-diagnostics

Because normal users are not expected to perform network engineering, the firmware should expose simple health states such as:

- Network OK;
- Weak link;
- Searching for route;
- Message queued;
- Delivered;
- Storage warning;
- Radio degraded;
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
- packet-loss thresholds.

Those choices belong to the firmware policy and validated defaults.

## Final release gate

A release may be called STABLE only when its evidence package shows that the user can flash the approved T-Deck Plus image and use the advertised stable features without performing developer-level validation themselves.
