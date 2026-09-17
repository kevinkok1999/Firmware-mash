# Testing Strategy

Firmware-mash uses evidence tiers:

- **PROVEN:** verified by relevant source/tests and, for hardware claims, real hardware evidence.
- **SIMULATED:** deterministic simulator/host evidence only.
- **EXPERIMENTAL:** compile-only, research, WIP or unverified hardware behavior.

## Mandatory simulator/host scenarios

1. Basic multi-hop: `A -> B -> C`.
2. Failover: primary `A -> B -> D`, backup `A -> C -> D`; remove B.
3. Hybrid forward: ESP-NOW `A -> B`, LoRa `B -> C -> D`.
4. Reverse hybrid: LoRa `A -> B`, ESP-NOW `B -> C`, LoRa `C -> D`.
5. Destination offline and later restored.
6. Flapping relay/neighbor.
7. High packet loss.
8. Congested LoRa channel/control traffic.
9. Dense topology.
10. Mobile destination moving between relays.
11. Active path breaks during delivery.
12. Same logical packet arrives through multiple paths.
13. Message-store near exhaustion.
14. Neighbor/peer table exhaustion.
15. 2.4 GHz coexistence pressure.
16. Route loop attempt / stale route chain.
17. Packet pool/event queue exhaustion.
18. Clock/time wrap/discontinuity where timers depend on monotonic time.
19. Message-store recovery after an incomplete/corrupt final record.
20. Deterministic eviction when durable store is full.
21. Rebuild volatile route/neighbor state after simulated reboot while preserving durable pending messages.
22. Queued message to an unreachable destination; restore direct reachability and verify immediate event-driven retry without user action.
23. Queued message to an unreachable destination; install a new indirect route and verify immediate event-driven retry without waiting for a fixed periodic timer.
24. Vary the range/link quality at which reachability returns and verify there is no hard-coded 200 m/500 m/1 km distance trigger.
25. Verify a single noisy RSSI sample does not create a retry storm; retry eligibility follows credible usable-link/path evidence.

## Metrics

Capture at minimum:

- delivery ratio;
- median and p95 latency;
- route discovery time;
- failover recovery time;
- queued-message route-recovery-to-retry latency;
- transmissions per delivered message;
- retransmission count;
- control bytes/packets;
- LoRa airtime;
- duplicate arrival rate;
- route churn;
- queue/pool high-water marks;
- RAM/PSRAM/flash;
- durable-store bytes used/free;
- message-store append/compaction counters;
- CPU where measurable;
- store-forward success/expiry;
- power impact in hardware tests.

## First hardware acceptance test

Use four nodes A/B/C/D. A cannot directly reach D. Two usable routes exist: `A-B-D` and `A-C-D`. Start on B, then power B off. Without reboot, app, phone or manual route selection, A must recover through C. Log detection time, invalidation reason, route selected, packet loss and recovery time.

## Re-entry / delayed-delivery hardware acceptance test

Use two T-Deck Plus units with no microSD requirement:

1. Establish normal direct communication.
2. Move one unit far enough away that no valid route exists.
3. Send a message; sender must retain it as `WAITING_ROUTE`/queued rather than discard it.
4. Bring the units back toward each other.
5. At the first point where the radio stack obtains credible usable-link/path evidence, the queued message must become immediately eligible for retry.
6. The user must not press Send again.
7. Delivery must complete once end-to-end ACK is received.
8. The application must display the message exactly once even if multiple retry/path events occur.

This test is intentionally **not tied to a fixed distance**. The actual point of recovery may be 50 m, 200 m, 800 m or another distance depending on environment, radio mode, antenna orientation and obstructions.

Record:

- transport/path that recovered first;
- time from usable-link/path event to first retry;
- time from event to delivered ACK;
- retries performed;
- duplicates suppressed;
- RSSI/SNR/link evidence for diagnostics only.

## Standalone no-microSD acceptance test

Run on a stock T-Deck Plus with **no microSD card installed**:

1. Cold boot to local UI.
2. Confirm identity/config persistence.
3. Send/receive LoRa message.
4. Queue a message while destination is offline.
5. Power-cycle the sending device.
6. Confirm queued message still exists.
7. Bring destination back.
8. Confirm exactly one delivery/ACK.
9. Exercise near-full internal message store.
10. Confirm device still boots after interrupted store mutation/power cut tests.
11. Confirm USB recovery/flash path still works.

Failure of this test blocks STABLE promotion.

## Hybrid hardware acceptance test

`T-Deck A --ESP-NOW--> T-Deck B --LoRa--> C --LoRa--> T-Deck D`. Confirm end-to-end ACK/delivery. Then disable ESP-NOW. If a LoRa-only alternate exists, routing must fail over automatically. If it does not exist, message must enter an explicit waiting/no-route state rather than silently disappear.

## Power/battery acceptance

Measure at minimum:

- LoRa-only idle;
- LoRa receive/relay activity;
- ESP-NOW discovery enabled;
- display active vs idle/dimmed;
- GNSS on/off where used.

Do not promote a hybrid feature without documenting its incremental power cost on real hardware.

## LoRa-only regression gate

Every beta/lab feature can be disabled. A LoRa-only build must continue to compile, boot and pass its baseline communication tests without a microSD card.

## No fake evidence

A green compile is only a compile result. Simulator success is not hardware validation. A single successful field message is not a scale claim. Every report must state which evidence tier supports it.
