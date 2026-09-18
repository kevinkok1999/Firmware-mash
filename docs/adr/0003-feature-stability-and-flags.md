# ADR 0003 — Feature Stability and Compile-Time Isolation

**Status:** Accepted for pre-build architecture

## Decision

Features are explicitly classified as STABLE, BETA or LAB and independently switchable where technically practical.

Initial intended policy:

- STABLE candidate: LoRa backbone, validated routing/reliability/store-forward.
- BETA candidate: hardware-validated ESP-NOW local lane and hybrid forwarding.
- LAB: NAN, TDMA/regional routing experiments, RF-assist and external backscatter.

A LoRa-only configuration is a permanent regression target.

## Promotion rule

A feature cannot be promoted because it compiles. Promotion requires relevant unit/simulator tests and real hardware evidence for hardware-facing claims, plus bounded resource behavior and a documented failure path.

## Consequences

Experimental radio work can progress without making the core dependent on it. Removing or disabling a failed experiment must not require redesigning the routing/messaging layer.
