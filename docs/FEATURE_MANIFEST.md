# Firmware-mash Feature Manifest

## Purpose

This is the single checklist of intended product capabilities. It prevents a one-shot implementation from silently omitting a feature or promoting an unproven experiment to STABLE.

## STABLE target capabilities

### Device independence

- Stock LILYGO T-Deck Plus target.
- Core operation with no microSD installed.
- Local display, keyboard, trackball/touch operation without a phone.
- Internal identity/config persistence.
- Internal durable pending-message storage.
- USB recovery/flash path.

### Messaging

- Direct messages/conversations.
- One logical PacketId per message.
- End-to-end delivery evidence distinct from link TX success.
- Automatic bounded retries.
- Exactly-once application presentation within dedup policy.
- Clear states: Sending, Waiting for connection, Queued, Delivered, Expired/failed.
- Pending message automatically retried when credible connectivity returns; no second Send press.

### LoRa backbone

- SX1262 LoRa required and always available in stable core.
- Region-aware EU868 configuration for intended EU build.
- Central AirtimeManager for all LoRa TX.
- Multi-hop relay.
- Reactive bounded route discovery.
- Congestion/airtime-aware route evaluation.

### Multipath/failover

- Small bounded candidate-route set.
- Cached alternate path before broad rediscovery.
- Path-diversity evaluation.
- Path-scoped route-error invalidation.
- Stable PacketId across failover.
- Hysteresis to avoid route flapping.

### Store-and-forward / delayed delivery

- Durable WAITING_ROUTE state.
- Survives reboot/power interruption once committed.
- Route/link recovery events make message retry-eligible immediately.
- No fixed distance threshold.
- Deterministic TTL/priority/full-store policy.

### User experience

- Smartphone-like shell and navigation.
- Home, Messages, Contacts, Network, Settings.
- Familiar conversation list and chat view.
- Compact status bar with battery/network state.
- Simple human-readable network status.
- Optional Advanced diagnostics.
- No required transport, hop, route, retry or RF tuning by normal users.

### Reliability/health

- Bounded packet/event/ACK/route/peer/work queues.
- Explicit overflow/backpressure policy.
- Storage health and recovery metrics.
- Route/retry/airtime pressure metrics.
- No silent failure.
- Wrap-safe timers and stale-state recovery.

## BETA target capabilities

### ESP-NOW hybrid transport

- ESP-NOW Normal support.
- ESP-NOW Long Range mode when confirmed supported by pinned ESP-IDF/target.
- Automatic NORMAL/LR capability/policy selection.
- Dynamic bounded peer cache.
- Direct T-Deck-to-T-Deck ESP-NOW.
- ESP-NOW -> LoRa forwarding.
- LoRa -> ESP-NOW forwarding.
- Same PacketId/reliability semantics across transport changes.
- 2.4 GHz RadioScheduler/coexistence handling.
- Automatic fallback to LoRa/other valid path when ESP-NOW is unavailable.

ESP-NOW is promoted to STABLE only after repeatable real T-Deck hardware evidence.

## LAB capabilities

- Wi-Fi Aware/NAN transport research.
- TDMA/regional/hierarchical routing experiments if scale evidence justifies them.
- RIS/passive RF-assist advisory integration.
- External compatible backscatter transport research.

LAB features must be removable and cannot become dependencies of CFG-LORA-STABLE.

## Explicit non-features / forbidden assumptions

- No arbitrary third-party Wi-Fi routers used as unconfigured relays.
- No magical amplification of LoRa by unrelated ambient RF.
- No firmware-only ambient-backscatter claim on stock T-Deck hardware.
- No microSD requirement for core messaging.
- No manual per-message route selection in normal UI.
- No unlimited flooding/retries/path storage.
- No custom cryptographic primitive invented by Firmware-mash.
- No claim that compile/simulator success equals hardware validation.

## One-flash release target

The intended normal-user outcome is one approved T-Deck Plus release package. A user flashes it, performs only normal onboarding, and uses the advertised STABLE features. Engineering validation remains a project responsibility, not an end-user workflow.

## Implementation completeness rule

A one-shot coding pass is complete only when every STABLE feature above has either:

1. implementation + mapped passing evidence, or
2. an explicit documented blocker based on measured baseline/hardware facts.

A feature may not simply disappear because implementation became difficult.