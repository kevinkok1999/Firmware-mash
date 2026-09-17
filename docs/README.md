# Firmware-mash Documentation Index

This directory is the design/evidence source of truth for Firmware-mash implementation and release promotion.

## Start here

- `CODING_START_READY.md` — current coding-start decision, phase status and next build order.
- `THREE_PHASE_IMPLEMENTATION_PLAN.md` — mandatory foundation -> communication engine -> product/release sequence.
- `FINAL_CONTROLLER_REVIEW_2026-09-17.md` — latest three-phase controller verdict and real blockers.
- `CODING_TRIGGER_CONTRACT.md` — authoritative coding-start/promote contract.
- `ARCHITECTURE.md` — system architecture and stable invariants.
- `FEATURE_MANIFEST.md` — complete STABLE/BETA/LAB scope.
- `IMPLEMENTATION_BLUEPRINT.md` — exact module/dependency order.
- `ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` — coding/self-repair/controller execution rules.
- `PACKET_DELIVERY_CONTRACT.md` — origin-qualified logical identity, ACK, delayed-delivery and exactly-once semantics.
- `STORE_CARRY_CUSTODY_CONTRACT.md` — optional durable relay custody/store-carry-forward semantics.
- `MESSAGE_STORE_DESIGN.md` — durable sender/custody storage contract.
- `PHASE1_STORAGE_DURABILITY_DESIGN.md` — implemented snapshot/journal/recovery foundation.
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

## Current coding state

Coding is active on `develop` under `CODING_START_READY.md`.

- Phase 1 foundation/storage implementation exists but promotion still needs the current target-build evidence.
- GitHub-hosted runner/scheduling failure is isolated in issue #19; it is not counted as a firmware compile/test failure.
- Host-testable Phase-2 primitives may be developed while that external runner gate is open, but they are not promoted into the target build around a red Phase-1 gate.
- Phase 3 remains fail-closed until product, release and physical T-Deck evidence exist.

## One-click workflow scaffolds

- `.github/workflows/coding-readiness.yml` — manual fail-closed readiness check.
- `.github/workflows/phase1-foundation.yml` — foundation/storage host + T-Deck target evidence.
- `.github/workflows/runner-probe.yml` — minimal runner/scheduling isolation probe.
- `.github/workflows/release-package.yml` — future manual build/package/flasher workflow.

Neither workflow bypasses evidence. Release packaging eventually runs only after all lower phase gates genuinely pass.

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
- `BASELINE_APPROVED`
- `BASELINE_EVIDENCE_2026-09-17.md`
- `BASELINE_PROVENANCE_2026-09-17.md`
- `BASELINE_TEMPLATE.md`
- `PROVENANCE_TEMPLATE.md`
- `LICENSING.md`

## Reviews / risks

- `FINAL_CONTROLLER_REVIEW_2026-09-17.md` — current final architecture verdict.
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
- `test/run_host_tests.sh` — Phase-1 host storage/fault integration entry point.
- `test/run_phase2_tests.sh` — Phase-2 communication-engine entry point; intentionally red until all suites exist.
- `test/run_phase3_tests.sh` — Phase-3 product/hardware evidence entry point; intentionally red until release evidence exists.

## ADRs

See `adr/README.md`. Recent decisions:

- ADR 0006 — one EnergyManager / RF-harvest isolation;
- ADR 0007 — one IP transport + gateway federation below HybridRouter;
- ADR 0008 — bounded durable custody; acceptance is not end-to-end delivery;
- ADR 0009 — pinned Bramble foundation with Firmware-mash overlay integration.

## Normative ownership

- architecture -> `ARCHITECTURE.md` / ADRs;
- coding start -> `CODING_START_READY.md` / `CODING_TRIGGER_CONTRACT.md`;
- three-phase promotion -> `THREE_PHASE_IMPLEMENTATION_PLAN.md` / `scripts/phase-gate.sh`;
- packet/delivery -> `PACKET_DELIVERY_CONTRACT.md`;
- custody -> `STORE_CARRY_CUSTODY_CONTRACT.md`;
- storage -> `STANDALONE_TDECK_REQUIREMENTS.md` / `MESSAGE_STORE_DESIGN.md` / `PHASE1_STORAGE_DURABILITY_DESIGN.md`;
- energy -> `ENERGY_MANAGEMENT_CONTRACT.md`;
- IP/gateway -> `IP_GATEWAY_FEDERATION_CONTRACT.md` / `GATEWAY_SECURITY_REQUIREMENTS.md`;
- build/test -> `BUILD_CONFIG_MATRIX.md` / `TEST_TRACEABILITY.md`;
- implementation order -> `IMPLEMENTATION_BLUEPRINT.md` / `ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` / `ROADMAP.md`;
- final package -> `USER_RELEASE_CONTRACT.md` / `FLASHER_RELEASE_CONTRACT.md` / `scripts/release-package.sh`.

If normative files conflict, resolve the owning contract/ADR before implementation continues.
