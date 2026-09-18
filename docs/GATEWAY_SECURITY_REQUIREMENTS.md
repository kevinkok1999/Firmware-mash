# Gateway Federation Security Requirements

## Scope

These requirements apply before any public Internet federation is exposed. They supplement the foundation's end-to-end message security and do not replace it.

## Hard requirements

- No custom cryptographic primitive.
- Use maintained SDK/foundation TLS/security implementations for IP sessions.
- Unique gateway/device credentials; no universal embedded private key shared by all devices.
- Credential provisioning, rotation, revocation and expiry must be defined before public rollout.
- Gateway discovery advertisements must be authenticated before privileged routing trust is granted.
- Unsupported protocol/security versions fail closed.
- User message bodies remain end-to-end protected across gateway relays where the foundation security model permits.
- Gateways do not require plaintext user content simply to route/relay.
- Logs never contain plaintext message bodies by default.
- Precise user GPS/location is not part of ordinary gateway advertisements.
- Per-peer RX/TX queues, sessions, advertisements and reconnect attempts are bounded.
- Rate-limit authentication failures, discovery floods and malformed-frame sources.
- Replay/duplicate protection applies to control advertisements where required by the chosen protocol.
- Malformed federation input must not block the local network task or LoRa operation.

## Public gateway role

A public listener is a gateway-class role. Normal handheld T-Deck units use outbound sessions by default and do not need public inbound ports.

## Metadata minimisation

Advertise only what is required for routing/policy, such as:

```text
gateway identity
protocol version
capability flags
reachability summary
freshness/expiry
bounded cost/trust hints
```

Do not automatically advertise:

```text
chat history
message content
contact list
precise live GPS coordinates
unnecessary hardware identifiers
```

## Security tests before public federation

- invalid certificate/peer credential rejected;
- expired/revoked credential behavior;
- forged gateway advertisement rejected;
- replayed/stale advertisement rejected/expired;
- malformed frame/parser fuzz/negative cases;
- excessive connection/session attempts remain bounded;
- excessive advertisement/control traffic remains bounded;
- peer disconnect/reconnect does not duplicate user-visible messages;
- gateway compromise/failure cannot make CFG-LORA-STABLE unusable.

## Release rule

A development or private-lab federation may precede the full public-security gate. Public federation is not labelled production-ready until the credential lifecycle, negative tests, resource limits and incident/recovery procedures are documented and exercised.
