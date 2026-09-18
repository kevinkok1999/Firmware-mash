# Technical Risk Register

| Risk | Why it matters | Initial mitigation | Gate before stable |
|---|---|---|---|
| LoRa control-traffic saturation | Multipath/discovery can consume scarce airtime | Cache alternates, bounded discovery, central airtime gate, simulate dense networks | Airtime/control traffic stays within defined budget under target scenarios |
| Routing loops/stale alternates | Can waste airtime and prevent delivery | Sequence/freshness rules, loop checks, expiry, path-scoped invalidation | Loop/stale-route fault tests pass |
| False confidence from compile-only ESP-NOW work | Build success may hide RF/coexistence problems | Keep beta until real hardware acceptance | Repeated T-Deck evidence |
| ESP-NOW peer table pressure | Peer resources are bounded | Dynamic cache + deterministic eviction | Exhaustion test passes without LoRa regression |
| 2.4 GHz coexistence contention | ESP-NOW/Wi-Fi/BLE share RF resources | Explicit RadioScheduler + hardware tests | No deadlock; defined degradation |
| Memory growth from multipath | Embedded RAM is finite | Fixed-capacity route/neighbor/ACK/event pools | High-water marks stay inside budget |
| Retry/failover storm | Loss can trigger retries/discovery | Single ReliabilityManager, backoff/jitter, airtime budgets | High-loss stress stays bounded |
| Duplicate application delivery | Multipath/transports/custody may duplicate packets | Stable PacketId + dedup | Exactly-once presentation tests pass |
| IP socket success mistaken for delivery | Gateway/socket acceptance is not destination delivery | ReliabilityManager remains E2E truth | IP-006 passes |
| Mixed radio/IP duplicate delivery | Same packet may arrive through multiple paths | Transport-independent PacketId/dedup | DED-002 passes |
| Gateway advertisement spoofing | Fake gateways can attract traffic/exhaust state | Authenticated expiring ads + bounded trust policy | GW negative tests pass |
| Gateway/discovery table pressure | Federation can consume RAM | Fixed-capacity pools + deterministic eviction | GW-003/GW-007 pass |
| Federation loop/storm | Peer summaries can loop | TTL/freshness/dedup/rate limits | GW-006 stays bounded |
| Single bootstrap dependency | One service outage could partition assisted delivery | Multiple bootstrap/peers; local radio fallback | GW-005 passes |
| Cloud dependency creeps into core | Offline messaging could stop | Local identity/history/store; IP optional | CFG-LORA-STABLE + offline hardware pass |
| Cellular modem/provider failure | SIM/registration/data loss strands IP path | Replaceable NetifProvider + bearer-down events + radio fallback | CELL-001/CELL-002 pass |
| Cellular support overclaim | eSIM/modem/operator capabilities vary | Advertise only selected tested combinations | Hardware + release evidence |
| IP metadata/privacy exposure | Gateways reveal network metadata | E2E payload protection, minimal logs, no automatic precise GPS | Security/privacy review |
| Federation credential compromise | Bad credentials can expose peers | Standard TLS/security, unique credentials, rotation/revocation | Credential lifecycle tests |
| Battery drain from IP keepalive/modem | Wi-Fi/cellular can dominate power | EnergyManager budgets + bounded reconnect/keepalive | Measured power profile |
| NAT/firewall assumptions | Handheld inbound sessions may fail | Outbound sessions by default; listeners only on gateway roles | CFG-IP-BETA without inbound requirement |
| Volatile store-and-forward | RAM-only pending queue loses messages | Internal durable MessageStore | Reboot/power-cut tests pass |
| Store-and-forward pressure | Offline destinations fill storage | TTL/priority/bounded store/eviction | Near-full deterministic behavior |
| Custody accepted before durable commit | Sender may release responsibility and packet can vanish | Commit-before-accept invariant | CUS-001 power-cut ordering passes |
| Custody acknowledgement mistaken for delivery | UI could falsely show Delivered | Delivery truth remains destination E2E ACK only | REL-001 + custody UI test |
| Custody replication explosion | Mobile relays can create epidemic traffic/storage | Single-active-holder-oriented bounded ownership; no epidemic replication | CUS-004/CUS-006 + resource stress |
| Custody relay reboot loses packet | Accepted responsibility could disappear | Durable custody metadata in MessageStore | CUS-003 passes |
| Custody holder disappears permanently | Responsibility could become stranded | Bounded reconciliation/TTL/previous-holder policy | Failure/reconciliation tests documented |
| Custody store/energy pressure | Low-resource relays could overpromise | Deterministic reject before accept | CUS-005 passes |
| Internal flash wear | High-churn telemetry/state can shorten flash life | Persist only recovery-critical state; batch/append | Wear counters measured |
| Power loss during storage mutation | Interrupted append/compaction can corrupt state | Record integrity/journal/fault injection | Repeated power-cut tests |
| Accidental microSD dependency | Core could fail without removable card | Internal flash is source of truth | Stable no-SD suite passes |
| Partition exhaustion | Firmware growth crowds recovery/store | Measure baseline first; explicit growth/recovery margin | Release budget review |
| Energy-state flapping | Noisy telemetry changes behavior too often | Hysteresis/confidence/staleness | ENG-002/ENG-010 pass |
| Energy policy causes false delivery | Deferred TX could look delivered | ReliabilityManager is delivery truth | ENG-004 passes |
| Low-energy recovery storm | Returning power wakes too much work | Bounded eligibility + jitter/backoff | ENG-003/ENG-008 pass |
| Invalid PMIC/harvest telemetry | Bad readings trigger bad policy | Plausibility/confidence checks | ENG-005 passes |
| Optional harvester becomes dependency | Stock T-Deck could fail without accessory | Provider isolation | CFG-LORA-STABLE + ENG-006 pass |
| Ambient RF power overclaim | Harvested energy is environment-dependent | LAB only until measured | EHW-001 before claims |
| Harvester frontend degrades LoRa | RF loading/desense can reduce range | Separate harvesting antenna default | EHW-003/EHW-004 |
| Brownout during reserve TX | Stored energy may be insufficient | Measured thresholds/hysteresis | EHW-005 |
| Unsafe battery charging integration | External PMIC/storage can violate board limits | Supported hardware path + schematic/thermal review | Hardware power review |
| Security regression through bridges | New transports may bypass E2E protection | Encrypt above routing/transport, negative tests | Security review |
| License contamination | Incompatible code changes distribution obligations | Provenance + compatible-license/original code | Licensing review |
| RF/range claims exceed evidence | Environment varies | Evidence-tiered documented conditions | Documentation review |
| Foundation churn upstream | New upstream changes assumptions | Exact pin + intentional update ADR | Reproducible baseline |
| OTA brick risk | Bad update can make field node unusable | A/B-compatible margin, signing/rollback gate | OTA rollback test before enablement |
| Single route score bias | Heuristic can oscillate/favor bad paths | Versioned scoring + hysteresis + simulation | Route-flap tests pass |
| Release offset/partition mismatch | Wrong flasher metadata can brick or corrupt data | Generate from exact build/partition metadata; never guess offsets | RELSE-005 + recovery test |
| Release evidence overclaim | Compile-only build could be labelled stable | Fail-closed release tier gate | RELSE-006 passes |
| Release artifact/source mismatch | User cannot know what was flashed | Manifest + SHA-256 + git/foundation/toolchain identifiers | RELSE-004/008 pass |
| Recovery path untested | One-click flash failure could leave unusable device | Exact-layout recovery artifact/instructions + target test | RELSE-007 passes |
| Coding trigger starts from stale/incomplete architecture | Agent may implement conflicting design | Final controller report + baseline + required-doc checks | TRIG-001..003 pass |
| GitHub Actions runner unavailable | CI can queue before any step, hiding otherwise valid workflows | Treat as infrastructure blocker; retain reproducible local/container paths and do not claim CI pass | Runner executes or equivalent reproduced evidence exists |

## Risk policy

A risk is not closed because code compiles. Each feature has a direct test or operational gate.

The STABLE product definition assumes **no microSD, no RF-harvesting accessory, no Internet connection and no optional custody dependency**. Optional features cannot weaken internal durability, LoRa backbone, PacketId/chat identity or delivery truth.

Ambient RF remains LAB until physical measurements exist. IP/gateway promotion requires real T-Deck Wi-Fi, multi-peer failure, security/resource and offline-fallback evidence. Cellular requires selected modem/provider evidence. Custody requires real durable carry/reboot/delivery evidence before promotion.

Release engineering is part of product safety: exact flash layout comes from the measured build, and STABLE publication fails closed when evidence is incomplete.
