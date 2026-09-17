## What changed

Describe one bounded change.

## Status

- Feature tier: STABLE / BETA / LAB candidate
- Evidence: PROVEN / SIMULATED / EXPERIMENTAL

## Tests actually executed

List commands/scenarios and actual results. Do not list planned tests as completed.

## Resource impact

- RAM:
- Flash:
- Queue/pool impact:
- LoRa airtime/control traffic:
- Power impact if measured:

## Architecture checks

- [ ] HybridRouter remains the only routing authority.
- [ ] LoRa-only operation remains available.
- [ ] New queues/tables/retries are bounded.
- [ ] Failure/rollback behavior is documented.
- [ ] No transport callback directly owns routing state.

## Security and licensing

- [ ] No new custom cryptography.
- [ ] End-to-end security boundary is preserved.
- [ ] Upstream source provenance is documented if code was reused/adapted.
- [ ] License obligations were reviewed.

## Failure test

Describe the failure scenario most likely to break this change and how it was tested.
