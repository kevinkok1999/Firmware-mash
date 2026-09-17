# Requirement-to-Test Traceability

## Purpose

Every stable requirement maps to at least one concrete test. This prevents implementation from drifting away from the architecture while still claiming completion.

| Requirement | Primary owner | Required evidence |
|---|---|---|
| No microSD required for core operation | standalone/storage | no-SD cold boot + send/receive + queue/reboot/recover |
| One logical PacketId across retries/transports | packet/reliability | duplicate-path + retry + transport-switch tests |
| Conversation identity independent from transport | UI/message/packet | LoRa/IP/ESP-NOW mixed-path conversation test |
| One routing authority | HybridRouter | architecture/static review + adapter isolation tests |
| One energy-policy authority | EnergyManager | architecture/static review + provider isolation tests |
| LoRa-only remains functional | LoRa/router | CFG-LORA-STABLE compile + hardware regression |
| Stable firmware works with RF harvesting absent | energy/build | CFG-LORA-STABLE with harvester OFF + stock-hardware regression |
| Stable firmware works with all IP/gateway features absent | build/router | CFG-LORA-STABLE with IP/gateway OFF + hardware regression |
| Automatic retry when connectivity returns | reliability/store/router | offline -> route-up -> auto delivery without second Send press |
| No fixed distance retry threshold | reliability/router | route-up event at varied simulated conditions; no distance field in retry policy |
| Cached alternate before broad rediscovery | multipath/router | A-B-D / A-C-D failover simulator + hardware test |
| Duplicate arrival displayed once | dedup/reliability | multipath duplicate + lost-ACK replay test |
| Route errors only invalidate relevant paths | router | unrelated-RERR regression test |
| Retry/discovery bounded | reliability/router/airtime | high-loss stress + congestion tests |
| Energy-state transitions hysteretic | energy | noisy battery/source simulation without state flapping |
| Low-energy policy remains bounded | energy/router | critical/survival simulations with no retry/discovery storm |
| Energy deferral does not fake delivery | energy/reliability | deferred TX remains queued/pending until real E2E ACK |
| Invalid power sample isolated | energy/health | impossible/stale sample rejection + safe-policy test |
| Energy provider failure cannot break LoRa | energy/LoRa | provider fault/removal test + CFG-LORA-STABLE regression |
| ESP-NOW Normal supported when enabled | espnow | direct hardware send/receive + hybrid forwarding |
| ESP-NOW LR supported when enabled/compatible | espnow | target capability check + direct hardware LR test |
| ESP-NOW mode choice automatic | espnow/router | NORMAL/LR capability/fallback test; no required user toggle |
| 2.4 GHz coexistence bounded | radio scheduler | ESP-NOW/Wi-Fi/BLE coexistence hardware test |
| Wi-Fi IP backhaul preserves PacketId/chat | IP/router/UI | real T-Deck Wi-Fi send/receive + mixed-route test |
| IP socket success is not delivery truth | IP/reliability | gateway accepts packet but destination ACK withheld |
| IP loss automatically falls back | IP/router | active IP path dropped; radio alternate or WAITING_ROUTE |
| IP recovery triggers queued retry | IP/reliability/store | offline IP gateway -> gateway returns -> auto retry |
| Wi-Fi and cellular are bearer providers, not routers | IP architecture | static/provider-isolation test |
| Gateway advertisements expire and are bounded | gateway | stale/flood/overflow simulation |
| Unauthenticated gateway advertisements fail closed | gateway/security | forged/invalid advertisement negative tests |
| Bootstrap loss does not break active federation peers | gateway/federation | remove bootstrap after peer sessions established |
| Federation peer loss invalidates only affected paths | gateway/router | multi-peer failover scenario |
| Gateway route loops remain bounded | gateway/router | federation loop-attempt simulation |
| Duplicate mixed radio/IP arrival displayed once | dedup/reliability | same PacketId via LoRa and IP gateway paths |
| Cellular provider failure cannot break LoRa | IP/cellular | modem unavailable/drop test + CFG-LORA-STABLE regression |
| Normal handheld requires no public inbound socket | IP/gateway | CFG-IP-BETA architecture/integration test |
| Pending messages survive reboot | MessageStore | reboot recovery test |
| Corrupt/incomplete queue tail cannot boot-loop | MessageStore | fault-injection recovery test |
| Full MessageStore deterministic | MessageStore | near-full/full-store test |
| Identity/config isolated from queue churn | storage | corrupt/erase queue-domain test preserving identity/config |
| All LoRa TX pass AirtimeManager | LoRa/airtime | code-path/static test + integration test |
| Experimental transport removal does not break stable core | build config | CFG-LORA-STABLE build with all beta/lab adapters OFF |
| RF-harvest provider compiles independently | energy/build | CFG-ENERGY-LAB with mock provider ON |
| RF-harvest provider can be removed | energy/build | same commit builds with provider OFF |
| TX-reserve event cannot bypass reliability/airtime | energy/reliability | mock TX_RESERVE_READY integration test |
| Harvest measurements never fabricated | energy | unknown/missing telemetry remains unknown in API/UI diagnostics |
| Shared-antenna harvesting not enabled without evidence | release/RF | configuration/review gate; no stable default path |
| Stable logical envelope survives transport changes | packet/wire | LoRa->ESP-NOW/IP retry/failover with same PacketId |
| Unknown/malformed wire version rejected cleanly | packet/wire | parser negative tests |
| Fragmentation cannot create duplicate app message | packet/reliability | fragment loss/duplicate/reassembly tests |
| Smartphone-like Home/Messages/Contacts/Network/Settings flow | UI | navigation acceptance test on target/emulator |
| Keyboard/trackball/touch core messaging usable without phone | UI | target input acceptance test |
| UI remains responsive during route search/store/energy/IP work | UI/core | non-blocking interaction under discovery + compaction + energy/gateway changes |
| Queued message visibly transitions to Delivered automatically | UI/reliability | delayed-delivery UI state test |
| Normal UI requires no transport/route/gateway/power-electronics tuning | UI/router/energy | user-flow review + settings inspection |
| User gets one-flash normal experience | release | release package + documented stable evidence |

