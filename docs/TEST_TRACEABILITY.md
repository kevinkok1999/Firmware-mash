# Requirement-to-Test Traceability

## Purpose

Every requirement maps to concrete evidence. A source file existing or compiling is never enough by itself.

| Requirement | Primary owner | Required evidence |
|---|---|---|
| No microSD required for core operation | standalone/storage | no-SD cold boot + send/receive + queue/reboot/recover |
| One logical PacketId across retries/transports/custody | packet/reliability | duplicate-path + retry + transport/custody switch tests |
| PacketId generation cannot repeat after reboot/rollback | packet/runtime | reboot/power-cut/rollback generator tests |
| Durable TTL is not tied to a volatile boot clock | packet/store/runtime | reboot with pending TTL + clock-reset tests |
| Conversation identity independent from transport/address | UI/message/identity | LoRa/IP/ESP-NOW/address-change conversation tests |
| Multipath arrival ordering remains deterministic/bounded | message/UI | reorder-window + missing/late message tests |
| Reboot cannot create duplicate chat presentation | message/dedup/store | destination reboot + duplicate retry test |
| One routing authority | HybridRouter | architecture/static review + adapter isolation tests |
| One energy-policy authority | EnergyManager | architecture/static review + provider isolation tests |
| LoRa-only remains functional | LoRa/router | CFG-LORA-STABLE compile + hardware regression |
| Stable firmware works with RF harvesting absent | energy/build | CFG-LORA-STABLE with harvester OFF + stock-hardware regression |
| Stable firmware works with all IP/gateway/custody features absent | build/router | CFG-LORA-STABLE with optional features OFF + hardware regression |
| Automatic retry when connectivity returns | reliability/store/router | offline -> route-up -> auto delivery without second Send press |
| No fixed distance retry threshold | reliability/router | route-up event at varied conditions; no distance field in retry policy |
| Cached alternate before broad rediscovery | multipath/router | A-B-D / A-C-D failover simulator + hardware test |
| Duplicate arrival displayed once | dedup/reliability | multipath duplicate + lost-ACK replay test |
| Terminal delivery wins custody race | reliability/custody | delivery ACK racing custody offer/accept |
| Route errors only invalidate relevant paths | router | unrelated-RERR regression test |
| Retry/discovery bounded | reliability/router/airtime | high-loss stress + congestion tests |
| Telemetry flood cannot evict terminal control truth | events/runtime | event-priority overflow test |
| Event overflow can recover state | events/runtime | forced overflow + state resync convergence |
| Stale async completion cannot overwrite newer generation | runtime | generation race tests across session/storage/routing |
| Network task never blocks on storage/TLS/modem/UI | runtime | watchdog/responsiveness tests |
| Energy-state transitions hysteretic | energy | noisy battery/source simulation without state flapping |
| Energy deferral does not fake delivery | energy/reliability | deferred TX remains pending until real E2E ACK |
| Survival policy preserves bounded protocol-completion reserve | energy/runtime | ACK/storage/custody-safe completion under survival policy |
| Invalid power sample isolated | energy/health | impossible/stale sample rejection + safe-policy test |
| Energy provider failure cannot break LoRa | energy/LoRa | provider fault/removal + CFG-LORA-STABLE regression |
| ESP-NOW Normal/LR integration | espnow | direct hardware + fallback + hybrid forwarding |
| Wi-Fi association/roam invalidates stale ESP-NOW channel state | radio scheduler/espnow | channel-generation hardware/integration tests |
| 2.4 GHz coexistence bounded | radio scheduler | ESP-NOW/Wi-Fi/BLE coexistence hardware test |
| Wi-Fi IP backhaul preserves PacketId/chat | IP/router/UI | real T-Deck Wi-Fi + mixed-route test |
| IP socket success is not delivery truth | IP/reliability | gateway accepts packet but destination ACK withheld |
| IP stream framing handles partial/coalesced/oversized data | IP/framing/security | parser split/coalesce/oversize tests |
| Old IP session callback cannot tear down new session | IP/runtime | reconnect generation-race test |
| Path MTU change cannot create nested/duplicate app fragmentation | packet/wire | alternate-path re-fragment/reassembly tests |
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
| Release authenticity metadata fails closed when invalid | release/security | signature/attestation negative test |
| Flash descriptor derives from build layout | release | generated partition/offset comparison |
| Normal update preserves identity and MessageStore | release/storage | non-destructive upgrade hardware/fault test |
| Persistent schema migration is power-loss safe | storage/release | migration fault injection at every commit boundary |
| Rollback cannot corrupt newer unknown schema | storage/release | downgrade/rollback compatibility negative test |
| Wrong chip/layout/update path rejected before write | flasher/release | preflight mismatch test |
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
RAD-001 2.4 GHz coexistence baseline
RAD-002 Wi-Fi channel change invalidates stale ESP-NOW evidence
RAD-003 ESP-NOW reacquires compatible current Wi-Fi channel
RAD-004 unresolved channel transition falls back to LoRa/WAITING_ROUTE
RAD-005 stale ESP-NOW old-generation completion ignored
RAD-006 Wi-Fi roam/scan under load remains bounded and UI responsive
RAD-007 Wi-Fi/ESP-NOW/BLE coexistence stress no deadlock
AIR-001 LoRa airtime gate enforcement
REL-001 end-to-end ACK vs link/socket/custody success
REL-002 retry bound/backoff
DED-001 multipath duplicate exactly-once presentation
DED-002 mixed LoRa/IP duplicate exactly-once presentation
DED-003 destination reboot + duplicate retry no second chat message
WIR-001 same PacketId across transport change
WIR-002 malformed/unknown version rejection
WIR-003 fragmentation duplicate/reassembly
WIR-004 path MTU change re-fragments same PacketId
WIR-005 old incomplete fragments expire while alternate path succeeds
IP-001 Wi-Fi NetifProvider lifecycle
IP-002 Wi-Fi IP direct federation send/receive
IP-003 LoRa -> gateway -> IP -> gateway -> LoRa
IP-004 IP link loss automatic fallback
IP-005 IP return automatic queued retry
IP-006 socket success != end-to-end delivery
IP-007 same conversation across radio/IP path changes
IP-008 IP reconnect/backoff bounded
IP-009 stream parser split header/body
IP-010 stream parser coalesced frames + oversized rejection
IP-011 stale old-session callback cannot tear down new session
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
CUS-011 final delivery ACK wins custody negotiation race
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
ENG-011 survival protocol-completion reserve
EVT-001 telemetry flood cannot evict terminal control event
EVT-002 control overflow triggers bounded state resync
EVT-003 stale-generation completion cannot overwrite recovered state
TSK-001 network task responsive during storage compaction
TSK-002 network task responsive during DNS/TLS/modem delays
PID-001 PacketId generator no repeat across reboot
PID-002 interrupted generator persistence no reuse
PID-003 firmware rollback no live PacketId reuse
TIM-001 durable TTL survives reboot without boot-clock confusion
CONV-001 same contact across LoRa/IP one conversation
CONV-002 out-of-order N+1/N deterministic bounded reorder
CONV-003 missing N cannot block N+1 forever
CONV-004 late N reconciles without duplicate bubbles
CONV-005 reboot preserves local outgoing bubble/order/state
CONV-006 reachability address changes do not split chat
CONV-007 unverified identity replacement not silently merged
CONV-008 verified supported identity rotation preserves chat
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
RELSE-002 release manifest update/schema/evidence metadata
RELSE-003 manifest hashes match artifacts
RELSE-004 flasher descriptor matches generated layout
RELSE-005 STABLE rejects missing hardware evidence
RELSE-006 recovery path exact-release verification
RELSE-007 no manual binary assembly
RELSE-008 artifact maps to commit/board/region/tier
RELSE-009 default NON_DESTRUCTIVE flash preserves identity/data
RELSE-010 wrong chip/flash-layout/update path rejected pre-write
RELSE-011 invalid authenticated release metadata fails closed
RELSE-012 unsupported persistent-schema migration rejects STABLE
UPD-001 normal upgrade preserves identity/config
UPD-002 normal upgrade preserves/reopens pending MessageStore
UPD-003 migration power-cut boundaries recover deterministically
UPD-004 failed first boot reaches safe rollback/recovery
UPD-005 rollback cannot corrupt unknown newer schema
UPD-006 partition mismatch rejected pre-write
UPD-007 default one-click flash no identity/data erase
UPD-008 factory reset explicitly destructive
UPD-009 wrong chip/flash-layout preflight aborts
UPD-010 authenticity + SHA verification fail closed
UPD-011 feature evidence manifest cannot over-promote hardware feature
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

Require real T-Deck Wi-Fi/IP evidence, same PacketId/chat across radio/IP, at least two gateway peers on separate paths, bootstrap/peer failure tests, Internet-loss off-grid continuity, duplicate suppression, resource-pressure/security-negative tests, channel/coexistence evidence and measured memory/power impact. Cellular additionally requires selected real modem/provider evidence.

## Custody promotion evidence

Host/simulator tests validate the state machine, but promotion beyond BETA requires repeatable real T-Deck tests covering durable accept, sender power-off, relay reboot, physical disconnection/re-entry, storage pressure, transport change and final exactly-once presentation.

## Completion rule

A feature is not DONE merely because source exists or compiles. Its mapped tests must exist and the evidence tier must be accurate. Hardware-facing requirements require real hardware evidence before STABLE labeling.
