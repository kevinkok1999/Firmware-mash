# IP/Gateway Specialist Review — 2026-09-17

## Review roles

This review applies four engineering viewpoints to the proposed Internet-assisted Firmware-mash layer:

1. embedded networking / ESP-IDF;
2. resilient mesh/DTN routing;
3. Internet gateway/federation security;
4. product/reliability/operations.

## Verdict

Add IP backhaul, but only as an optional transport under the existing `HybridRouter`. Do not create a separate Internet messaging system.

The high-value product behavior is seamless continuity:

```text
same contact + same chat + same PacketId
```

while the physical route may move between LoRa, ESP-NOW, Wi-Fi IP, cellular IP, gateway federation and delayed/store-forward delivery.

## Embedded networking review

### Recommended

- one `mog_transport_ip` adapter;
- bearer-neutral `NetifProvider` boundary;
- Wi-Fi provider first because T-Deck already has Wi-Fi hardware;
- cellular provider through maintained modem/PPP facilities for selected hardware;
- outbound client sessions on handhelds;
- bounded reconnect/backoff and keepalive;
- EnergyManager budgets for background Wi-Fi/cellular use.

### Avoid

- custom TCP/IP or PPP implementation;
- one transport adapter per modem model;
- requiring public inbound sockets on handhelds;
- assuming every cellular module supports eSIM;
- hard-coded modem AT command logic scattered through routing code.

## Mesh/DTN review

### Recommended

- Internet gateway availability is another route fact, not a privileged delivery truth;
- preserve MessageStore and delayed-delivery semantics when IP disappears;
- federation reachability summaries are bounded, expiring and versioned;
- cached alternatives are tried before broad rediscovery;
- gateway recovery creates route evidence and retry eligibility, but does not bypass ReliabilityManager;
- later custody/store-carry-forward extensions must keep protected envelopes and explicit responsibility transfer.

### Avoid

- global flooding of all node routes;
- cloud server as authoritative owner of conversations;
- resetting PacketId when crossing a gateway;
- considering TCP/TLS write success equal to destination delivery.

## Gateway/security review

### Recommended

- standard maintained TLS/security implementation for IP sessions;
- end-to-end protected user payload remains above gateway transport;
- authenticated gateway advertisements;
- multiple bootstrap/entrypoints;
- bounded peer/session tables and rate limits;
- minimal metadata exposure;
- no automatic publication of precise GPS position;
- explicit credential rotation/revocation design before public federation.

### Avoid

- custom encryption;
- trusting any gateway announcement merely because it is reachable;
- embedding universal shared private credentials in firmware;
- unlimited logs/message retention on gateway servers;
- public federation before malformed-frame/resource-exhaustion tests exist.

## Product/reliability review

The user should see no separate "Internet chat" and "mesh chat".

Normal UI:

```text
Messages -> Contact -> type -> Send
```

Optional high-level network policy:

```text
Auto
Off-grid only
Internet assist
```

Message details may show the path after delivery, but transport selection is not required for sending.

Status should remain human-readable:

```text
Network OK
Internet assist available
Mesh only
Waiting for connection
Delivered
```

## Professional target architecture

```text
Conversation Service
        |
Reliability + MessageStore
        |
HybridRouter
  |        |        |
LoRa    ESP-NOW   IPBackhaul
                   |
               NetifProvider
                /        \
             Wi-Fi     Cellular PPP
                   |
             GatewayManager
                   |
            Federation sessions
```

## Priority implementation sequence

1. Define IP/gateway contracts and versioning.
2. Implement `mog_transport_ip` with host/mock provider.
3. Add Wi-Fi `NetifProvider` on T-Deck.
4. Add GatewayManager + bounded gateway advertisements.
5. Add one authenticated outbound federation session path.
6. Prove same PacketId / same chat through LoRa -> IP -> LoRa.
7. Prove IP failure -> automatic radio fallback / WAITING_ROUTE.
8. Add cellular PPP provider only against selected modem hardware.
9. Add multi-gateway federation and bootstrap-loss tests.
10. Tune scale/discovery summaries from simulator evidence.

## Evidence gates

No world-scale claim is accepted until there is real evidence for:

- T-Deck Wi-Fi backhaul;
- at least two gateways in separate networks;
- multi-gateway routing and failover;
- Internet loss with off-grid continuity;
- duplicate suppression across mixed paths;
- bounded discovery/control traffic;
- credential/authentication failure behavior;
- cellular modem hardware if cellular is advertised;
- memory/power impact on handheld targets.

## Final controller decision

**APPROVED FOR ARCHITECTURE/IMPLEMENTATION SCOPE, BETA UNTIL EVIDENCE.**

IP backhaul and gateway federation are valuable additions because they extend the same offline-first messaging model rather than replacing it. `CFG-LORA-STABLE` remains the permanent regression/fallback target.