## Test IDs

Use stable IDs in future automated tests and reports:

```text
STO-001 no-SD boot
STO-002 pending survives reboot
STO-003 corrupt-tail recovery
STO-004 full-store behavior
DEL-001 offline then automatic route-up delivery
DEL-002 lost ACK duplicate collapse
RTE-001 basic multihop
RTE-002 cached alternate failover
RTE-003 unrelated RERR isolation
RTE-004 route-flap hysteresis
RTE-005 bounded discovery under loss
ESP-001 ESP-NOW normal direct
ESP-002 ESP-NOW LR direct
ESP-003 automatic normal/LR fallback
ESP-004 ESP-NOW -> LoRa hybrid
ESP-005 LoRa -> ESP-NOW hybrid
RAD-001 2.4 GHz coexistence
AIR-001 LoRa airtime gate enforcement
REL-001 end-to-end ACK vs link success
REL-002 retry bound/backoff
DED-001 multipath duplicate exactly-once presentation
DED-002 mixed LoRa/IP duplicate exactly-once presentation
WIR-001 same PacketId across transport change
WIR-002 malformed/unknown version rejection
WIR-003 fragmentation duplicate/reassembly
IP-001 Wi-Fi NetifProvider lifecycle
IP-002 Wi-Fi IP direct federation send/receive
IP-003 LoRa -> gateway -> IP -> gateway -> LoRa
IP-004 IP link loss automatic fallback
IP-005 IP return automatic queued retry
IP-006 socket success != end-to-end delivery
IP-007 same conversation across radio/IP path changes
IP-008 IP reconnect/backoff bounded
GW-001 authenticated gateway advertisement accepted
GW-002 forged/invalid gateway advertisement rejected
GW-003 advertisement expiry/eviction bounded
GW-004 multiple gateway candidate failover
GW-005 bootstrap loss active peers survive
GW-006 federation loop/control-flood bounded
GW-007 peer/session exhaustion deterministic
GW-008 malformed federation frame fail-closed
CELL-001 selected modem PPP provider lifecycle
CELL-002 modem/network loss radio fallback
CELL-003 cellular power/cost metrics hardware evidence
ENG-001 EnergyManager stock provider initialization
ENG-002 energy-state hysteresis
ENG-003 critical/survival bounded policy
ENG-004 energy deferral preserves delivery truth
ENG-005 invalid/stale sample isolation
ENG-006 harvester absent no-regression
ENG-007 mock harvester provider lifecycle
ENG-008 TX_RESERVE_READY bounded reevaluation
ENG-009 no fabricated harvest telemetry
ENG-010 energy-aware route-score hysteresis
UX-001 smartphone shell navigation
UX-002 keyboard/trackball/touch message flow
UX-003 queued -> automatic delivered visual state
UX-004 UI responsive during network/storage/energy/IP work
UX-005 no normal-user transport/route/gateway/power tuning requirement
CFG-001 LoRa-only build
CFG-002 hybrid build
CFG-003 energy-lab mock-provider build
CFG-004 harvester-off stable regression
CFG-005 IP-beta build
CFG-006 gateway-beta build
CFG-007 IP/gateway-off stable regression
RELSE-001 one-flash release package
RELSE-002 RF-harvest claims/evidence gate
RELSE-003 IP/cellular/federation claims/evidence gate
```

## Hardware-only promotion tests for Ambient RF Energy Assist

These tests are required only when real harvesting hardware exists, but must be completed before the hardware feature leaves LAB:

```text
EHW-001 measured harvested power across documented RF conditions
EHW-002 charge/reservoir behavior and hysteresis
EHW-003 communications insertion-loss/sensitivity comparison
EHW-004 receiver desense/isolation test
EHW-005 transmit-event power integrity / no brownout
EHW-006 provider unplug/failure safe fallback
EHW-007 battery/runtime impact with and without harvesting hardware
```

Exact acceptance limits are set from the eventual hardware design and measured T-Deck baseline, not guessed in advance.

## IP/gateway promotion evidence

Wi-Fi/IP federation is not promoted from BETA on architecture alone. Require real evidence for at least:

```text
real T-Deck Wi-Fi association + IP transport
same PacketId/chat across radio/IP paths
at least two gateways on separate IP networks
gateway/peer failover
bootstrap loss with existing peer continuity
Internet loss -> radio/store-forward continuity
duplicate suppression across mixed paths
bounded session/advertisement/resource pressure
security negative tests
power/memory impact
```

Cellular requires separate real selected-modem/provider evidence before it is advertised as supported.

## Completion rule

A feature is not DONE merely because its source files exist or compile. Its mapped tests must exist and the evidence tier must be stated accurately. Hardware-facing requirements require real hardware evidence before STABLE labeling.

The EnergyManager software layer can be complete before RF-harvesting hardware exists. The physical Ambient RF Energy Assist feature remains LAB until EHW evidence exists. IP/gateway architecture may be implemented before cellular hardware exists; cellular remains target-specific until CELL evidence exists.
