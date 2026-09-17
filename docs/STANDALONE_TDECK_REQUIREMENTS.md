# Standalone T-Deck Plus Requirements

## Hard requirement

Firmware-mash must operate as a complete off-grid messaging device on a stock LILYGO T-Deck Plus **without a microSD/TF card installed**. The card slot may later be used for optional exports, maps or debug archives, but no stable messaging, identity, routing, queueing, recovery or boot function may depend on it.

## Hardware envelope

The target T-Deck Plus provides an ESP32-S3, 16 MB internal flash and 8 MB PSRAM. Core software must fit entirely inside those onboard resources. PSRAM is for volatile runtime data; durable state belongs in internal flash.

## Mandatory standalone capabilities

With no microSD inserted, the device must still be able to:

- boot to the full local UI;
- retain identity, keys and configuration across reboot;
- send and receive LoRa messages;
- route/relay messages;
- maintain bounded multipath state;
- use ESP-NOW when that feature is enabled;
- queue outgoing messages when no route exists;
- retain queued messages across reboot/power loss once persistent MessageStore lands;
- recover the message store after interrupted writes;
- show network/queue/health status locally on the T-Deck;
- recover/reflash through USB without requiring a card;
- run in LoRa-only mode if all experimental transports are disabled.

## Storage tiers

### Tier 1 — internal NVS

Use NVS for small durable values such as:

- device identity references and keys/security metadata;
- region/radio configuration;
- user preferences;
- feature flags;
- compact monotonic/security counters where suitable;
- small recovery metadata.

NVS is not the default place for a high-churn message log.

### Tier 2 — internal durable message store

Store-and-forward and locally pending messages require a dedicated internal-flash design. Candidate implementations must be benchmarked after the baseline build is pinned.

Preferred properties:

- append/journal oriented;
- checksummed records;
- bounded storage;
- deterministic TTL/priority eviction;
- power-loss recovery;
- wear-aware compaction;
- encrypted payloads at the existing end-to-end security boundary;
- small RAM index reconstructed at boot;
- corruption of the message store must not erase identity/configuration.

Evaluate a power-loss-safe filesystem such as LittleFS versus a purpose-built append-only journal. Do not select a filesystem solely for convenience. FATFS with wear levelling is acceptable for non-critical bulk data but is not the preferred first choice for a critical queue unless its sudden-power-loss behavior is proven acceptable for our access pattern.

### Tier 3 — RAM / PSRAM only

Keep high-churn transient state out of flash whenever possible:

- neighbor table;
- RSSI/SNR samples;
- route candidates;
- route scores;
- dedup hot cache;
- packet/event pools;
- ESP-NOW hot-peer cache;
- RF intelligence windows;
- UI caches.

Critical state may be checkpointed only when there is a demonstrated recovery need; do not persist every metric update.

### Tier 4 — optional microSD

The microSD slot is optional only. Permitted future uses include:

- manual diagnostic export;
- optional offline map packs;
- optional long-term logs;
- user-imported assets.

Removing the card at runtime must not break core messaging. No identity, key, required route state or undelivered core-message queue may live only on the card.

## Partition strategy

Do not freeze exact partition sizes until the untouched baseline image size is measured. The 16 MB flash layout must reserve space for:

- bootloader/partition table;
- NVS and security/NVS-key material;
- OTA data;
- two application slots if safe A/B OTA is retained;
- a durable internal message-store partition;
- small recovery/crash metadata if useful.

The selected layout must leave explicit growth margin for both application images and the message store. A feature that consumes the OTA/recovery margin is not free.

## OTA/recovery rule

OTA is not required for the first stable routing milestone, but the partition plan must not paint us into a corner. Safe OTA requires two app slots and rollback/recovery semantics. USB flashing remains the lowest-level recovery path.

A new image must not be marked healthy merely because `app_main()` started. Future OTA validation should confirm at least internal storage mount/recovery, display/input task health and LoRa initialization before application-controlled confirmation.

## Flash endurance rules

- no persistent write per received RSSI/SNR sample;
- no persistent write per route-score update;
- batch/coalesce non-critical settings writes;
- append rather than repeatedly rewrite large structures;
- expose message-store write/compaction counters in diagnostics;
- test full-store and repeated-compaction behavior;
- keep identity/config partitions isolated from high-churn queue data.

## Power-loss requirements

Test power interruption during:

- queued-message append;
- message deletion/ACK processing;
- compaction;
- first boot after firmware update;
- configuration update.

After reboot, the device must either recover the last committed state or discard only the incomplete record. It must not enter a reboot loop because the queue is damaged.

## Battery / radio behavior

Real-world usability also requires bounded power draw. ESP-NOW/Wi-Fi/BLE share the ESP32-S3 2.4 GHz radio; discovery/scanning must therefore be scheduled rather than permanently aggressive. The stable device must remain useful with ESP-NOW disabled and must have display/radio idle policies measured on real hardware.

## No-SD acceptance gate

A release cannot be promoted to STABLE unless a T-Deck Plus with **no microSD card installed** passes:

1. cold boot;
2. identity/config persistence;
3. local UI/input;
4. LoRa send/receive;
5. multi-hop relay;
6. queue message with destination offline;
7. power cycle;
8. recover queued message;
9. destination returns;
10. queued message is delivered once;
11. internal store near-full behavior is deterministic;
12. USB recovery remains available.

MicroSD-dependent tests may exist only as optional feature tests.
