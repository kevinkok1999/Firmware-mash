# Firmware-mash Documentation Index

This directory is the design and evidence source of truth before production firmware is admitted into the repository.

## Start here

- `ARCHITECTURE.md` — system architecture and stable invariants.
- `API_CONTRACTS.md` — internal interfaces and ownership rules.
- `ROUTING_SPEC_DRAFT.md` — transport-neutral routing and failover behavior.
- `STANDALONE_TDECK_REQUIREMENTS.md` — no-microSD, internal-storage and recovery requirements.
- `ROADMAP.md` — staged implementation order.
- `PREBUILD_CHECKLIST.md` — hard gate before production source.

## Foundation and reproducibility

- `FOUNDATION_BRAMBLE_NOTES.md` — current foundation candidate details.
- `BUILD_PHASE_1_PLAN.md` — exact baseline-reproduction runbook.
- `BASELINE_TEMPLATE.md` — evidence fields required before source directories unlock.
- `PROVENANCE_TEMPLATE.md` — upstream code and license provenance record.
- `LICENSING.md` — project licensing strategy.

## Research and review evidence

- `UPSTREAM_AUDIT_2026-09-17.md` — upstream technology snapshot.
- `PREBUILD_REVIEW_2026-09-17.md` — adversarial pre-build review.
- `REAL_WORLD_COMPETITOR_REVIEW_2026-09-17.md` — real-world standalone usability review.
- `RISK_REGISTER.md` — technical risk register and release gates.

## Testing

- `TESTING.md` — simulator, host and hardware evidence policy.

## Architecture Decision Records

See `adr/README.md` and `adr/0001-*.md` onward. ADRs record decisions that should not be changed casually during implementation.

## Repository layout

See `REPOSITORY_LAYOUT.md` for which future code belongs in which top-level directory after baseline approval.

## Documentation rule

Do not duplicate the same normative rule across multiple documents unless one file is explicitly an index or summary. Normative ownership is:

- architecture invariants -> `ARCHITECTURE.md` / ADRs;
- API semantics -> `API_CONTRACTS.md`;
- routing semantics -> `ROUTING_SPEC_DRAFT.md`;
- storage/no-SD requirements -> `STANDALONE_TDECK_REQUIREMENTS.md`;
- test evidence -> `TESTING.md`;
- implementation sequence -> `ROADMAP.md` / build runbooks.

When two documents disagree, stop and resolve the conflict with an ADR before production code is merged.
