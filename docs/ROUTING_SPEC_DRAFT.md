# Routing Specification Draft

This document defines routing behavior before implementation. It is deliberately transport-neutral and simulator-first.

## Goals

- reactive discovery rather than constant global control traffic;
- retain a small number of useful alternate paths;
- fail over to cached alternatives before broad rediscovery;
- prefer independent backups over near-duplicates;
- keep control traffic, memory and retries bounded;
- allow LoRa and optional local transports under one routing authority.

## Non-goals for v1

- internet-style shortest-path convergence at arbitrary scale;
- unlimited path storage;
- always sending a message on multiple routes;
- opaque ML-based routing decisions;
- nationwide scale claims before simulation/field evidence.

## Route discovery

Initial implementation should remain close to the selected foundation's proven reactive routing model. A multipath extension may retain more than one valid reply/candidate from a bounded discovery window when those candidates satisfy loop/freshness rules.

Discovery uses explicit bounds:

- hop/TTL limit;
- duplicate suppression;
- discovery timeout;
- maximum responses/candidates retained;
- retry/backoff limit;
- airtime/control-traffic budget.

## Candidate acceptance

A candidate is rejected if it is stale, loop-forming, violates protocol validity, exceeds resource bounds or provides no useful route. Candidate replacement must be deterministic when the fixed route set is full.

## Route error scope

A route/link failure may invalidate a pending path only when the reported failing node/link/next hop intersects that path's forwarding state. A RERR for destination D is not sufficient by itself to cancel every direct or alternate delivery to D.

This rule requires unit/simulator coverage.

## Primary and alternate selection

For destination D:

1. Prefer direct reachable neighbor where appropriate and valid.
2. Evaluate valid cached candidates.
3. Select a primary using the versioned RouteScoreEngine.
4. Select backup(s) using both quality and PathDiversityEvaluator.
5. Apply hysteresis/stability rules so small metric noise does not cause continuous route flapping.

## Normal send state machine

```text
MESSAGE READY
    |
    v
DIRECT/CACHED PRIMARY AVAILABLE? -- no --> DISCOVERY
    | yes                               |
    v                                   v
SEND PRIMARY                       ROUTE FOUND?
    |                              /          \
    v                            yes           no
WAIT END-TO-END EVIDENCE          |             |
   /            \                 v             v
success         fail          SEND             STORE/WAIT_ROUTE
  |              |
DELIVERED    DEGRADE FAILED PATH
                 |
                 v
          CACHED ALTERNATE?
             /       \
           yes        no
            |          |
            v          v
      SEND ALTERNATE   DISCOVERY
```

## Failure evidence

A path can be degraded by bounded evidence such as:

- transport/link TX failures;
- expected forwarding evidence absent;
- end-to-end ACK timeout;
- authenticated/path-relevant route error;
- neighbor expiry;
- repeated high ETX/loss.

One noisy sample should not necessarily invalidate an otherwise stable path. Failure confidence and hysteresis are required.

## Failover rule

Cached alternate path is attempted before network-wide/broad discovery when it is still fresh and valid. If no valid alternate exists, start bounded discovery. If discovery fails, hand the encrypted logical message to MessageStore in `WAITING_ROUTE`/equivalent state.

## Path diversity

Two candidates with the same first hop are weak redundancy even if their later hops differ. Initial diversity evaluation includes:

- first-hop independence;
- shared intermediate-node ratio;
- shared-link ratio;
- shared constrained-interface/bottleneck signal where known.

The exact scoring formula remains configurable and simulator-tuned.

## Congestion and airtime

RouteScoreEngine consumes airtime/congestion metrics. A lower-hop route is not automatically better if it is congested or repeatedly retransmitting. Route discovery itself is also subject to airtime budgets.

## Multipath delivery

NORMAL delivery sends one logical packet on one route at a time. Failover reuses the same PacketId.

CRITICAL delivery may request an additional sufficiently independent path only when ReliabilityManager and AirtimeManager both permit it. Duplicate arrival is suppressed at the destination.

## Store-carry-forward trigger

When all current candidates fail and bounded discovery produces no valid route, the message becomes queued rather than silently discarded (subject to policy/TTL/storage capacity).

Queued delivery is **event driven**, not distance driven. There is no fixed retry distance such as 200 m, 500 m or 1 km.

A queued message should become immediately eligible for retry when the networking layer obtains credible evidence that delivery may now be possible, for example:

- a previously unreachable destination is heard directly again;
- a new LoRa neighbor is discovered;
- an existing neighbor/link transitions from unusable to usable;
- ESP-NOW or ESP-NOW Long Range discovers/re-establishes a viable peer link;
- HybridRouter installs a new valid path to the destination;
- a cached path becomes usable again after temporary degradation.

The first retry after such a route/link-up event should be scheduled with minimal latency, subject only to the central airtime/regulatory gate, radio availability and a very small anti-storm/jitter guard where required. Periodic background retries may still exist as a safety net, but they are not the primary recovery mechanism.

RSSI alone is not treated as a magic numeric threshold. A usable-link decision may combine valid frame reception, neighbor freshness, RSSI/SNR or ESP-NOW link evidence, path validity and radio state. This avoids waiting for an arbitrary signal-strength number when a real packet path is already available.

Example:

```text
A sends to B at 5 km -> no usable route -> WAITING_ROUTE
A moves closer
B becomes directly reachable at whatever distance the environment allows
LINK/NEIGHBOR/PATH UP event
-> queued packet becomes immediately eligible
-> send
-> end-to-end ACK
-> DELIVERED
```

The user does not press Send again and does not select the recovered transport manually.

## Transport changes inside a path

A logical path may conceptually contain segments using different transports, but the logical packet identity and end-to-end reliability state remain unchanged. The user never selects transport manually.

## Route ageing

Every installed path has freshness/last-success/failure information. Ageing must distinguish stable fixed backbone paths from stale or mobile observations without making permanent routes immortal. All timer arithmetic uses wrap-safe monotonic helpers.

## Mandatory tests before promotion

- two independent routes, primary relay removed;
- shared-first-hop "fake backup" is not treated as high diversity;
- RERR for unrelated path does not cancel direct/pending route;
- route discovery timeout recovers rather than parking forever;
- route-flapping/noisy metrics do not oscillate continuously;
- duplicate multipath arrival is delivered to application once;
- failure with no path enters explicit store/no-route state;
- queued message retries immediately when a valid direct link reappears;
- queued message retries immediately when a new indirect route is installed;
- recovery trigger is not tied to a fixed distance or hard-coded RSSI threshold;
- high-loss and congested cases remain bounded in transmissions/control airtime.
