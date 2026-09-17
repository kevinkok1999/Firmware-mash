# Requirement-to-Test Traceability

## Purpose

Every requirement maps to concrete evidence. A source file existing or compiling is never enough by itself.

| Requirement | Primary owner | Required evidence |
|---|---|---|
| No microSD required for core operation | standalone/storage | no-SD cold boot + send/receive + queue/reboot/recover |
| One logical PacketId across retries/transports/custody | packet/reliability | duplicate-path + retry + transport/custody switch tests |
| Conversation identity independent from transport | UI/message/packet | LoRa/IP/ESP-NOW mixed-path conversation test |
| One routing authority | HybridRouter | architecture/static review + adapter isolation tests |
| One energy-policy authority | EnergyManager | architecture/static review + provider isolation tests |
| LoRa-only remains functional | LoRa/router | CFG-LORA-STABLE compile + hardware regression |
| Stable firmware works with RF harvesting absent | energy/build | CFG-LORA-STABLE with harvester OFF + stock-hardware regression |
| Stable firmware works with all IP/gateway/custody features absent | build/router | CFG-LORA-STABLE with optional features OFF + hardware regression |
| Automatic retry when connectivity returns | reliability/store/router | offline -> route-up -> auto delivery without second Send press |
| No fixed distance retry threshold | reliability/router | route-up event at varied conditions; no distance field in retry policy |
| Cached alternate before broad rediscovery | multipath/router | A-B-D / A-C-D failover simulator + hardware test |
| Duplicate arrival displayed once | dedup/reliability | multipath duplicate + lost-ACK replay test |
| Route errors only invalidate relevant paths | router | unrelated-RERR regression test |
| Retry/discovery bounded | reliability/router/airtime | high-loss stress + congestion tests |
| Energy-state transitions hysteretic | energy | noisy battery/source simulation without state flapping |
| Energy deferral does not fake delivery | energy/reliability | deferred TX remains pending until real E2E ACK |
| Invalid power sample isolated | energy/health | impossible/stale sample rejection + safe-policy test |
| Energy provider failure cannot break LoRa | energy/LoRa | provider fault/removal + CFG-LORA-STABLE regression |
| ESP-NOW Normal/LR integration | espnow | direct hardware + fallback + hybrid forwarding |
| 2.4 GHz coexistence bounded | radio scheduler | ESP-NOW/Wi-Fi/BLE coexistence hardware test |
| Wi-Fi IP backhaul preserves PacketId/chat | IP/router/UI | real T-Deck Wi-Fi + mixed-route test |
| IP socket success is not delivery truth | IP/reliability | gateway accepts packet but destination ACK withheld |
| IP loss automatically falls back | IP/router | active IP path dropped -> radio alternate or WAITING_ROUTE |
| IP recovery triggers queued retry | IP/reliability/store | gateway returns -> automatic retry |
| Wi-Fi/cellular are bearer providers, not routers | IP architecture | static/provider-isolation test |
| Gateway advertisements expire and are bounded | gateway | stale/flood/overflow simulation |
| Unauthenticated gateway advertisements fail closed | gateway/security | forged/invalid advertisement negative tests |
| Bootstrap loss does not break active federation peers | gateway/federation | remove bootstrap after peer sessions established |
| Federation peer loss invalidates only affected paths | gateway/router | multi-peer failover scenario |
| Gateway loops/control flood bounded | gateway/router | federation loop/flood simulation |
| Cellular provider failure cannot break LoRa | IP/cellular | modem unavailable/drop + stable regression |
| Custody commit precedes acceptance | custody/store | power-cut/commit-order test |
| Custody acceptance is not Delivered | custody/reliability/UI | accepted custody with destination offline remains not-delivered |
| Relay reboot preserves accepted custody | custody/store | reboot recovery + later forward |
| Custody duplicate/reconciliation remains bounded | custody/dedup | lost evidence + duplicate offer/accept stress |
| Storage/energy pressure rejects custody safely | custody/energy/store | full-store + critical-energy rejection |
| Custody disabled preserves sender delayed delivery | custody/build/reliability | CFG-LORA-STABLE regression + WAITING_ROUTE test |
| Pending messages survive reboot | MessageStore | reboot recovery test |
| Corrupt/incomplete queue tail cannot boot-loop | MessageStore | fault-injection recovery test |
| Full MessageStore deterministic | MessageStore | near-full/full-store test |
| Identity/config isolated from queue churn | storage | corrupt/erase queue-domain preserving identity/config |
| All LoRa TX pass AirtimeManager | LoRa/airtime | static code-path + integration test |
| Optional transport/provider removal preserves stable core | build config | CFG-LORA-STABLE with beta/lab adapters OFF |
| RF-harvest provider compiles independently/removable | energy/build | CFG-ENERGY-LAB + stable provider-OFF build |
| Harvest measurements never fabricated | energy | unknown/missing telemetry remains unknown |
| Stable logical envelope survives transport changes | packet/wire | LoRa->ESP-NOW/IP failover with same PacketId |
| Unknown/malformed wire version rejected | packet/wire | parser negative tests |
| Smartphone local UI flow | UI | navigation + keyboard/trackball/touch acceptance |
| Queued message visibly transitions automatically | UI/reliability | WAITING -> Sending -> Delivered test |
| Normal UI hides route/gateway/power engineering | UI | settings/user-flow inspection |
| Clean checkout can create release package | release | release pipeline acceptance |
| Release hashes/manifest match binaries | release | manifest/hash verification |
| Flash descriptor derives from build layout | release | generated partition/offset comparison |
| STABLE rejects missing hardware evidence | release | negative release-gate test |
| Recovery path matches exact release layout | release | flash/recovery acceptance on target |
| User performs no manual binary assembly | release/flasher | end-to-end flasher acceptance |
| Coding trigger refuses invalid baseline | coding trigger/preflight | missing/invalid marker negative test |
| Coding trigger preserves document precedence | coding trigger/controller | contradiction fixture must stop before source mutation |

