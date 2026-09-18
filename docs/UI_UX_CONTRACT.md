# T-Deck UI/UX Contract — Smartphone-Like Experience

## Product principle

Firmware-mash must feel like holding a small familiar mobile phone, not like operating a radio terminal or developer dashboard.

The networking and energy stack may be complex internally, but normal users interact with a simple phone-style interface. LoRa, ESP-NOW Normal/LR, hops, route scores, retries, peer caches, airtime policy, power-policy thresholds and harvesting electronics remain automatic and mostly invisible.

## Overall shell

Use a familiar mobile layout:

```text
┌──────────────────────────────┐
│ 12:34        Mesh ●   🔋 78% │  <- status bar
├──────────────────────────────┤
│                              │
│      Home / active app       │
│                              │
│                              │
├──────────────────────────────┤
│  Back      Home      Recent  │  <- logical navigation
└──────────────────────────────┘
```

The exact input mapping adapts to T-Deck touch/trackball/keyboard hardware, but behavior should match modern phone expectations:

- Back returns one level;
- Home returns to the homescreen;
- opening an app preserves intuitive navigation state;
- focus is always visible;
- keyboard typing goes directly into the active text field;
- no hidden key combinations for core actions.

## Home screen

Keep the homescreen small and familiar. Recommended primary apps/tiles:

```text
💬 Messages     👥 Contacts
📡 Network      ⚙ Settings
```

Optional secondary tile later:

```text
🗺 Map / location
```

Do not expose separate apps for LoRa, ESP-NOW, routing or RF harvesting. Those are implementation details.

The Home screen may also show compact widgets:

- unread message count;
- queued-message count only when non-zero;
- battery;
- simple network state;
- time/date;
- compact energy-saving indicator only when the device is actively conserving power.

## Status bar

The top bar should give a normal phone-like at-a-glance state.

Suggested information:

```text
Time | network health icon | pending/queued indicator when needed | battery
```

Network icon semantics are simple:

- solid/healthy: usable route/network available;
- weak: usable but degraded;
- searching: no current route, discovery/recovery active;
- offline: messages will be queued;
- warning: radio/storage degraded.

Energy semantics remain familiar:

- normal battery percentage/icon;
- charging/external-power icon when known;
- energy-saving indicator in CONSERVE/CRITICAL as appropriate;
- survival indication only when the device intentionally enters the most restrictive power state;
- `Energy assist` may be shown only when compatible, detected harvesting hardware reports credible input.

Do not show raw RSSI/SNR, rectifier voltage, harvested microwatts or supercap voltage permanently in the status bar.

## Messages app

This is the primary experience and should resemble a normal SMS/WhatsApp-style messaging app.

### Conversation list

Each row contains:

- contact/device name;
- last message preview;
- timestamp;
- unread indicator;
- optional tiny delivery/queued indicator.

### Conversation screen

```text
┌──────────────────────────────┐
│ ‹  Brother              info │
├──────────────────────────────┤
│ Hey, are you there?          │
│                       12:30  │
│                              │
│                 I am coming  │
│            Waiting for link  │
│                              │
├──────────────────────────────┤
│ Type a message...      Send  │
└──────────────────────────────┘
```

Required outgoing states use normal language:

```text
Sending…
Waiting for connection
Queued
Delivered
Expired / Could not deliver
```

A queued message remains visible in the correct conversation and is automatically retried when usable connectivity returns. The user never needs to press Send again.

Energy policy may delay background work or a transmission attempt, but the user-visible delivery state remains truthful. An energy deferral is never displayed as Delivered.

## Automatic networking and energy UX

Normal users do not choose:

- LoRa;
- ESP-NOW Normal;
- ESP-NOW Long Range;
- next hop;
- number of hops;
- route;
- retry count;
- routing weights;
- EnergyManager thresholds;
- MPPT/rectifier settings;
- energy-reservoir/supercap thresholds.

The firmware chooses automatically.

An optional message-details view may show post-delivery information such as:

```text
Delivered
Transport: LoRa
Hops: 2
```

or

