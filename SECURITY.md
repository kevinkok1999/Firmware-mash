# Security Policy

Firmware-mash is pre-release research software. Do not rely on it for life-safety, emergency-service replacement or guaranteed delivery.

## Security architecture rules

- Do not invent new cryptographic primitives.
- End-to-end message protection is applied above routing/transports wherever practical.
- Relay nodes must not need plaintext message contents to forward traffic.
- Link-layer success is never treated as proof of end-to-end delivery.
- Replay and duplicate handling must survive multipath delivery.
- New transports may add protection but may not weaken the existing end-to-end boundary.
- Private keys/secrets must not be committed to the repository, CI or sample configs.

## Sensitive areas requiring explicit review

- identity/key provisioning;
- nonce/counter persistence;
- session establishment;
- control-plane authentication;
- ACK/receipt authentication;
- OTA signing and rollback;
- BLE/Wi-Fi provisioning;
- external bridge/backscatter control protocols.

## Experimental features

NAN, RF-assist and external backscatter remain disabled by default until threat models and hardware behavior are understood. ESP-NOW is not considered end-to-end security by itself.

## Reporting

Until a private reporting channel is configured, avoid posting exploitable secrets or private keys in public issues. For ordinary non-sensitive security hardening, open a GitHub issue describing the affected component and expected security property without including credentials.
