# Runtime Safety Contract

## Purpose

Freeze the failure behavior that is easy to miss during an embedded implementation: bounded queue overflow, task stalls, stale generations, reboot-safe identity/time semantics and recovery from lost internal events.

This contract does not add a second routing/reliability engine. It constrains how the existing `network_task`, ReliabilityManager, MessageStore, transports and UI exchange work.

## Event classes and overflow policy

A single undifferentiated drop-oldest queue is forbidden because it can lose a delivery ACK, storage completion or custody commitment while retaining low-value telemetry.

Events are classified conceptually as:

```text
TERMINAL_CONTROL
  DELIVERY_ACK
  durable storage commit/failure
  custody accept/reject after durable commit
  fatal transport/provider state transitions

CONTROL
  route/link/gateway up/down
  retry timeout
  recovery/resync request

TELEMETRY
  RSSI/SNR samples
  rolling metrics
  UI-only refresh hints
```

Required behavior:

- queues/pools remain bounded;
- telemetry is coalescible/droppable first;
- terminal-control events are never intentionally evicted by telemetry;
- if a terminal/control event cannot be enqueued, increment a health counter and request a bounded state resynchronization rather than silently continuing with divergent state;
- repeated identical route/link-up events may be coalesced;
- ISR/radio callbacks never block waiting for queue space.

## State resynchronization

Because bounded queues can still overflow under fault conditions, authoritative state must be queryable/recoverable.

Examples:

- ReliabilityManager can reconcile pending PacketIds against MessageStore and recently received ACK evidence;
- TransportManager can publish current adapter up/down state after an overflow;
- GatewayManager can republish a bounded snapshot of current authenticated gateway/session state;
- EnergyManager can republish its latest immutable policy snapshot;
- storage worker can reconcile command/result generation numbers.

A queue overflow therefore degrades freshness/performance but must not permanently strand the state machine.

## Generation and stale-completion rule

Asynchronous operations that can be restarted use a generation/epoch token or equivalent.

Examples:

```text
route discovery generation
IP session generation
storage compaction generation
custody transfer epoch
firmware update attempt generation
```

A completion/result from an older generation is ignored or reconciled; it cannot overwrite the state of a newer operation.

This prevents a late `SESSION_DOWN` or storage completion from tearing down/replacing a newly recovered state.

## Task blocking rules

`network_task` is the single logical writer of route/reliability/network state and therefore must not perform long blocking work.

Forbidden on the network-state owner:

- flash compaction;
- DNS/TLS connect waits;
- modem registration waits;
- long filesystem scans;
- UI rendering;
- arbitrary sleep/delay waiting for hardware.

These operations run in bounded workers and return events/results. Watchdog/health instrumentation must detect a worker or network task that stops making progress.

## Critical control reserve

Low-energy policy may reduce discovery, relay willingness, UI brightness and optional IP background work, but it must retain a bounded reserve for protocol completion such as:

- receiving/sending a required delivery ACK when legal/possible;
- persisting a terminal delivery/storage state;
- rejecting custody safely rather than accepting and then starving it;
- processing energy recovery state changes;
- user-originated emergency/critical-class work subject to AirtimeManager/regulatory rules.

`SURVIVAL` cannot mean "drop every control event forever".

## Reboot-safe PacketId rule

PacketId generation must follow `PACKET_DELIVERY_CONTRACT.md`. A volatile counter reset is forbidden. Generator state/entropy must not roll backward into a collision window after power cut, reboot or firmware rollback.

## Time domains

Runtime scheduling uses monotonic time. Durable message TTL uses persisted lifetime semantics and cannot compare a pre-reboot boot-relative absolute millisecond value against the new boot clock.

Gateway/route/link freshness can be rebuilt after reboot; durable user-message lifetime cannot silently depend on a volatile monotonic epoch.

## Reboot-safe exactly-once presentation

The destination's durable conversation/message layer must be idempotent by stable logical message identity. A RAM-only dedup cache is an optimization, not the only protection against duplicate UI presentation after reboot.

## Required tests

```text
EVT-001 telemetry flood cannot evict terminal delivery/storage event
EVT-002 forced control-queue overflow triggers bounded resync and converges
EVT-003 late stale-generation completion cannot overwrite recovered state
TSK-001 network task remains responsive while storage compacts
TSK-002 network task remains responsive during DNS/TLS/modem delays
ENG-011 survival policy preserves bounded protocol-completion reserve
PID-001 PacketId generator does not repeat across reboot
PID-002 interrupted generator-state persistence does not create reuse
PID-003 firmware rollback does not re-enter a live PacketId range
TIM-001 durable TTL survives reboot without boot-clock confusion
DED-003 destination reboot + duplicate retry does not create second chat message
```

Exact queue capacities and watchdog thresholds are measurement-dependent and are frozen after baseline/resource profiling, not guessed here.
