# Energy Management and Ambient RF Harvesting Contract

## Purpose

Firmware-mash gains one coherent energy-management layer that can optimize a stock T-Deck Plus today and later accept external energy-harvesting hardware without coupling power electronics to routing, messaging or UI internals.

The software architecture is implemented in the normal coding pass. Actual ambient-RF harvesting remains hardware-dependent and cannot be advertised as working on a stock T-Deck Plus until external rectenna/PMIC/storage hardware is built and measured.

## Physical truth and product boundary

- Firmware alone cannot turn the stock T-Deck Plus into an ambient-RF energy harvester.
- Ambient RF harvesting requires compatible external RF hardware: harvesting antenna/rectenna, matching/rectifier stage, energy-harvesting PMIC and an approved storage element such as a capacitor/supercapacitor or supported rechargeable cell interface.
- Harvested power is opportunistic and environment-dependent. The system must never assume a guaranteed RF energy rate.
- Ambient RF energy is treated only as energy. Firmware-mash does not need to demodulate, decode or consume third-party traffic to harvest RF power.
- LoRa/SX1262 communications remain fully functional when all harvesting hardware and harvesting software are absent or disabled.
- The stable product must never depend on ambient RF energy to boot, store messages, route, receive or transmit.

## Architecture

```text
Battery / USB / charger telemetry
             |
             +------------------+
                                v
                        +---------------+
External harvest HW --> | EnergyManager | --> EnergyPolicySnapshot
(optional LAB provider) +-------+-------+
                                |
             +------------------+------------------+
             |                  |                  |
             v                  v                  v
        UI/power state    HybridRouter input   Power policies
                           (energy cost only)   display/GNSS/
                                                discovery/relay
```

`EnergyManager` is the only policy authority for device energy state. Providers report measurements/capabilities; they do not make routing decisions.

`HybridRouter` may consume normalized energy cost/relay-budget information. It must not control PMICs, charge paths, supercapacitors or harvesting switches directly.

## Planned modules

```text
components/mog_energy/
  include/mog_energy.h
  mog_energy.c/.cpp
  mog_energy_policy.c/.cpp
  mog_energy_metrics.c/.cpp

components/mog_energy_rf_harvest/        # optional/LAB
  include/mog_energy_rf_harvest.h
  mog_energy_rf_harvest.c/.cpp
  mog_energy_rf_provider.c/.cpp
```

The exact foundation power/ADC/charger hooks are adapted after baseline inspection.

## Energy sources

V1 recognizes at least:

```text
BATTERY
USB_OR_EXTERNAL_POWER
RF_HARVEST        optional external hardware
FUTURE_HARVESTER  reserved provider class
```

Solar or other future harvesters may use the same provider model; they must not create parallel power-policy engines.

## Energy state model

Use explicit state with hysteresis; do not flap on single ADC samples.

Conceptual states:

```text
EXTERNAL_POWER
NORMAL
CONSERVE
CRITICAL
SURVIVAL
```

Exact voltage/percentage/power thresholds are not hard-coded in this contract. They must be derived from the selected battery/board/PMIC and hardware measurements.

### NORMAL

Normal UI and validated networking behavior.

### CONSERVE

May reduce display brightness/timeout, GNSS duty cycle, background discovery frequency and non-essential telemetry while preserving normal messaging semantics.

### CRITICAL

Protect delivery-critical state. Avoid wasteful multipath/discovery, reduce relay willingness and preserve enough reserve for controlled communication where possible.

### SURVIVAL

Aggressive sleep/low-duty behavior may be used, but durable queued messages and identity remain intact. The device must never falsely report a message Delivered merely because transmission was deferred for energy reasons.

## Provider contract

A provider reports observations, never policy.

Conceptual sample:

```c
typedef struct {
    bool available;
    bool external_power;
    uint32_t source_flags;
    uint32_t voltage_mv;
    int32_t current_ua;          // unknown allowed
    int32_t power_uw;            // unknown allowed
    uint32_t storage_voltage_mv; // e.g. supercap/harvester reservoir, optional
    uint8_t confidence;
    uint32_t sample_age_ms;
} mog_energy_sample_t;
```

Conceptual provider operations:

```text
init()
available()
sample()
capabilities()
health()
```

Unknown measurements remain unknown; firmware must not fabricate harvested-current or power values.

## EnergyPolicySnapshot

Consumers read an immutable normalized snapshot instead of raw PMIC state.

