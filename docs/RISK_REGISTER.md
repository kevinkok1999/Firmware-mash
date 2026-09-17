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
| Volatile store-and-forward | A RAM-only queue loses undelivered messages on reboot/power loss | Dedicated bounded internal-flash MessageStore plus RAM index/cache | No-SD reboot/power-cut queue recovery test passes |
| Store-and-forward pressure | Offline destinations can fill internal flash/RAM | TTL, bounded queue, priorities, deterministic eviction | Near-full store test has deterministic behavior |
| Internal flash wear | Persisting high-churn route/RF metrics can shorten flash life | Persist only critical durable state; keep route/RF data in RAM/PSRAM; append/batch writes | Endurance/compaction counters measured and no per-metric persistent writes |
| Power loss during storage mutation | Handheld battery/reset can interrupt append, ACK-delete or compaction | Record integrity, recoverable journal/filesystem, isolated partitions | Repeated power-cut tests recover without boot loop or identity loss |
| Accidental microSD dependency | Removable card can be absent/corrupt/removed | Core state must live in internal flash; SD optional only | Full stable acceptance suite passes with no card installed |
| Partition exhaustion | Firmware growth can crowd out OTA/recovery or durable messages | Measure baseline image first; reserve explicit app/data/recovery growth margins | Partition budget reviewed on every release-size increase |
| Battery drain from hybrid discovery | Always-on Wi-Fi/ESP-NOW/BLE/GNSS/display can make handheld impractical | Role-aware discovery windows, radio scheduler, display idle policy, measured power budgets | Real T-Deck battery/power profile documented for stable features |
| Security regression through bridges | New transports can accidentally bypass E2E protection | Encrypt above routing/transport, authenticate control where feasible | Security review and negative tests |
| License contamination | GPL/custom-license code could change distribution obligations | Provenance log, MIT-first reuse, clean-room implementation where needed | Licensing review before merge/release |
| RF claims exceed evidence | Range is environment/hardware dependent | Record conditions and evidence tier; no universal range promises | Documentation review |
| Backscatter/RIS overreach | Standard T-Deck cannot gain arbitrary ambient-RF features by firmware | Keep as disabled external-hardware/RF-assist modules | Real compatible hardware required |
| Foundation churn upstream | Fast-moving upstream can break our assumptions | Pin exact commit; update intentionally through reviewed ADR/PR | Baseline reproducible from pin |
| OTA brick risk | Power loss/bad image can render field nodes unusable | Reserve A/B-compatible layout; defer OTA until signing, rollback and recovery path exist | Dedicated OTA design review and rollback test |
| Single route score bias | One heuristic can oscillate or favor poor links | Versioned scoring, hysteresis, simulator tuning, path diversity | Route-flapping tests pass |

## Risk policy

A risk is not considered closed because code compiles. Each stable feature must have a test or operational control that directly addresses its failure mode.

The stable product definition assumes **no microSD card**. Optional storage features cannot weaken or bypass the internal-flash durability requirements.
