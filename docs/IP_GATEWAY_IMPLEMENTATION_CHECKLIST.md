# IP/Gateway Implementation Checklist

Use this checklist after `docs/BASELINE_APPROVED` exists and the implementation phase reaches IP backhaul.

## A. Core seam

- [ ] Create `mog_transport_ip` with no Wi-Fi/cellular-specific routing logic.
- [ ] Create NetifProvider interface.
- [ ] Create host/mock NetifProvider.
- [ ] Keep HybridRouter as sole route owner.
- [ ] Preserve conversation identity and PacketId across IP transitions.

## B. Wi-Fi backhaul

- [ ] Implement T-Deck Wi-Fi NetifProvider using maintained ESP-IDF/foundation APIs.
- [ ] Emit bounded bearer-up/down/address-change events.
- [ ] Add reconnect backoff with jitter.
- [ ] Add bounded keepalive/session state.
- [ ] Integrate with RadioScheduler for ESP-NOW/Wi-Fi coexistence.
- [ ] Integrate EnergyManager background-radio budget.

## C. Gateway discovery

- [ ] Create GatewayManager fixed/bounded table.
- [ ] Create authenticated/versioned GatewayAdvertisement parser.
- [ ] Expire stale advertisements.
- [ ] Add deterministic eviction under pressure.
- [ ] Accept mesh/radio advertisements.
- [ ] Add optional local-LAN discovery.
- [ ] Add multiple configured bootstrap peers.
- [ ] Reject malformed/unsupported/unauthenticated input cleanly.

## D. Federation session

- [ ] Use maintained standard TLS/security implementation.
- [ ] Handheld initiates outbound connection by default.
- [ ] Keep user payload end-to-end protected.
- [ ] Do not log plaintext message bodies.
- [ ] Bound peer/session queues and connection counts.
- [ ] Version IP/federation framing.
- [ ] Distinguish link/session write success from destination delivery.

## E. Hybrid routing

- [ ] Convert gateway reachability into HybridRouter candidate evidence.
- [ ] Include latency/loss/energy/metered/trust inputs in versioned route score.
- [ ] Never hard-code Internet as preferred.
- [ ] Support heterogeneous LoRa -> IP -> LoRa path.
- [ ] Invalidate only paths affected by failed gateway/session.
- [ ] Suppress mixed-path duplicate presentation by PacketId.

## F. Failure/recovery

- [ ] IP down -> try valid radio alternate.
- [ ] No alternate -> durable WAITING_ROUTE.
- [ ] Gateway returns -> queued message retry eligible.
- [ ] Bootstrap loss -> established peers continue.
- [ ] Peer loss -> unrelated peer paths remain.
- [ ] Internet fully down -> same local/off-grid chat remains usable.

## G. Cellular provider

Do not claim cellular support until a specific modem target is chosen.

- [ ] Select supported modem/BOM.
- [ ] Implement provider through maintained modem/PPP APIs.
- [ ] Keep modem-specific commands out of routing code.
- [ ] Test SIM/network registration/data-session loss.
- [ ] Measure power use.
- [ ] Record provider/network limitations.
- [ ] eSIM only if the selected hardware/operator path actually supports it.

## H. UI

- [ ] One contact = one conversation.
- [ ] No separate Internet/mesh chat.
- [ ] Optional policy: Auto / Off-grid only / Internet assist.
- [ ] Message details may show delivered path after the fact.
- [ ] Normal send flow never asks for transport/gateway.

## I. Required evidence

- [ ] CFG-IP-BETA compiles.
- [ ] CFG-GATEWAY-BETA compiles when gateway target exists.
- [ ] CFG-LORA-STABLE still compiles with all IP/gateway code OFF.
- [ ] IP-001..IP-008 pass as applicable.
- [ ] GW-001..GW-008 pass as applicable.
- [ ] DED-002 passes.
- [ ] CELL-001..CELL-003 only claimed after selected real modem hardware.
- [ ] Resource/power delta recorded.
- [ ] Security review completed before public federation.

## Done definition

The IP/gateway layer is implementation-complete for BETA when a real T-Deck can send within the same conversation over a mixed radio/IP path, lose Internet, fall back or queue safely, recover automatically, avoid duplicate presentation and keep the LoRa-only build fully functional.