```text
Delivered
Transport: ESP-NOW LR
```

This is informational, not a required setting.

## Contacts app

Familiar phone-style contact list:

- name;
- small status if useful;
- search/filter;
- open contact -> message;
- QR/import/pairing actions only where supported by the selected identity/security foundation.

Technical node IDs belong in Details/Advanced, not as the primary contact label.

## Network app

Default page is simple, visual and human-readable:

```text
Network OK
3 reachable nodes
1 message queued

[ simple local topology / node list ]
```

Possible states:

```text
Network OK
Weak connection
Searching for route
Offline — messages will be queued
Radio degraded
```

A simple node/topology view may visualize direct/known reachable nodes, but it must not require users to understand routing protocols.

### Advanced diagnostics

Behind an explicit Advanced action only:

- LoRa RSSI/SNR;
- current transport;
- ESP-NOW Normal/LR capability;
- hop count/path details;
- route candidates;
- airtime/congestion;
- queue/pool pressure;
- storage health;
- RAM/PSRAM high-water marks;
- EnergyManager state;
- external-power presence;
- energy-policy deferral counters;
- optional harvesting-provider availability/confidence;
- raw harvest power/reservoir voltage only when actual compatible hardware exposes measured data.

Advanced diagnostics never block ordinary messaging.

## Settings app

Phone-style grouped settings:

```text
Device
Display
Notifications
Contacts / identity
Radio region
Storage
Battery & power
Firmware / About
Advanced
```

First-level settings use plain language. Developer/radio/power-electronics tuning remains inside Advanced and may be read-only in STABLE builds where changing it would undermine validated defaults, battery safety or regional compliance.

`Battery & power` may expose simple user choices such as display timeout or an allowed battery-saver preference if validated. It must not expose raw PMIC/harvester engineering controls to normal users.

## First-run experience

First boot should feel like lightweight phone onboarding, not radio setup.

Recommended flow:

1. Welcome;
2. choose/confirm device name;
3. confirm legal radio region (EU build: EU868);
4. create/import required identity/channel/contact material according to the selected security foundation;
5. show a short success screen;
6. enter Home.

No routing weights, modem engineering, ESP-NOW mode, retry values or harvesting/power-electronics settings appear in normal onboarding.

## Notifications

When the T-Deck receives a message while another screen is active, provide a familiar compact notification banner/icon where the hardware/UI stack allows it.

Queued messages should not generate repeated noisy notifications while retrying. Notify on meaningful state changes such as delivered, failed permanently, storage warning or received message.

Energy state changes should normally be quiet. Notify only when meaningful to the user, such as entry into a severe survival/critical condition or a hardware/power fault that affects operation.

## Visual hierarchy

Design for the small T-Deck display:

1. large clear title/current contact;
2. core content/messages;
3. one obvious primary action;
4. concise status;
5. advanced data only on demand.

Rules:

- readable default text size;
- high contrast;
- generous focus/highlight indication;
- do not depend on color alone for status;
- consistent iconography;
- avoid dense tables in normal screens;
- touch targets large enough for practical use;
- physical keyboard/trackball navigation remains first-class.

## Responsiveness

UI must never block on radio discovery, route search, flash compaction, delivery ACK, EnergyManager sampling or harvester-provider I/O.

The UI consumes snapshots/events and immediately reflects state. Long-running operations show a visible status instead of freezing.

## Core interaction acceptance

A normal user must be able to perform this without technical knowledge:

```text
Power on
-> Home
-> Messages
-> select contact
-> type on T-Deck keyboard
-> Send
```

If no connection exists:

```text
message remains in conversation
-> Waiting for connection
-> internally stored
-> connectivity returns later
-> automatic retry
-> Delivered
```

No second Send press, transport selection, route configuration or energy-engineering configuration is required.

## Design constraint

The finished experience should be recognizable to someone familiar with a normal smartphone messaging app within seconds, while still exposing an optional powerful Advanced diagnostics layer for expert users.

Ambient RF Energy Assist must feel like an invisible optional efficiency feature, not like a second operating mode the user has to understand.