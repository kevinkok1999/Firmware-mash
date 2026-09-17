# IP Backhaul and Gateway Federation Contract

## Purpose

Add Internet-assisted transport to Firmware-mash without weakening its off-grid-first design. Wi-Fi and cellular connectivity are optional IP bearers below the same `HybridRouter`; they are not separate chats, identities or routing stacks.

The product invariant is:

```text
one contact -> one conversation -> one logical PacketId
```

The selected physical path may change before or during delivery without creating a second conversation or second user-visible message.

## Core user experience

The normal user sends from the same conversation regardless of path:

```text
Contact: Brother
Message: "Where are you?"
Send
```

Firmware-mash may deliver that logical message using, for example:

```text
LoRa -> receiver
ESP-NOW LR -> receiver
LoRa -> local gateway -> IP -> remote gateway -> LoRa -> receiver
Wi-Fi IP -> remote gateway -> LoRa -> receiver
cellular PPP -> federation -> remote gateway -> LoRa -> receiver
store/wait -> later recovered path -> receiver
```

Transport changes do not create a new chat. Delivery truth remains owned by `ReliabilityManager` and is complete only after valid end-to-end evidence.

## Layering

```text
UI / Conversation Service
        |
ReliabilityManager / MessageStore
        |
HybridRouter                    <- only routing authority
        |
TransportManager
  |        |          |
  |        |          +-- mog_transport_ip
  |        |                |
  |        |                +-- NetifProvider: Wi-Fi
  |        |                +-- NetifProvider: cellular PPP
  |        |                +-- future Ethernet/other IP bearer
  |        |
  |        +-- mog_transport_espnow
  |
  +-- mog_transport_lora

GatewayManager / GatewayDiscovery provide gateway reachability/capability evidence to HybridRouter.
They do not become a second routing engine.
```

## Wi-Fi rule

A normal third-party Wi-Fi access point may provide ordinary IP connectivity when the T-Deck is legitimately connected to that network. It is **not** treated as an unconfigured Firmware-mash relay and does not understand Firmware-mash packets.

Local same-LAN peer discovery may be implemented with standard IP discovery mechanisms where supported, but it must tolerate access-point/client-isolation environments where peer-to-peer LAN traffic is blocked.

## Cellular rule

Cellular data is optional and hardware-dependent. A stock T-Deck Plus is not assumed to contain a cellular modem.

The preferred architecture is a replaceable cellular `NetifProvider` backed by a supported modem/PPP implementation. SIM/eSIM capability depends on the selected modem hardware/provider and is never fabricated in software.

Cellular failure, SIM absence, registration loss or data-session loss must degrade only the IP path. LoRa/off-grid operation continues.

## IP transport

`mog_transport_ip` carries the same logical protected Firmware-mash envelope over an IP session. The IP bearer may be Wi-Fi or cellular without changing PacketId, conversation identity or application payload semantics.

Conceptual responsibilities:

```text
connect / disconnect
bearer-up / bearer-down reporting
session establishment
bounded TX/RX queues
framing over the IP stream/datagram substrate
keepalive with bounded cadence
path MTU reporting
latency/loss/cost/metered metrics
failure/recovery events
```

The first implementation should prefer a simple maintained ESP-IDF networking path rather than inventing a new transport stack.

## Secure session rule

Do not invent custom cryptography.

Internet-facing sessions use maintained standard security from the selected SDK/foundation, such as ESP-TLS/mbedTLS where appropriate. The Firmware-mash end-to-end protected message envelope remains authoritative for user-message confidentiality; transport security is additive.

A gateway must not require plaintext user-message content merely to relay a protected packet.

Certificate/peer-authentication policy, key provisioning and rotation require an explicit security review before public federation.

## Gateway roles

### Handheld

A normal T-Deck:

- sends/receives local radio traffic;
- may use Wi-Fi IP directly when configured;
- may use cellular IP only with compatible external/integrated modem hardware;
- never requires Internet for core messaging.

### Edge Gateway

A powered T-Deck, compatible gateway appliance or future node may bridge local Firmware-mash reachability to IP.

An Edge Gateway:

- advertises bounded capabilities/reachability;
- forwards only through `HybridRouter`/gateway policy;
- may expose LoRa/ESP-NOW local reachability to the IP backbone;
- may store protected pending packets only under explicit bounded store/custody policy;
- cannot silently downgrade end-to-end security.

### Federation Peer / Backbone Gateway

A gateway-class process/device may maintain IP sessions to several other federation peers. It exchanges bounded reachability summaries and protected packet traffic without requiring one mandatory central server.

## Gateway discovery

Discovery is multi-source and bounded.

Preferred discovery sources, in order of trust/policy rather than hard-coded transport priority:

1. authenticated/signed gateway advertisements received through the existing Firmware-mash network;
2. local-LAN discovery where available and permitted;
3. a small configured bootstrap set of known federation entrypoints;
4. bounded federation peer advertisements learned from already-authenticated peers.

Discovery information has:

- gateway identity;
- protocol version;
- capabilities;
- freshness/expiry;
- reachability scope/summary;
- cost/metered indication when known;
- energy/power class where policy permits;
- authentication evidence;
- bounded size and announce rate.

A discovery advertisement is evidence, not automatic permission to route sensitive traffic. Policy/authentication still applies.

## No single-server dependency

Firmware-mash federation must not require one irreplaceable cloud server for message delivery.

A bootstrap service may help peers find each other, but:

