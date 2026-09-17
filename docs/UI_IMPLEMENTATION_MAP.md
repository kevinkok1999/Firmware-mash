# UI Implementation Map

## Purpose

Translate `UI_UX_CONTRACT.md` into implementation-ready screens, states and event bindings so the coding pass does not need to invent navigation or status behavior.

## App shell

Persistent shell responsibilities:

```text
StatusBar
HomeNavigator
AppNavigator
NotificationBanner
GlobalErrorBanner
```

StatusBar reads immutable/snapshot state only. It does not own network or energy logic.

## Screen map

```text
Home
├── Messages
│   ├── ConversationList
│   └── Conversation
│       └── MessageDetails (optional/advanced)
├── Contacts
│   ├── ContactList
│   └── ContactDetails
├── Network
│   ├── NetworkOverview
│   └── AdvancedDiagnostics
└── Settings
    ├── Device
    ├── Display
    ├── Notifications
    ├── Identity/Contacts
    ├── RadioRegion
    ├── Storage
    ├── BatteryPower
    ├── Firmware/About
    └── Advanced
```

## Home

Inputs:

- unread count;
- queued count;
- network summary;
- battery;
- current time;
- compact energy-saving state when active.

Actions:

- open Messages;
- open Contacts;
- open Network;
- open Settings.

No raw routing or harvesting metrics on Home.

## ConversationList

Each list item contains:

```text
contact/display name
last-message preview
timestamp
unread count
small delivery/queued marker where relevant
```

Sort by most recent logical conversation activity.

## Conversation

Core UI objects:

```text
Header(contact)
MessageList
Composer
SendAction
DeliveryStateText/Icon
```

Outgoing message mapping:

```text
CREATED/READY       -> Sending…
SENDING/WAITING_ACK -> Sending…
WAITING_ROUTE       -> Waiting for connection
DEFERRED            -> Queued
DELIVERED           -> Delivered
EXPIRED              -> Expired
FAILED_PERMANENT    -> Could not deliver
```

An energy-deferred transmission remains pending/queued according to its real reliability state. It never maps to Delivered until end-to-end evidence exists.

A WAITING_ROUTE/DEFERRED message remains in the same bubble position; automatic retry updates the status in place. Do not create a second bubble for the retry.

## Contacts

Normal fields:

```text
display name
reachable/simple status when known
last seen (optional, human-readable)
Message action
```

Advanced details may reveal technical node/identity information.

## NetworkOverview

Primary states:

```text
Network OK
Weak connection
Searching for route
Offline — messages will be queued
Radio degraded
```

Secondary information:

```text
reachable node count
queued-message count
simple topology/node list
```

No route editing in the normal view.

## BatteryPower

Normal-user view may show:

```text
Battery percentage if known
Charging / external power
Battery saver status
Survival mode status when active
Energy assist active only with detected compatible hardware
```

Do not expose raw rectifier/PMIC/reservoir engineering parameters here.

## AdvancedDiagnostics

Read-only by default in STABLE:

```text
current link/transport
ESP-NOW Normal/LR capability/mode
LoRa RSSI/SNR
hop count/current path summary
route candidates
retry counters
queue/pool pressure
storage health
RAM/PSRAM high-water marks
airtime/congestion
EnergyManager state
external-power state
energy-policy deferrals
optional harvest-provider availability/confidence
measured harvest power/reservoir telemetry if actual hardware provides it
firmware/build identifiers
```

## Settings

Stable normal settings do not expose arbitrary routing/RF/power-electronics tuning. Region changes require clear validation/warnings.

## Event bindings

UI consumes high-level events/snapshots such as:

```text
MESSAGE_STATE_CHANGED
MESSAGE_RECEIVED
UNREAD_COUNT_CHANGED
NETWORK_STATE_CHANGED
QUEUE_COUNT_CHANGED
BATTERY_STATE_CHANGED
ENERGY_STATE_CHANGED
ENERGY_SOURCE_CHANGED
STORAGE_WARNING_CHANGED
RADIO_HEALTH_CHANGED
```

UI must not directly subscribe to raw radio, ADC, PMIC or harvesting-provider callbacks when a normalized application/network/energy state exists.

## Input mapping

Physical keyboard:

- type in focused composer/text field;
- Enter/defined send action sends only when intended by final board UX mapping;
- Back/Escape maps predictably to Back.

Trackball/touch:

- move/select list items and controls;
- visible focus/highlight;
- no tiny required targets.

Exact keycodes are adapted to the pinned foundation board/input layer after baseline inspection.

## Non-blocking rule

The UI thread/task cannot block on:

- route discovery;
- end-to-end ACK;
- MessageStore compaction;
- ESP-NOW peer discovery;
- LoRa receive/transmit completion;
- EnergyManager sampling;
- harvester-provider I/O.

Every long-running operation is represented as state and returns control to the UI loop.

## Acceptance flow

```text
Boot
-> Home visible
-> Messages
-> select Contact
-> type message
-> Send
-> message bubble appears immediately
-> if unreachable: Waiting for connection
-> route/link later appears
-> status becomes Sending… automatically
-> E2E ACK
-> Delivered
```

Energy-state changes may alter background behavior without changing this mental model. The complete flow is usable using the T-Deck itself with no phone required.