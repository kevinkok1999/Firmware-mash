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

StatusBar reads immutable/snapshot state only. It does not own network logic.

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
    ├── Firmware/About
    └── Advanced
```

## Home

Inputs:

- unread count;
- queued count;
- network summary;
- battery;
- current time.

Actions:

- open Messages;
- open Contacts;
- open Network;
- open Settings.

No raw routing metrics on Home.

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
CREATED/READY      -> Sending…
SENDING/WAITING_ACK-> Sending…
WAITING_ROUTE      -> Waiting for connection
DEFERRED           -> Queued
DELIVERED          -> Delivered
EXPIRED            -> Expired
FAILED_PERMANENT   -> Could not deliver
```

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
firmware/build identifiers
```

## Settings

Stable normal settings do not expose arbitrary routing/RF tuning. Region changes require clear validation/warnings.

## Event bindings

UI consumes high-level events/snapshots such as:

```text
MESSAGE_STATE_CHANGED
MESSAGE_RECEIVED
UNREAD_COUNT_CHANGED
NETWORK_STATE_CHANGED
QUEUE_COUNT_CHANGED
BATTERY_STATE_CHANGED
STORAGE_WARNING_CHANGED
RADIO_HEALTH_CHANGED
```

UI must not directly subscribe to raw radio callbacks when a normalized application/network state exists.

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
- LoRa receive/transmit completion.

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

The complete flow is usable using the T-Deck itself with no phone required.