- multiple bootstrap endpoints can be configured;
- loss of all bootstrap endpoints does not break already-known federation peers;
- loss of all Internet federation still leaves local LoRa/ESP-NOW/store-forward operation intact;
- persistent chats/messages remain local and do not depend on a cloud account.

Do not claim global decentralization until multi-peer/failure testing proves it.

## NAT/firewall strategy

Handhelds and typical cellular gateways are assumed to be behind NAT/firewalls. The initial design therefore prefers outbound authenticated sessions to reachable federation peers rather than requiring every T-Deck to accept public inbound connections.

Publicly reachable gateway listeners are gateway-class infrastructure, not a requirement for normal handhelds.

More advanced peer-to-peer NAT traversal may be researched later, but it must not be required for the first usable IP backhaul.

## Reachability summaries

Federation peers exchange bounded destination/reachability summaries, not global copies of every node's route table.

Requirements:

- bounded entries;
- expiry/freshness;
- versioning;
- loop prevention;
- anti-replay/authentication where applicable;
- aggregation/summarisation when scale evidence requires it;
- no unlimited global flooding.

Exact summarisation is simulator-tuned and may evolve behind protocol versioning.

## Route scoring

An IP-assisted path is a normal candidate path under `HybridRouter`.

Metrics may include:

```text
delivery probability
latency
loss
freshness
hop/gateway count
congestion
energy cost
metered-data cost class
privacy/trust policy
path diversity
```

Internet is not automatically preferred simply because it is available. A direct reliable LoRa path may remain preferable depending on policy/score.

## User policy modes

Normal UI may expose only high-level policy, never per-message route engineering:

```text
AUTO
OFF-GRID ONLY
INTERNET ASSIST
```

`AUTO` is the normal default once IP backhaul reaches the required evidence tier.

- `AUTO`: HybridRouter chooses any validated allowed path.
- `OFF-GRID ONLY`: IP backhaul is excluded; local radio/store-forward remains.
- `INTERNET ASSIST`: IP paths receive policy preference when healthy, but radio fallback remains automatic.

The active mode does not split conversation history.

## Failure and recovery

Examples:

```text
Wi-Fi lost -> IP_TRANSPORT_DOWN -> try valid alternate -> otherwise WAITING_ROUTE
cellular lost -> IP bearer down -> LoRa/ESP-NOW candidates remain
federation peer lost -> invalidate only affected IP paths
Internet returns -> TRANSPORT_RECOVERED/GATEWAY_AVAILABLE -> queued message becomes retry-eligible
```

The same PacketId survives all transitions.

A recovered IP gateway must not trigger a retry storm; ReliabilityManager/backoff and bounded gateway/discovery policy still apply.

## Energy integration

EnergyManager provides a normalized budget to IP networking.

Wi-Fi/cellular background activity is power-aware:

- external power may permit longer gateway/listener sessions;
- low-battery handhelds reduce background discovery/keepalive intensity;
- user-originated messaging remains governed by delivery policy rather than silently dropped;
- a cellular modem's measured power cost is included only after real hardware characterization.

No transport reads raw PMIC/harvester state directly.

## Privacy and metadata

Gateway/federation design minimizes metadata exposure:

- user content remains end-to-end protected;
- logs are bounded and avoid plaintext message bodies;
- reachability advertisements expose only required routing metadata;
- precise GPS/location is not automatically published as part of gateway discovery;
- public federation telemetry is opt-in/operational, not required for ordinary chats.

## Abuse and resilience controls

Internet-facing gateways require:

- authentication before accepting privileged federation behavior;
- bounded per-peer queues;
- rate limits for discovery/control traffic;
- replay/duplicate handling;
- resource-exhaustion protection;
- malformed-frame rejection;
- connection/session limits;
- observable health counters;
- fail-closed handling for unsupported protocol/security versions.

A hostile or broken gateway must not be able to make the local LoRa-only device unbootable or permanently stall the network task.

## Build configurations

The design introduces two configurations in addition to existing ones:

```text
CFG-IP-BETA
  stock T-Deck Wi-Fi IP backhaul enabled
  cellular provider optional/OFF
  federation client enabled
  gateway server role optional/OFF on handheld

CFG-GATEWAY-BETA
  IP backhaul enabled
  GatewayManager enabled
  federation peer/listener capabilities enabled where target resources allow
  Wi-Fi/cellular bearer selected by target hardware
```

`CFG-LORA-STABLE` remains able to compile and operate with all IP/gateway code disabled.

## Evidence tiers

Before promotion:

### Wi-Fi IP backhaul

Requires real T-Deck evidence for connect, send/receive, drop/recovery and automatic fallback.

### Cellular backhaul

Requires real selected modem/SIM-provider evidence. Compile-only modem support remains EXPERIMENTAL/BETA as labelled.

### Federation

Requires multi-gateway tests including peer loss, bootstrap loss, loop prevention, duplicate delivery, queue pressure and recovery.

No "30 km / 300 km / worldwide" availability guarantee may be made from architecture alone. Internet-assisted distance depends on actual IP connectivity and reachable participating gateways.

## Missing-code rule

If `mog_transport_ip`, NetifProvider, GatewayManager, GatewayDiscovery, framing, policy or simulator components do not exist upstream, they are implemented as original Firmware-mash `mog_` components against documented SDK/vendor APIs.

Missing code is not a reason to remove the approved feature. Missing physical modem/network capability remains honestly hardware/evidence-gated.
