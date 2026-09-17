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
RF-harvest provider: OFF
Reliability/dedup: ON
Multipath/failover: ON after validated
ESP-NOW: OFF
ESP-NOW LR: OFF
IP backhaul: OFF
Gateway federation: OFF
NAN: OFF
RF assist/backscatter: OFF
microSD: OPTIONAL only
```

This configuration must remain fully functional on stock T-Deck Plus hardware with no harvesting accessory and no Internet connection.

### CFG-HYBRID-BETA

Purpose: compile/test hybrid local transport integration.

```text
LoRa: ON
HybridRouter: ON
Internal MessageStore: ON
EnergyManager: ON
RF-harvest provider: OFF by default
Reliability/dedup: ON
Multipath/failover: ON
ESP-NOW Normal: ON
ESP-NOW LR: ON when target/IDF support is confirmed
RadioScheduler: ON
IP backhaul: OFF unless a combined test explicitly enables it
NAN: OFF
RF assist/backscatter: OFF
```

### CFG-IP-BETA

Purpose: compile/test T-Deck Internet-assisted delivery without cellular hardware dependency.

```text
LoRa: ON
HybridRouter: ON
Internal MessageStore: ON
EnergyManager: ON
Reliability/dedup: ON
Multipath/failover: ON
mog_transport_ip: ON
Wi-Fi NetifProvider: ON
Cellular NetifProvider: OFF
GatewayManager: ON
GatewayDiscovery: ON
Federation client/outbound session: ON
Public inbound gateway listener: OFF
ESP-NOW: optional per scenario
RF-harvest provider: OFF
```

This configuration must prove that Internet loss leaves off-grid operation intact and that the same PacketId/chat survives IP/radio path changes.

### CFG-GATEWAY-BETA

Purpose: gateway-class target capable of bridging local Firmware-mash reachability to federated IP peers.

```text
LoRa: ON where target has LoRa
HybridRouter: ON
MessageStore: ON when gateway store-forward is enabled
EnergyManager: ON when target exposes relevant power state
mog_transport_ip: ON
GatewayManager: ON
GatewayDiscovery: ON
Federation client: ON
Federation listener/server role: ON only on reviewed gateway-class target
Wi-Fi/Cellular/Ethernet NetifProvider: target-specific
Session/peer/advertisement limits: explicit and bounded
```

Gateway builds may have different resource budgets from handhelds, but they cannot define a second message protocol or routing authority.

### CFG-ENERGY-LAB

Purpose: compile/test the Ambient RF Energy Assist software seam without implying real hardware validation.

```text
LoRa: ON
HybridRouter: ON
MessageStore: ON
EnergyManager: ON
RF-harvest provider interface: ON
RF-harvest mock/simulator provider: ON
Real RF-harvest hardware provider: OFF unless a specific validated target is selected
ESP-NOW: optional according to test scenario
IP backhaul: optional according to test scenario
NAN: OFF by default
RF assist/backscatter: OFF by default
```

A real harvesting-hardware build may add a target-specific provider configuration, but it must retain a safe hardware-absent/unavailable path and may not replace CFG-LORA-STABLE.

### CFG-LAB

Purpose: isolated experiments only.

May enable NAN/advanced NAT traversal/RF-assist/future external backscatter and target-specific RF-harvest hardware, but must retain ability to disable them and return to CFG-LORA-STABLE.

## Feature ownership

Feature flags control adapters/capabilities/providers, not duplicate routing or energy-policy engines. Disabling ESP-NOW or IP must not remove HybridRouter, PacketId, ReliabilityManager or MessageStore. Disabling RF harvesting must not remove EnergyManager.

ESP-NOW Normal and ESP-NOW LR are modes/capabilities of the same transport adapter. Wi-Fi and cellular are bearer providers of one `mog_transport_ip`, not separate user-selected messaging networks.

EnergyManager remains one stable policy authority; source providers are replaceable/optional measurement inputs. GatewayManager is a gateway reachability/capability service, not a second router.

## Runtime defaults

Normal end users do not select a build-time route mode or a route per message. Safe runtime defaults:

- LoRa backbone enabled;
- automatic routing enabled;
- durable delayed delivery enabled;
- EnergyManager enabled;
- stock battery/external-power telemetry used when available;
- RF-harvest provider disabled/unavailable unless compatible validated hardware is detected/configured;
- multipath enabled only after its validation gate passes;
- ESP-NOW auto mode enabled only in a build where hardware evidence supports it;
- ESP-NOW link-mode choice NORMAL/LR is automatic policy, not a required user setting;
- IP backhaul enabled only in release tiers/configurations whose Wi-Fi/gateway evidence passes;
- cellular provider disabled unless the selected modem target is validated;
- normal high-level network policy is `AUTO`; `OFF-GRID ONLY` and `INTERNET ASSIST` may be exposed as simple user policies;
- experimental transports disabled in STABLE.

## Compile gates

Once production code exists, CI must compile at minimum:

1. CFG-LORA-STABLE;
2. CFG-HYBRID-BETA when ESP-NOW code exists;
3. CFG-IP-BETA when IP/gateway code exists;
4. CFG-GATEWAY-BETA when gateway listener/federation peer code exists;
5. CFG-ENERGY-LAB when the harvesting-provider seam exists;
6. host/unit test configuration;
7. simulator configuration.

A feature hidden behind a flag is not considered covered unless at least one CI job turns that flag ON.

The same commit must also prove that `mog_energy_rf_harvest`, `mog_transport_ip`, gateway federation and cellular providers can be OFF while CFG-LORA-STABLE still builds.

## Version/pinning rule

Pin and record:

- foundation commit;
- ESP-IDF version;
- component/dependency lock where applicable;
- board target/config;
- partition table version;
- packet/wire format version;
- route-score version;
- MessageStore record version;
- EnergyPolicy version;
- IP-backhaul protocol/framing version;
- gateway-advertisement/federation protocol version;
- selected modem/provider/BOM revision when cellular is introduced;
- target-specific harvester-provider revision/BOM identifier when real hardware is introduced.

Do not silently float the toolchain, protocol format, gateway/federation semantics or energy-policy semantics in a stable release.
