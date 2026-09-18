# Energy Integration Specialist Review — 2026-09-17

## Scope

Review the proposed addition of energy-aware firmware and future ambient-RF energy harvesting to Firmware-mash without weakening the T-Deck Plus communications core.

## Review team lenses

### 1. Embedded power / PMIC lens

Finding: power policy belongs in one dedicated manager. Battery/USB telemetry and low-power modes are useful on the stock T-Deck today; RF harvesting itself requires external hardware.

Required controls:

- hysteretic energy states;
- no raw ADC sample directly changing routing/UI policy;
- no high-frequency flash logging of power samples;
- no brownout-prone TX decision based on an unverified reservoir threshold;
- external provider failure must degrade to ordinary battery operation.

### 2. RF / antenna coexistence lens

Finding: a harvesting frontend can reduce communication performance if it loads or shares the communications antenna incorrectly.

Required controls:

- separate harvesting antenna/rectenna is the default physical assumption;
- shared antenna is an explicit future experiment only;
- measure insertion loss, matching impact, receiver desense and transmit isolation before any shared-RF design is promoted;
- harvested-energy measurements may not be used to claim communication gain.

### 3. Networking / routing lens

Finding: energy belongs in route scoring and relay policy as one bounded metric, not as a new routing authority.

Required controls:

- reliability remains more important than a small energy saving;
- low-battery handhelds may reduce relay/discovery/multipath work;
- externally powered/fixed nodes may receive a more favorable relay budget;
- energy transitions may make work eligible/deferred but cannot bypass ReliabilityManager, AirtimeManager, PacketId or dedup semantics;
- no retry storm when energy becomes available again.

### 4. Product / release lens

Finding: the feature must be split into a software capability and a hardware evidence tier.

STABLE software target:

- EnergyManager;
- battery/external-power state where board telemetry supports it;
- NORMAL/CONSERVE/CRITICAL/SURVIVAL policy;
- energy-aware relay/discovery/multipath budgets;
- UI status in ordinary language.

LAB hardware target:

- ambient-RF harvesting provider;
- rectenna/harvester-PMIC telemetry;
- optional supercap/TX reserve;
- real harvested-power evidence;
- RF coexistence evidence.

## Current external technical evidence

Energy-harvesting PMICs exist specifically for weak/intermittent sources. Examples include RF-capable devices such as the e-peas AEM30940, which supports RF/AC sources through adapted rectifiers and MPPT, and ultra-low-power harvesting PMICs such as TI BQ25570, which supports energy storage and very-low-power operation. These are architectural references, not a frozen BOM.

Published energy-harvesting work also shows that intermittent LoRa operation can be scheduled around capacitor charge state. This supports a threshold/hysteresis architecture, but it does not prove that a complete T-Deck Plus can run from ambient RF. The T-Deck remains battery-powered; ambient RF is treated as optional energy assist until measured otherwise.

## Decisions

1. Add `mog_energy` to the normal implementation plan.
2. Add `mog_energy_rf_harvest` as an optional LAB provider.
3. Keep harvesting physically/logically separate from `TransportAdapter`.
4. Add EnergyPolicySnapshot as the only normal interface consumed by router/UI/power schedulers.
5. Add power/energy events and traceable tests.
6. Add build configuration proving the stable firmware remains complete with RF harvesting OFF.
7. Add an Advanced UI power/energy diagnostics surface only when data exists.
8. Do not freeze PMIC, rectenna, antenna or supercapacitor values before hardware characterization.

## Red-team failure cases

### Harvest source disappears during TX preparation

Expected behavior: operation is re-evaluated/deferred or uses normal battery reserve; durable message state remains correct; no false Delivered state.

### Bad ADC/PMIC sample reports impossible energy

Expected behavior: reject/low-confidence sample, preserve last safe policy, increment health metric.

### Energy returns after a long outage

Expected behavior: bounded work becomes eligible; retry/discovery remains backoff/airtime controlled.

### RF harvesting hardware is absent

Expected behavior: provider reports unavailable; normal T-Deck boot/messaging/routing is unchanged.

### Harvesting frontend hurts LoRa sensitivity

Expected behavior: hardware cannot be promoted; harvesting remains LAB/disabled.

### Very low battery while acting as relay

Expected behavior: node reduces opportunistic transit work while preserving own durable messages and policy-permitted user-originated communication.

## Verdict

**ARCHITECTURE-READY FOR SOFTWARE IMPLEMENTATION; RF-HARVEST HARDWARE REMAINS LAB UNTIL MEASURED.**

The new software layer can be implemented in the same one-shot coding project without waiting for harvesting hardware because the provider boundary is explicit and optional. Actual ambient-RF harvesting capability must not be labelled working until external hardware exists and passes the hardware power/RF gates.