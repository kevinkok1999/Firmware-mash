# Transport Coexistence and Framing Contract

## Purpose

Prevent a class of bugs that compile cleanly but fail in real mixed-transport use: ESP-NOW/Wi-Fi channel conflicts, stale callbacks after bearer changes, stream framing errors, MTU changes and nested fragmentation.

This contract sits below HybridRouter. It does not add another routing authority.

## ESP32-S3 2.4 GHz reality

The T-Deck Plus ESP32-S3 has one 2.4 GHz Wi-Fi/BLE RF resource. Firmware must not model Wi-Fi and ESP-NOW as independent simultaneous radios.

When Wi-Fi STA is associated, the AP/home channel constrains ESP-NOW operation. ESP-NOW peers must use the current compatible channel; an old peer-channel assumption cannot remain valid after association/roam/channel change.

## RadioScheduler ownership

`mog_radio_scheduler` owns 2.4 GHz coexistence policy for Wi-Fi scans/association/connected activity, ESP-NOW Normal/LR and BLE work.

Hard rules:

- HybridRouter chooses logical path; it does not set Wi-Fi channels directly;
- ESP-NOW adapter does not silently retune the active Wi-Fi STA away from its AP;
- Wi-Fi provider reports channel/home-channel generation changes;
- ESP-NOW peer/link state is invalidated or reacquired when the effective channel changes;
- during an unresolved channel transition, ESP-NOW is temporarily unavailable/degraded rather than pretending TX succeeded;
- LoRa remains an independent fallback path while 2.4 GHz is unavailable/reconciling;
- scans/roams are bounded and EnergyManager-aware;
- no UI-visible second chat or resend is created by channel changes.

## Channel generation

Use a monotonically changing runtime `radio_channel_generation` or equivalent whenever the effective 2.4 GHz channel context changes.

ESP-NOW send completions/discovery results carry the generation they started under. A late result from an older channel generation cannot mark the current peer/path healthy.

## Wi-Fi roam/reconnect sequence

Conceptually:

```text
Wi-Fi AP/channel change detected
-> increment channel generation
-> mark affected ESP-NOW link evidence stale/unavailable
-> suppress new ESP-NOW TX until scheduler confirms compatible channel
-> refresh/reacquire bounded peer/channel state
-> publish ESP-NOW link recovered when valid
-> queued messages become retry-eligible through normal ReliabilityManager path
```

If reacquisition fails, HybridRouter keeps using another valid route or WAITING_ROUTE.

## ESP-NOW peer cache rule

Peer cache entries include channel-context validity. A MAC address being present in the peer table is not sufficient proof that it can currently be reached on the active channel.

Peer eviction/re-add after channel change is bounded. Private/link keys and identity material follow the selected maintained security implementation; no custom crypto is introduced.

## IP stream framing

If federation uses a stream substrate such as TCP/TLS, one `write()` or `read()` is not one Firmware-mash frame.

`mog_ip_framing` must implement explicit bounded framing, for example a reviewed version/type/length header around the protected logical frame.

Required parser behavior:

- accept partial header/body reads;
- accept multiple coalesced frames in one read;
- reject declared lengths above configured maximum before allocation/copy;
- reject overflow/underflow and impossible version/type combinations;
- timeout incomplete frames with bounded state;
- never allocate unbounded memory from a peer-controlled length;
- one socket/TLS write result remains link/session evidence, not Delivered.

Datagram substrates still apply the same maximum-frame/version validation.

## MTU and fragmentation ownership

A mixed path can move between a large IP MTU and much smaller LoRa/ESP-NOW constraints. Transport change must not create nested uncontrolled fragmentation.

Rules:

1. exactly one logical packet/fragmentation owner is selected from the pinned foundation/Firmware-mash packet layer;
2. link adapters may apply only their required bounded link framing, not independently create a second application fragmentation scheme;
3. fragment count and total reassembly size are bounded before resource reservation;
4. reassembly is keyed by stable PacketId + fragment generation/identity;
5. path change during retry may re-encode/re-fragment the same logical protected message using the current path MTU, but old and new fragments still deduplicate to one PacketId;
6. an incomplete old-path fragment set expires and cannot block a complete new-path delivery;
7. relay re-fragmentation of opaque protected data is allowed only through the single reviewed packet/framing owner; no nested fragment-of-fragment accumulation.

Exact MTUs and fragment sizes come from the pinned build/hardware measurement.

## Session/bearer generation

Every IP connection/session has a generation. DNS/TLS/connect completions and socket callbacks from generation N cannot mutate generation N+1 state after reconnect.

A bearer transition Wi-Fi -> cellular or cellular -> Wi-Fi preserves logical PacketId and conversation while establishing a fresh transport/session generation.

## Required tests

```text
RAD-002 Wi-Fi association channel change invalidates stale ESP-NOW peer evidence
RAD-003 ESP-NOW recovers on compatible current Wi-Fi channel without manual user action
RAD-004 unresolved ESP-NOW channel transition falls back to LoRa/WAITING_ROUTE
RAD-005 stale ESP-NOW completion from old channel generation cannot mark link healthy
RAD-006 Wi-Fi roam/scanning under message load remains bounded and UI responsive
RAD-007 Wi-Fi/ESP-NOW/BLE coexistence stress has no deadlock
IP-009 TLS stream parser handles split header/body reads
IP-010 TLS stream parser handles multiple frames in one read and rejects oversized length
IP-011 old IP session callback cannot tear down new session generation
WIR-004 path MTU change re-fragments same logical PacketId without duplicate app delivery
WIR-005 incomplete old-path fragments expire while complete alternate-path packet succeeds
```

## Promotion rule

Compilation/simulation proves parser/state-machine logic. Wi-Fi + ESP-NOW coexistence remains hardware-evidence-gated on real T-Deck Plus units before STABLE promotion.
