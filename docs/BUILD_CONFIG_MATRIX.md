# Build and Feature Configuration Matrix

## Goal

Every significant feature must compile in at least one CI configuration and optional features must prove they can be removed without breaking the stable core.

## Canonical configurations

### CFG-BASELINE

Purpose: reproduce pinned upstream T-Deck Plus foundation before Firmware-mash feature changes.

```text
LoRa: upstream baseline
HybridRouter: off/not yet introduced
MessageStore extension: off
EnergyManager extension: off
Multipath: off
ESP-NOW: off
ESP-NOW LR: off
IP backhaul: off
Gateway federation: off
Custody/store-carry-forward: off
NAN: off
RF harvest/assist/backscatter: off
microSD requirement: forbidden for core acceptance
```

### CFG-LORA-STABLE

Purpose: permanent stable-core regression target.

```text
LoRa: ON
HybridRouter: ON
Internal MessageStore: ON
EnergyManager: ON
Battery/external-power provider: ON where board telemetry exists
Reliability/dedup: ON
Multipath/failover: ON after validated
ESP-NOW: OFF
IP backhaul: OFF
Gateway federation: OFF
Custody/store-carry-forward: OFF
RF-harvest provider: OFF
NAN/RF assist/backscatter: OFF
microSD: OPTIONAL only
```

This configuration must remain fully functional on stock T-Deck Plus hardware with no Internet, cellular modem, custody feature, harvesting accessory or microSD dependency.

### CFG-HYBRID-BETA

Purpose: compile/test hybrid local transport integration.

```text
LoRa/HybridRouter/MessageStore/EnergyManager/Reliability: ON
Multipath/failover: ON
ESP-NOW Normal: ON
ESP-NOW LR: ON when target/IDF support is confirmed
RadioScheduler: ON
IP backhaul: OFF unless a combined scenario enables it
Custody: OFF unless a combined scenario enables it
RF-harvest provider: OFF
```

### CFG-IP-BETA

Purpose: T-Deck Internet-assisted delivery without cellular hardware dependency.

```text
LoRa/HybridRouter/MessageStore/EnergyManager/Reliability: ON
mog_transport_ip: ON
Wi-Fi NetifProvider: ON
Cellular NetifProvider: OFF
GatewayManager/GatewayDiscovery: ON
Federation client/outbound session: ON
Public inbound gateway listener: OFF
ESP-NOW: optional per scenario
Custody: OFF by default
RF-harvest provider: OFF
```

Must prove Internet loss leaves off-grid operation intact and the same PacketId/chat survives radio/IP path changes.

### CFG-GATEWAY-BETA

Purpose: gateway-class target bridging local Firmware-mash reachability to federated IP peers.

```text
HybridRouter: ON
LoRa: ON where target has LoRa
MessageStore: ON when gateway store-forward enabled
EnergyManager: ON where target exposes power state
mog_transport_ip: ON
GatewayManager/GatewayDiscovery: ON
Federation client: ON
Federation listener/server role: ON only on reviewed gateway-class target
Wi-Fi/Cellular/Ethernet NetifProvider: target-specific
Session/peer/advertisement limits: explicit and bounded
Custody: optional only when CFG-CUSTODY-BETA requirements also pass
```

Gateway builds may have different resource budgets from handhelds but cannot define a second message protocol/routing authority.

### CFG-CUSTODY-BETA

Purpose: compile/test opportunistic durable store-carry-forward custody.

```text
LoRa: ON
HybridRouter: ON
Internal MessageStore: ON
Reliability/dedup: ON
EnergyManager: ON
mog_custody or equivalent custody state machine: ON
Custody durable metadata/recovery: ON
Single-active-holder bounded ownership policy: ON
ESP-NOW/IP: optional per test scenario
Unbounded replication: FORBIDDEN
```

This configuration must pass CUS-001..CUS-010 and preserve exactly-once application presentation. The same commit must prove custody can be OFF while sender-side WAITING_ROUTE still functions.

### CFG-ENERGY-LAB

Purpose: compile/test Ambient RF Energy Assist software seam without implying hardware validation.

```text
LoRa/HybridRouter/MessageStore/EnergyManager: ON
RF-harvest provider interface: ON
RF-harvest mock/simulator provider: ON
Real RF-harvest provider: OFF unless a validated target exists
ESP-NOW/IP: optional per scenario
```

### CFG-LAB

Purpose: isolated experiments only. May enable NAN/advanced NAT traversal/RF-assist/external backscatter/target-specific RF-harvest hardware, but must always be removable back to CFG-LORA-STABLE.

## Feature ownership

Feature flags control adapters/capabilities/providers, not duplicate routing, reliability or energy engines.

- disabling ESP-NOW/IP/custody must not remove HybridRouter, PacketId, ReliabilityManager or sender MessageStore;
- disabling RF harvesting must not remove EnergyManager;
- ESP-NOW Normal/LR are modes of one adapter;
- Wi-Fi/cellular are bearer providers of one `mog_transport_ip`;
- GatewayManager is reachability/capability/session policy, not a router;
- custody is bounded ownership/reconciliation layered on ReliabilityManager/MessageStore, not a second delivery truth.

## Runtime defaults

Normal users never choose a route per message. Safe defaults:

- LoRa backbone enabled;
- automatic routing enabled;
- sender-side durable delayed delivery enabled;
- EnergyManager enabled;
- RF-harvest provider unavailable unless validated hardware exists;
- multipath/ESP-NOW/IP enabled only at evidence tier achieved by the release;
- cellular provider disabled unless selected modem hardware is validated;
- custody disabled in STABLE until its hardware promotion gate passes;
- high-level network policy defaults to `AUTO`; optional `OFF-GRID ONLY` and `INTERNET ASSIST` remain simple user policies;
- experimental transports disabled in STABLE.

## Compile gates

Once production code exists, CI must compile as applicable:

1. CFG-LORA-STABLE always;
2. CFG-HYBRID-BETA when ESP-NOW code exists;
3. CFG-IP-BETA when IP/gateway code exists;
4. CFG-GATEWAY-BETA when gateway listener/federation peer code exists;
5. CFG-CUSTODY-BETA when custody code exists;
6. CFG-ENERGY-LAB when harvesting provider seam exists;
7. host/unit test configuration;
8. simulator configuration.

The same commit must prove all optional beta/lab adapters/providers can be OFF while CFG-LORA-STABLE builds and sender delayed delivery remains functional.

## Release configuration rule

Release automation consumes generated build metadata. It does not introduce another feature configuration. A STABLE release may only package features whose build/test/hardware evidence gates have passed under `FLASHER_RELEASE_CONTRACT.md`.

## Version/pinning rule

Pin and record:

- foundation commit;
- ESP-IDF/toolchain versions;
- component/dependency locks where applicable;
- board target/config;
- partition table version/hash;
- packet/wire format version;
- route-score version;
- MessageStore record version;
- custody record/protocol version when enabled;
- EnergyPolicy version;
- IP-backhaul framing version;
- gateway advertisement/federation protocol version;
- selected modem/provider/BOM revision when cellular is introduced;
- target-specific harvester-provider revision/BOM when real hardware is introduced.

Do not silently float toolchain, protocol, custody, gateway/federation or energy-policy semantics in a stable release.