## Test IDs

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
REL-001 end-to-end ACK vs link/socket/custody success
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
CUS-001 durable commit precedes custody acceptance
CUS-002 sender powers off after accepted custody; relay later delivers
CUS-003 relay reboot preserves accepted custody
CUS-004 duplicate custody offer/accept exactly-once app presentation
CUS-005 full-store/critical-energy custody rejection
CUS-006 lost custody evidence bounded reconciliation
CUS-007 custody TTL expiry deterministic
CUS-008 same PacketId across LoRa/ESP-NOW/IP after custody
CUS-009 custody disabled sender WAITING_ROUTE regression
CUS-010 malformed/unauthenticated custody control fails closed
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
CFG-002 hybrid ESP-NOW build
CFG-003 energy-lab mock-provider build
CFG-004 harvester-off stable regression
CFG-005 IP-beta build
CFG-006 gateway-beta build
CFG-007 IP/gateway-off stable regression
CFG-008 custody-beta build
CFG-009 custody-off stable regression
TRIG-001 missing baseline blocks coding trigger
TRIG-002 invalid baseline marker blocks coding trigger
TRIG-003 unresolved controller contradiction blocks coding trigger
RELSE-001 clean-checkout one-flash release package
RELSE-002 RF-harvest claims/evidence gate
RELSE-003 IP/cellular/federation claims/evidence gate
RELSE-004 manifest hashes match artifacts
RELSE-005 flasher descriptor matches generated layout
RELSE-006 STABLE rejects missing hardware evidence
RELSE-007 recovery path exact-release verification
RELSE-008 no manual binary assembly / artifact maps to commit-board-region-tier
```

## Hardware-only Ambient RF promotion tests

```text
EHW-001 measured harvested power across documented RF conditions
EHW-002 charge/reservoir behavior and hysteresis
EHW-003 communications insertion-loss/sensitivity comparison
EHW-004 receiver desense/isolation test
EHW-005 transmit-event power integrity / no brownout
EHW-006 provider unplug/failure safe fallback
EHW-007 battery/runtime impact with and without harvesting hardware
```

## IP/gateway promotion evidence

Require real T-Deck Wi-Fi/IP evidence, same PacketId/chat across radio/IP, at least two gateway peers on separate paths, bootstrap/peer failure tests, Internet-loss off-grid continuity, duplicate suppression, resource-pressure/security-negative tests and measured memory/power impact. Cellular additionally requires selected real modem/provider evidence.

## Custody promotion evidence

Host/simulator tests validate the state machine, but promotion beyond BETA requires repeatable real T-Deck tests covering durable accept, sender power-off, relay reboot, physical disconnection/re-entry, storage pressure, transport change and final exactly-once presentation.

## Completion rule

A feature is not DONE merely because source exists or compiles. Its mapped tests must exist and the evidence tier must be accurate. Hardware-facing requirements require real hardware evidence before STABLE labeling.
