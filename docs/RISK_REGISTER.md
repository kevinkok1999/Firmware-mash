# Technical Risk Register

| Risk | Why it matters | Initial mitigation | Gate before stable |
|---|---|---|---|
| LoRa control-traffic saturation | Multipath/discovery can consume scarce airtime | Cache alternates, bounded discovery, central airtime gate, simulate dense networks | Airtime/control traffic stays within defined budget under target scenarios |
| Routing loops/stale alternates | Can waste airtime and prevent delivery | Sequence/freshness rules, loop checks, expiry, RERR-style invalidation | Loop/stale-route fault tests pass |
| False confidence from compile-only ESP-NOW work | Build success may hide RF/coexistence problems | Mark compile-only as EXPERIMENTAL, require hardware acceptance tests | Repeated T-Deck hardware evidence |
| ESP-NOW peer table pressure | ESP-NOW has bounded peer resources | Dynamic hot-peer cache, eviction, broadcast discovery | Peer exhaustion test passes without crash or LoRa regression |
| 2.4 GHz coexistence contention | ESP-NOW/Wi-Fi/BLE share ESP32-S3 radio resources | Explicit RadioScheduler and hardware coexistence tests | Defined degradation behavior and no deadlock |
| Memory growth from multipath | Embedded RAM is finite | Fixed-capacity route/neighbor/ACK/event pools | High-water marks within budget; exhaustion handled explicitly |
| Retry/failover storm | Loss can trigger duplicate retries and rediscovery | Single ReliabilityManager, backoff+jitter, airtime budgets | High-loss stress test remains bounded |
| Duplicate application delivery | Multipath can deliver the same packet twice | Stable PacketId + dedup window | Duplicate-path test shows one app delivery |
| Store-and-forward pressure | Offline destinations can fill flash/RAM | TTL, bounded queue, priorities, eviction policy | Near-full store test has deterministic behavior |
| Security regression through bridges | New transports can accidentally bypass E2E protection | Encrypt above routing/transport, authenticate control where feasible | Security review and negative tests |
| License contamination | GPL/custom-license code could change distribution obligations | Provenance log, MIT-first reuse, clean-room implementation where needed | Licensing review before merge/release |
| RF claims exceed evidence | Range is environment/hardware dependent | Record conditions and evidence tier; no universal range promises | Documentation review |
| Backscatter/RIS overreach | Standard T-Deck cannot gain arbitrary ambient-RF features by firmware | Keep as disabled external-hardware/RF-assist modules | Real compatible hardware required |
| Foundation churn upstream | Fast-moving upstream can break our assumptions | Pin exact commit; update intentionally through reviewed ADR/PR | Baseline reproducible from pin |
| OTA brick risk | Power loss/bad image can render field nodes unusable | Defer OTA until signing, rollback and recovery path exist | Dedicated OTA design review |
| Single route score bias | One heuristic can oscillate or favor poor links | Versioned scoring, hysteresis, simulator tuning, path diversity | Route-flapping tests pass |

## Risk policy

A risk is not considered closed because code compiles. Each stable feature must have a test or operational control that directly addresses its failure mode.
