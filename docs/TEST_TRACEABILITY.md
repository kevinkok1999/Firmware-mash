# Requirement-to-Test Traceability

## Purpose

Every stable requirement maps to at least one concrete test. This prevents implementation from drifting away from the architecture while still claiming completion.

| Requirement | Primary owner | Required evidence |
|---|---|---|
| No microSD required for core operation | standalone/storage | no-SD cold boot + send/receive + queue/reboot/recover |
| One logical PacketId across retries/transports | packet/reliability | duplicate-path + retry + transport-switch tests |
| One routing authority | HybridRouter | architecture/static review + adapter isolation tests |
| LoRa-only remains functional | LoRa/router | CFG-LORA-STABLE compile + hardware regression |
| Automatic retry when connectivity returns | reliability/store/router | offline -> route-up -> auto delivery without second Send press |
| No fixed distance retry threshold | reliability/router | route-up event at varied simulated conditions; no distance field in retry policy |
| Cached alternate before broad rediscovery | multipath/router | A-B-D / A-C-D failover simulator + hardware test |
| Duplicate arrival displayed once | dedup/reliability | multipath duplicate + lost-ACK replay test |
| Route errors only invalidate relevant paths | router | unrelated-RERR regression test |
| Retry/discovery bounded | reliability/router/airtime | high-loss stress + congestion tests |
| ESP-NOW Normal supported when enabled | espnow | direct hardware send/receive + hybrid forwarding |
| ESP-NOW LR supported when enabled/compatible | espnow | target capability check + direct hardware LR test |
| ESP-NOW mode choice automatic | espnow/router | NORMAL/LR capability/fallback test; no required user toggle |
| 2.4 GHz coexistence bounded | radio scheduler | ESP-NOW/Wi-Fi/BLE coexistence hardware test |
| Pending messages survive reboot | MessageStore | reboot recovery test |
| Corrupt/incomplete queue tail cannot boot-loop | MessageStore | fault-injection recovery test |
| Full MessageStore deterministic | MessageStore | near-full/full-store test |
| Identity/config isolated from queue churn | storage | corrupt/erase queue-domain test preserving identity/config |
| All LoRa TX pass AirtimeManager | LoRa/airtime | code-path/static test + integration test |
| Experimental transport removal does not break stable core | build config | CFG-LORA-STABLE build with all lab adapters OFF |
| Stable logical envelope survives transport changes | packet/wire | LoRa->ESP-NOW retry/failover with same PacketId |
| Unknown/malformed wire version rejected cleanly | packet/wire | parser negative tests |
| Fragmentation cannot create duplicate app message | packet/reliability | fragment loss/duplicate/reassembly tests |
| Smartphone-like Home/Messages/Contacts/Network/Settings flow | UI | navigation acceptance test on target/emulator |
| Keyboard/trackball/touch core messaging usable without phone | UI | target input acceptance test |
| UI remains responsive during route search/store I/O | UI/core | non-blocking interaction test under discovery + compaction |
| Queued message visibly transitions to Delivered automatically | UI/reliability | delayed-delivery UI state test |
| Normal UI requires no transport/route selection | UI/router | user-flow review + settings inspection |
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
WIR-001 same PacketId across transport change
WIR-002 malformed/unknown version rejection
WIR-003 fragmentation duplicate/reassembly
UX-001 smartphone shell navigation
UX-002 keyboard/trackball/touch message flow
UX-003 queued -> automatic delivered visual state
UX-004 UI responsive during network/storage work
UX-005 no normal-user transport/route tuning requirement
CFG-001 LoRa-only build
CFG-002 hybrid build
RELSE-001 one-flash release package
```

## Completion rule

A feature is not DONE merely because its source files exist or compile. Its mapped tests must exist and the evidence tier must be stated accurately. Hardware-facing requirements require real hardware evidence before STABLE labeling.