Conceptual fields:

```text
energy_state
battery_percent_if_known
external_power_present
harvest_available
harvest_power_class
relay_budget
background_discovery_budget
multipath_budget
nonessential_ui_budget
energy_cost_bias
```

The snapshot is advisory to networking except where a board-safety/brownout guard must reject an operation.

## Routing integration

Energy is one route metric, never the sole route metric.

A path remains evaluated by reliability, airtime, congestion, freshness, hop count, stability/diversity and energy cost. Energy-aware routing must not choose an unreliable route only because it is cheaper.

Relay policy may prefer externally powered/fixed nodes and reduce transit traffic through critically low-battery handhelds. User-originated messages retain priority over opportunistic relay work according to policy.

Route-score weights remain versioned and evidence-driven.

## TX reserve and intermittent energy

If future external harvesting hardware exposes an energy reservoir such as a supercapacitor, firmware may model a `TX_RESERVE_READY` capability/event.

Rules:

- a readiness threshold must include hysteresis and measured TX headroom;
- no exact voltage threshold is frozen before hardware characterization;
- a reservoir-ready event makes a deferred transmission eligible for policy evaluation; it does not bypass ReliabilityManager or AirtimeManager;
- a failed/brownout-prone energy source must degrade safely to ordinary battery/LoRa behavior;
- persistent message state is committed before an energy-constrained TX attempt where delivery durability requires it.

## RF/antenna isolation

Preferred hardware architecture uses a separate harvesting antenna/rectenna from the tuned EU868 communications antenna.

Sharing an antenna is not assumed safe or beneficial. Any shared-antenna design requires measured insertion loss, impedance impact, receiver desense, transmit isolation, switch behavior and regional-compliance review before it can be enabled.

The harvesting path must not silently degrade SX1262 range or sensitivity.

## Power scheduling integration

`EnergyManager` may publish budgets consumed by:

- display/backlight policy;
- GNSS sampling policy;
- ESP-NOW discovery cadence;
- optional Wi-Fi/BLE background work;
- relay willingness;
- multipath duplication allowance;
- metrics/reporting frequency;
- sleep/idle decisions.

It must not change PacketId, delivery semantics, encryption boundaries or message TTL merely to save energy.

## User experience

Normal UI remains simple. The user does not manage rectifier voltages, MPPT points or TX-reservoir thresholds.

Normal surfaces may show:

```text
Battery
Charging / External power
Energy saving
Survival mode
Energy assist active     # only when actual compatible hardware is detected
```

Raw harvested micro-watts, supercap voltage and provider diagnostics belong in Advanced Diagnostics.

## Metrics

At minimum expose bounded/rolling metrics for:

```text
energy_state transitions
external-power time
estimated energy-policy deferrals
relay reductions due to energy policy
harvest provider availability
harvest sample confidence
TX-reserve-ready events
brownout/undervoltage warnings where board telemetry supports them
```

Do not persist high-frequency power telemetry to internal flash.

## Failure isolation

Failure of an RF-harvest provider must result in `unavailable/degraded`, not a boot failure.

Removing the harvesting component at build time must leave CFG-LORA-STABLE behavior intact.

Bad/implausible sensor values are rejected or marked low-confidence. A provider cannot directly disable the required LoRa backbone.

## Evidence tiers

### STABLE software

- EnergyManager abstraction;
- battery/external-power state where supported by the board foundation;
- hysteretic NORMAL/CONSERVE/CRITICAL/SURVIVAL policy;
- energy-aware relay/multipath/discovery budgets;
- no dependency on harvester hardware.

### LAB hardware integration

- external RF harvesting provider;
- rectenna/PMIC telemetry;
- optional supercap/TX reserve behavior;
- measured harvested energy and RF coexistence/isolation.

RF harvesting may move above LAB only after repeatable real-hardware measurements prove useful net energy without unacceptable communication loss, instability or safety/compliance regressions.

## Acceptance principles

The implementation is acceptable only if:

1. stock T-Deck firmware builds/runs with harvesting OFF;
2. removing the RF-harvest provider causes no LoRa or messaging regression;
3. energy-state transitions are hysteretic and bounded;
4. energy policy cannot cause duplicate user delivery or false delivery state;
5. simulated intermittent energy cannot create retry/discovery storms;
6. real harvesting hardware, when eventually connected, is measured rather than assumed;
7. any RF frontend interaction is validated for communication performance before release.