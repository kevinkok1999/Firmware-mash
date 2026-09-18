# ADR 0005 — Standalone Internal Storage, No microSD Dependency

**Status:** Accepted for pre-build architecture

## Context

The target T-Deck Plus includes onboard flash/PSRAM and an optional TF/microSD slot. A removable card is not reliable enough to be a prerequisite for identity, messaging, routing or recovery. The audited Bramble mailbox is primarily a bounded RAM structure, so durable store-and-forward across reboot/power loss needs an explicit design decision.

## Decision

The stable Firmware-mash core must operate with no microSD card installed.

Mandatory durable state lives in onboard internal flash. PSRAM is volatile workspace only. microSD is optional bulk storage for non-critical features such as map packs, manual logs or exports.

Durable state is split by churn/failure domain:

- identity/security/configuration in NVS or an equivalently appropriate small durable store;
- pending/store-forward user messages in a separate bounded internal-flash MessageStore;
- route/neighbor/RF metrics primarily in RAM/PSRAM and rebuilt after boot.

The MessageStore implementation must be selected only after the pinned baseline image/partition budget is measured. It must provide bounded capacity, record integrity, power-loss recovery and a deterministic full-store policy.

## Partition rule

Exact partition sizes are not frozen in this ADR. The future 16 MB layout must reserve enough room for the selected application/recovery strategy, durable configuration/security state and a dedicated message-store area. If A/B OTA is retained, both app slots and rollback margin are first-class constraints.

## Consequences

- Core messaging cannot fail because no microSD is present.
- A reboot may lose transient route optimization but not device identity or committed pending messages.
- Flash endurance is protected by keeping high-churn metrics out of persistent storage.
- Storage corruption is isolated: message-store recovery must not require wiping identity/configuration.
- Every stable hardware release must pass a no-microSD acceptance test.
