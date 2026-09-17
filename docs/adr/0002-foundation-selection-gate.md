# ADR 0002 — Foundation Selection Gate

**Status:** Accepted for pre-build process

## Context

Bramble is currently the leading foundation candidate because it already targets T-Deck Plus, uses a modular ESP-IDF component layout, provides routing/reliability/security/store-forward pieces and includes host/simulator infrastructure. That does not justify blindly importing it.

## Decision

No production source is imported until an exact upstream commit/tag is pinned and a reproducible baseline is demonstrated. The gate requires:

- T-Deck Plus target confirmed in source;
- baseline build reproduced;
- baseline tests/simulator run;
- RAM/flash recorded;
- LoRa-only behavior recorded;
- license captured at the pinned commit;
- upstream limitations documented.

If another foundation clearly outperforms Bramble on target support, testability, maintainability, security and licensing during the audit, this ADR must be superseded before import.

## Consequences

The project optimizes for reproducibility rather than rushing to a large source import. Upstream updates are intentional and reviewable rather than floating dependencies.
