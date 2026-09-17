# Firmware-mash Documentation Index

This directory is the design/evidence source of truth before production firmware is admitted into the repository.

## Start here

- `FINAL_CONTROLLER_REVIEW_2026-09-17.md` — latest three-phase pre-code verdict and real blockers.
- `CODING_TRIGGER_CONTRACT.md` — authoritative future coding start contract.
- `ARCHITECTURE.md` — system architecture and stable invariants.
- `FEATURE_MANIFEST.md` — complete STABLE/BETA/LAB scope.
- `IMPLEMENTATION_BLUEPRINT.md` — exact module/dependency order.
- `ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` — coding/self-repair/controller execution rules.
- `PACKET_DELIVERY_CONTRACT.md` — PacketId, ACK, delayed-delivery and exactly-once semantics.
- `STORE_CARRY_CUSTODY_CONTRACT.md` — optional durable relay custody/store-carry-forward semantics.
- `MESSAGE_STORE_DESIGN.md` — durable sender/custody storage contract.
- `ENERGY_MANAGEMENT_CONTRACT.md` — EnergyManager and RF-harvest isolation.
- `IP_GATEWAY_FEDERATION_CONTRACT.md` — Wi-Fi/cellular IP backhaul and gateway federation.
- `GATEWAY_SECURITY_REQUIREMENTS.md` — federation security/privacy/resource gates.
- `UI_UX_CONTRACT.md` / `UI_IMPLEMENTATION_MAP.md` — smartphone-like UI contract/bindings.
- `WIRE_PROTOCOL_CONTRACT.md` — transport-neutral wire/versioning rules.
- `BUILD_CONFIG_MATRIX.md` — canonical stable/beta/lab configurations.
- `TEST_TRACEABILITY.md` — requirements to stable test IDs.
- `PREBUILD_CHECKLIST.md` — hard pre-source checklist.
- `FLASHER_RELEASE_CONTRACT.md` — release manifest, hashes, flasher and recovery contract.
- `ROADMAP.md` — staged roadmap through final one-flash package.

## One-click workflow scaffolds

- `.github/workflows/coding-readiness.yml` — manual fail-closed readiness check before a coding agent begins implementation.
- `.github/workflows/release-package.yml` — future manual build/package/flasher workflow; intentionally fails until production release script/evidence exist.

Neither workflow bypasses evidence. The coding workflow validates readiness; a coding agent still performs source implementation. The release workflow eventually packages only a genuinely built/validated release.

## Architecture / API

- `API_CONTRACTS.md`
- `ROUTING_SPEC_DRAFT.md`
- `REPOSITORY_LAYOUT.md`
- `IP_GATEWAY_IMPLEMENTATION_CHECKLIST.md`
- `RESOURCE_BUDGET.md`
- `STANDALONE_TDECK_REQUIREMENTS.md`

## Foundation / reproducibility

- `FOUNDATION_BRAMBLE_NOTES.md`
- `BUILD_PHASE_1_PLAN.md`
- `BASELINE_TEMPLATE.md`
- `PROVENANCE_TEMPLATE.md`
- `LICENSING.md`

## Reviews / risks

- `FINAL_CONTROLLER_REVIEW_2026-09-17.md` — current final verdict.
- `CONTROLLER_REVIEW_2026-09-17.md` — earlier controller pass, retained as history.
- `ENERGY_INTEGRATION_REVIEW_2026-09-17.md`
- `IP_GATEWAY_REVIEW_2026-09-17.md`
- `PREBUILD_REVIEW_2026-09-17.md`
- `REAL_WORLD_COMPETITOR_REVIEW_2026-09-17.md`
- `UPSTREAM_AUDIT_2026-09-17.md`
- `RISK_REGISTER.md`

## Testing

- `TESTING.md` — evidence policy.
- `TEST_TRACEABILITY.md` — STO/DEL/RTE/ESP/IP/GW/CELL/CUS/ENG/UX/CFG/TRIG/RELSE mapping.

## ADRs

See `adr/README.md`. Recent decisions:

- ADR 0006 — one EnergyManager / RF-harvest isolation;
- ADR 0007 — one IP transport + gateway federation below HybridRouter;
- ADR 0008 — bounded durable custody; acceptance is not end-to-end delivery.

## Normative ownership

- architecture -> `ARCHITECTURE.md` / ADRs;
- coding start -> `CODING_TRIGGER_CONTRACT.md`;
- packet/delivery -> `PACKET_DELIVERY_CONTRACT.md`;
- custody -> `STORE_CARRY_CUSTODY_CONTRACT.md`;
- storage -> `STANDALONE_TDECK_REQUIREMENTS.md` / `MESSAGE_STORE_DESIGN.md`;
- energy -> `ENERGY_MANAGEMENT_CONTRACT.md`;
- IP/gateway -> `IP_GATEWAY_FEDERATION_CONTRACT.md` / `GATEWAY_SECURITY_REQUIREMENTS.md`;
- build/test -> `BUILD_CONFIG_MATRIX.md` / `TEST_TRACEABILITY.md`;
- implementation order -> `IMPLEMENTATION_BLUEPRINT.md` / `ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` / `ROADMAP.md`;
- final package -> `USER_RELEASE_CONTRACT.md` / `FLASHER_RELEASE_CONTRACT.md`.

If normative files conflict, resolve the owning contract/ADR before implementation continues.