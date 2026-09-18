# ADR 0007 — IP Backhaul and Gateway Federation

Status: Accepted

## Context

Firmware-mash is off-grid first, but the same T-Deck user experience should be able to use ordinary IP infrastructure when available. Wi-Fi and cellular data can extend reach dramatically, while LoRa/ESP-NOW/store-forward must remain available when Internet infrastructure disappears.

The design must avoid creating separate chats or duplicate routing stacks for Wi-Fi, cellular and radio paths.

## Decision

1. `HybridRouter` remains the single logical routing authority.
2. Introduce one logical `mog_transport_ip` adapter for Internet/private-IP backhaul.
3. Wi-Fi and cellular are `NetifProvider` bearers below `mog_transport_ip`, not separate user-facing networks.
4. A logical message keeps the same conversation identity and PacketId across LoRa, ESP-NOW and IP path changes.
5. Introduce `GatewayManager` and `GatewayDiscovery` as capability/reachability services. They provide evidence to HybridRouter and do not own independent route tables.
6. Public federation uses authenticated standard transport security from maintained SDK/foundation libraries; no custom cryptographic primitive is introduced.
7. User-message confidentiality remains end-to-end above gateways/transports where the selected foundation permits it.
8. Federation is designed for multiple bootstrap/peer entrypoints and must not require one irreplaceable central cloud server.
9. Normal handhelds are allowed to make outbound federation sessions; accepting public inbound Internet connections is a gateway-class role, not a handheld requirement.
10. `CFG-LORA-STABLE` must compile and operate with all IP/gateway code disabled.
11. Cellular support is hardware/evidence-gated. Stock T-Deck Plus hardware is not claimed to contain a cellular modem.
12. A normal Wi-Fi router is only an IP access medium after legitimate association; it is not treated as an unconfigured Firmware-mash relay.

## Consequences

- One chat can transparently survive route changes between off-grid and Internet-assisted paths.
- Wi-Fi and modem implementations can evolve without creating separate routing engines.
- Federation/discovery/security/resource limits become first-class engineering work.
- Internet outages degrade the available path set rather than breaking the product's core messaging model.
- Global reach is possible only when participating gateways and IP connectivity exist; it is not guaranteed by firmware alone.

## Evidence / constraints

- ESP-IDF provides standard network-interface mechanisms and `esp_modem` supports PPPoS-backed cellular modems such as BG96/SIM7600-class modules in current documentation.
- Existing resilient-network designs demonstrate that multiple heterogeneous interfaces and discoverable gateway entrypoints are practical architectural patterns.
- DTN/BPv7 establishes store-carry-forward as an application-layer overlay across heterogeneous constituent networks.
- Exact IP framing, security credential lifecycle, modem target and route-summary format remain implementation/evidence decisions and must be versioned/tested.

## Supersedes / superseded by

None.
