# Firmware-mash Feature Manifest

## Purpose

This is the single checklist of intended product capabilities. It prevents a one-shot implementation from silently omitting a feature or promoting an unproven experiment to STABLE.

## STABLE target capabilities

### Device independence

- Stock LILYGO T-Deck Plus target.
- Core operation with no microSD installed.
- Core operation with no external energy-harvesting hardware installed.
- Core operation with no Internet connectivity.
- Local display, keyboard, trackball/touch operation without a phone.
- Internal identity/config persistence.
- Internal durable pending-message storage.
- USB recovery/flash path.

### Messaging

- Direct messages/conversations.
- Conversation identity independent from transport/path.
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
- Heterogeneous route segments may be represented without changing application identity.

### Store-and-forward / delayed delivery

- Durable WAITING_ROUTE state.
- Survives reboot/power interruption once committed.
- Route/link recovery events make message retry-eligible immediately.
- No fixed distance threshold.
- Deterministic TTL/priority/full-store policy.

### Energy management

- One `EnergyManager` policy authority.
- Battery/external-power state where the selected foundation exposes reliable telemetry.
- Explicit hysteretic states: external power/normal/conserve/critical/survival.
- Energy-aware background-discovery, relay and multipath budgets.
- Energy cost available as one bounded route-score input.
- Energy-driven deferral cannot produce a false Delivered state or change PacketId/dedup semantics.
- Stable firmware remains complete with all RF-harvesting code/providers removed.
- No high-frequency energy telemetry persisted to flash.

### User experience

- Smartphone-like shell and navigation.
- Home, Messages, Contacts, Network, Settings.
- Familiar conversation list and chat view.
- Same chat remains continuous across all available routes/transports.
- Compact status bar with battery/network state.
- Simple human-readable network status.
- Human-readable energy-saving/survival indication when relevant.
- Optional Advanced diagnostics.
- No required transport, hop, route, retry, RF, gateway or power-electronics tuning by normal users.

### Reliability/health

- Bounded packet/event/ACK/route/peer/work queues.
- Explicit overflow/backpressure policy.
- Storage health and recovery metrics.
- Route/retry/airtime pressure metrics.
- Energy-policy transition/deferral health metrics.
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

### IP backhaul / gateway federation

- One `mog_transport_ip` logical adapter.
- Wi-Fi and cellular treated as bearer providers below the IP adapter, not separate chats/routing engines.
- Stock T-Deck Wi-Fi backhaul support after real hardware validation.
- Optional cellular PPP provider only for selected compatible modem hardware.
- Same conversation and PacketId across LoRa/ESP-NOW/IP transitions.
- `GatewayManager` and bounded `GatewayDiscovery` provide reachability/capability evidence to HybridRouter.
- Authenticated/expiring gateway advertisements.
- Outbound authenticated federation sessions for handhelds.
- Multiple bootstrap/peer entrypoints; no single mandatory cloud server.
- Loss of IP/gateway path automatically falls back to other valid paths or WAITING_ROUTE.
- Internet recovery makes queued messages retry-eligible without a second Send press.
- End-to-end user payload remains protected across gateways.
- Gateway/session/advertisement queues and rate limits are bounded.
- High-level user policy only: Auto / Off-grid only / Internet assist.

IP/gateway features remain BETA until real T-Deck Wi-Fi, multi-gateway failover and security/resource tests pass. Cellular capability remains target-specific and hardware-evidence-gated.

## LAB capabilities

### Ambient RF Energy Assist

- Optional external RF harvesting provider under `EnergyManager`.
- Rectenna/harvester-PMIC telemetry where compatible hardware exists.
- Optional external energy reservoir/supercap awareness.
- `TX_RESERVE_READY` style event only after measured hardware characterization.
- Measured harvested-power/energy reporting with confidence and no fabricated values.
- Separate harvesting antenna/rectenna as the default hardware assumption.
- Shared-antenna experiments only after insertion-loss/desense/isolation measurements.

Ambient RF Energy Assist is an optional energy source, not a communications transport and not a guaranteed power source. A stock T-Deck Plus is never claimed to harvest ambient RF through firmware alone.

### Other LAB capabilities

- Wi-Fi Aware/NAN transport research.
- Advanced NAT traversal/direct Internet peer experiments after the first gateway federation works.
- TDMA/regional/hierarchical routing experiments if scale evidence justifies them.
- RIS/passive RF-assist advisory integration.
- External compatible backscatter transport research.

LAB features must be removable and cannot become dependencies of CFG-LORA-STABLE.

## Explicit non-features / forbidden assumptions

- No arbitrary third-party Wi-Fi routers used as unconfigured Firmware-mash relays.
- No 4G/5G mast used without legitimate modem/subscription/network access.
- No claim that an Internet gateway preserves off-grid operation; IP is an optional path only.
- No single mandatory cloud server owning chats or message history.
- No magical amplification of LoRa by unrelated ambient RF.
- No firmware-only ambient-backscatter claim on stock T-Deck hardware.
- No firmware-only ambient-RF energy harvesting claim on stock T-Deck hardware.
- No guaranteed harvested-power claim without measured hardware/environment evidence.
- No unvalidated loading/sharing of the tuned LoRa antenna for energy harvesting.
- No microSD requirement for core messaging.
- No manual per-message route selection in normal UI.
- No unlimited flooding/retries/path/gateway storage.
- No custom cryptographic primitive invented by Firmware-mash.
- No claim that compile/simulator success equals hardware validation.
- No guaranteed 30 km/300 km/global availability claim without real participating connectivity/gateway evidence.

## One-flash release target

The intended normal-user outcome is one approved T-Deck Plus release package. A user flashes it, performs only normal onboarding, and uses the advertised STABLE features. Engineering validation remains a project responsibility, not an end-user workflow.

The same package may contain disabled BETA/LAB interfaces, but no normal user is told that cellular, public federation or RF harvesting works unless matching hardware/network evidence exists.

## Implementation completeness rule

A one-shot coding pass is complete only when every STABLE feature above has either:

1. implementation + mapped passing evidence, or
2. an explicit documented blocker based on measured baseline/hardware facts.

The EnergyManager software layer is part of this implementation completeness rule. The physical ambient-RF harvester is not: its interface/simulator path is implemented, while actual hardware promotion remains evidence-gated.

Approved BETA architecture such as IP backhaul/gateway federation must retain its implementation seam/tests even when not yet promoted to STABLE.

A feature may not simply disappear because implementation became difficult.
