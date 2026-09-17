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
| IP socket success mistaken for delivery | Gateway/socket acceptance is not destination delivery | ReliabilityManager remains source of E2E delivery truth | IP-006 passes with destination ACK withheld |
| Mixed radio/IP duplicate delivery | Same PacketId may arrive through radio and federation | Transport-independent PacketId/dedup | DED-002 shows one app delivery |
| Gateway advertisement spoofing | Fake gateways could attract traffic or exhaust state | Authenticate advertisements, trust policy, expiry and bounded tables | GW-001/GW-002/GW-008 negative tests pass |
| Gateway/discovery table pressure | Public/federated discovery can consume RAM | Fixed-capacity gateway/advertisement/session pools with deterministic eviction | GW-003/GW-007 pass under pressure |
| Federation control-loop/storm | Gateway summaries can loop across peers | TTL/freshness/duplicate suppression/rate limits and bounded summaries | GW-006 simulator stress remains bounded |
| Single bootstrap dependency | One dead service could partition Internet-assisted delivery | Multiple bootstrap peers; retain authenticated active peers; radio fallback | GW-005 passes after bootstrap removal |
| Federation peer failure over-invalidates routes | Losing one peer must not destroy unrelated paths | Path-scoped gateway/peer failure evidence | GW-004 multi-peer failover passes |
| Cloud dependency creeps into core | Conversations/history could stop working offline | Local identity/history/MessageStore; IP optional; CFG-LORA-STABLE regression | CFG-007 + offline hardware acceptance pass |
| Cellular modem/provider failure | Registration/SIM/data loss can strand IP path | Replaceable NetifProvider; explicit bearer-down events; radio fallback | CELL-001/CELL-002 pass on selected hardware |
| Cellular support overclaim | Modem/eSIM/network capabilities vary by hardware/provider | Advertise only selected tested modem/provider combinations | RELSE-003 + hardware evidence |
| IP metadata/privacy exposure | Internet gateways can reveal addresses/traffic patterns | E2E payload protection, minimal logs/advertisements, no automatic precise GPS publication | Security/privacy review before public federation |
| Federation credential compromise | Shared/unrotated credentials could expose gateway network | Standard TLS/security libraries, unique credentials, rotation/revocation design | Security review and credential lifecycle test before public deployment |
| Battery drain from IP keepalive/modem | Wi-Fi/cellular background activity can dominate handheld power | EnergyManager budgets, bounded reconnect/keepalive, gateway roles favor powered nodes | Real T-Deck/modem power profile recorded |
| NAT/firewall assumptions | Handhelds behind NAT may not accept inbound sessions | Outbound sessions by default; public listeners only on gateway-class targets | CFG-IP-BETA works without inbound socket |
| Volatile store-and-forward | A RAM-only queue loses undelivered messages on reboot/power loss | Dedicated bounded internal-flash MessageStore plus RAM index/cache | No-SD reboot/power-cut queue recovery test passes |
| Store-and-forward pressure | Offline destinations can fill internal flash/RAM | TTL, bounded queue, priorities, deterministic eviction | Near-full store test has deterministic behavior |
| Internal flash wear | Persisting high-churn route/RF/energy metrics can shorten flash life | Persist only critical durable state; keep route/RF/energy data in RAM/PSRAM; append/batch writes | Endurance/compaction counters measured and no per-metric persistent writes |
| Power loss during storage mutation | Handheld battery/reset can interrupt append, ACK-delete or compaction | Record integrity, recoverable journal/filesystem, isolated partitions | Repeated power-cut tests recover without boot loop or identity loss |
| Accidental microSD dependency | Removable card can be absent/corrupt/removed | Core state must live in internal flash; SD optional only | Full stable acceptance suite passes with no card installed |
| Partition exhaustion | Firmware growth can crowd out OTA/recovery or durable messages | Measure baseline image first; reserve explicit app/data/recovery growth margins | Partition budget reviewed on every release-size increase |
| Battery drain from hybrid discovery | Always-on Wi-Fi/ESP-NOW/BLE/GNSS/display can make handheld impractical | EnergyManager, role-aware discovery windows, radio scheduler, display idle policy, measured power budgets | Real T-Deck battery/power profile documented for stable features |
| Energy-state flapping | Noisy ADC/source telemetry can rapidly change routing/power behavior | Hysteresis, confidence/staleness checks, bounded sampling | ENG-002/ENG-010 pass under noisy telemetry |
| Energy policy causes false delivery | Deferring a TX must not look delivered | ReliabilityManager remains source of delivery truth | ENG-004 shows no Delivered state without E2E ACK |
| Low-energy recovery storm | Returning power could wake many retries/discovery tasks at once | Bounded eligibility, jitter/backoff, AirtimeManager remains authoritative | ENG-003/ENG-008 stay bounded |
| Invalid PMIC/harvest telemetry | Bad readings could trigger unsafe policy | Plausibility checks, confidence/staleness, last-safe state | ENG-005 passes; no crash/route corruption |
| Optional harvester becomes dependency | Stock T-Deck could stop working without accessory | Provider isolation, CFG-LORA-STABLE with harvester OFF | ENG-006 + CFG-004 pass |
| Ambient RF power overclaim | Ambient energy is environment-dependent and often very small | Treat as LAB energy assist only; measure actual hardware/environment | EHW-001 evidence before any power/runtime claim |
| Harvester frontend degrades LoRa | Shared/loading RF path can detune/desensitize communications | Separate harvesting antenna default; shared antenna only as measured experiment | EHW-003/EHW-004 before promotion |
| Brownout during energy-reserve TX | Stored energy may be insufficient under real TX load | Measured reservoir threshold/hysteresis; persistent message state before attempt | EHW-005 passes repeatedly |
| Unsafe/unsupported battery charging integration | External PMIC/storage can violate board/battery assumptions | Use supported charging path/components; hardware review before connection | Hardware schematic/power review + thermal/power tests |
| Security regression through bridges | New transports can accidentally bypass E2E protection | Encrypt above routing/transport, authenticate control where feasible | Security review and negative tests |
| License contamination | GPL/custom-license code could change distribution obligations | Provenance log, MIT-first reuse, clean-room/original implementation where needed | Licensing review before merge/release |
| RF claims exceed evidence | Range is environment/hardware dependent | Record conditions and evidence tier; no universal range promises | Documentation review |
| Backscatter/RIS overreach | Standard T-Deck cannot gain arbitrary ambient-RF features by firmware | Keep as disabled external-hardware/RF-assist modules | Real compatible hardware required |
| Foundation churn upstream | Fast-moving upstream can break our assumptions | Pin exact commit; update intentionally through reviewed ADR/PR | Baseline reproducible from pin |
| OTA brick risk | Power loss/bad image can render field nodes unusable | Reserve A/B-compatible layout; defer OTA until signing, rollback and recovery path exist | Dedicated OTA design review and rollback test |
| Single route score bias | One heuristic can oscillate or favor poor links | Versioned scoring, hysteresis, simulator tuning, path diversity | Route-flapping tests pass |

## Risk policy

A risk is not considered closed because code compiles. Each stable/beta feature must have a test or operational control that directly addresses its failure mode.

The stable product definition assumes **no microSD card, no RF-harvesting accessory and no Internet connection**. Optional storage, harvesting, IP or gateway features cannot weaken or bypass the internal-flash durability, LoRa-backbone or delivery-truth requirements.

Ambient RF Energy Assist cannot leave LAB based on simulator results alone. It requires actual harvested-power and communications-impact measurements.

IP/gateway federation cannot be promoted based on a working socket alone. It requires real T-Deck Wi-Fi evidence, multi-gateway failure tests, security/resource-pressure tests and proof that Internet loss preserves the same local chat/off-grid behavior. Cellular support additionally requires selected real modem/provider evidence.
