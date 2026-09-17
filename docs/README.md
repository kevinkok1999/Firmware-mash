# Firmware-mash Documentation Index

This directory is the design and evidence source of truth before production firmware is admitted into the repository.

## Start here

- `ARCHITECTURE.md` — system architecture and stable invariants.
- `FEATURE_MANIFEST.md` — complete STABLE/BETA/LAB capability checklist.
- `CODING_TRIGGER_CONTRACT.md` — authoritative future start condition/instruction for production coding.
- `IMPLEMENTATION_BLUEPRINT.md` — exact module and coding order after baseline approval.
- `ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` — execution/self-repair contract for the coding agent.
- `PACKET_DELIVERY_CONTRACT.md` — PacketId, ACK, delayed-delivery and exactly-once presentation semantics.
- `STORE_CARRY_CUSTODY_CONTRACT.md` — optional durable relay custody/store-carry-forward semantics.
- `ENERGY_MANAGEMENT_CONTRACT.md` — EnergyManager, power-state, optional harvester-provider and RF isolation contract.
- `IP_GATEWAY_FEDERATION_CONTRACT.md` — Wi-Fi/cellular IP backhaul, gateway discovery/federation and same-chat continuity.
- `GATEWAY_SECURITY_REQUIREMENTS.md` — hard security/privacy/resource gates before public federation.
- `UI_UX_CONTRACT.md` / `UI_IMPLEMENTATION_MAP.md` — smartphone-like T-Deck UX and bindings.
- `WIRE_PROTOCOL_CONTRACT.md` — transport-neutral wire layering/versioning/compatibility.
- `STANDALONE_TDECK_REQUIREMENTS.md` — no-microSD, internal-storage and recovery requirements.
- `MESSAGE_STORE_DESIGN.md` — durable internal pending/custody storage contract.
- `BUILD_CONFIG_MATRIX.md` — baseline/stable/beta/lab build configurations.
- `TEST_TRACEABILITY.md` — requirement-to-test mapping and stable test IDs.
- `RESOURCE_BUDGET.md` — flash/RAM/PSRAM/persistence/OTA budget rules.
- `USER_RELEASE_CONTRACT.md` — normal-user one-flash requirement.
- `FLASHER_RELEASE_CONTRACT.md` — reproducible release package, manifest, hashes, flasher and recovery contract.
- `ROADMAP.md` — staged implementation order.
- `PREBUILD_CHECKLIST.md` — hard gate before production source.

## Architecture and APIs

- `API_CONTRACTS.md` — internal interfaces and ownership rules.
- `ROUTING_SPEC_DRAFT.md` — transport-neutral routing/failover behavior.
- `REPOSITORY_LAYOUT.md` — source placement after baseline approval.
- `IP_GATEWAY_IMPLEMENTATION_CHECKLIST.md` — concrete IP/gateway implementation checklist.

## Foundation and reproducibility

- `FOUNDATION_BRAMBLE_NOTES.md` — selected foundation candidate/pin evidence.
- `BUILD_PHASE_1_PLAN.md` — baseline-reproduction runbook.
- `BASELINE_TEMPLATE.md` — fields required before source directories unlock.
- `PROVENANCE_TEMPLATE.md` — upstream code/license provenance record.
- `LICENSING.md` — project licensing strategy.

## Research and review evidence

- `UPSTREAM_AUDIT_2026-09-17.md` — upstream technology snapshot.
- `PREBUILD_REVIEW_2026-09-17.md` — adversarial pre-build review.
- `REAL_WORLD_COMPETITOR_REVIEW_2026-09-17.md` — standalone usability review.
- `CONTROLLER_REVIEW_2026-09-17.md` — earlier controller pass; superseded for final go/no-go by the final pre-code controller report when present.
- `ENERGY_INTEGRATION_REVIEW_2026-09-17.md` — energy/RF integration review.
- `IP_GATEWAY_REVIEW_2026-09-17.md` — IP/gateway/federation review.
- `RISK_REGISTER.md` — technical risk register and release gates.

## Testing

- `TESTING.md` — simulator, host and hardware evidence policy.
- `TEST_TRACEABILITY.md` — stable IDs including STO/DEL/RTE/ESP/IP/GW/CELL/CUS/ENG/UX/CFG/TRIG/RELSE tests.

## Architecture Decision Records

See `adr/README.md` and `adr/0001-*.md` onward. Key recent decisions:

- ADR 0006 — one EnergyManager / RF-harvest isolation;
- ADR 0007 — one IP backhaul transport + gateway federation below HybridRouter;
- ADR 0008 — optional bounded durable custody; acceptance is not end-to-end delivery.

## Normative ownership

- architecture invariants -> `ARCHITECTURE.md` / ADRs;
- full feature scope -> `FEATURE_MANIFEST.md`;
- implementation trigger -> `CODING_TRIGGER_CONTRACT.md`;
- packet/delivery truth -> `PACKET_DELIVERY_CONTRACT.md`;
- custody/store-carry -> `STORE_CARRY_CUSTODY_CONTRACT.md`;
- storage -> `STANDALONE_TDECK_REQUIREMENTS.md` / `MESSAGE_STORE_DESIGN.md`;
- energy -> `ENERGY_MANAGEMENT_CONTRACT.md`;
- IP/gateway -> `IP_GATEWAY_FEDERATION_CONTRACT.md` / `GATEWAY_SECURITY_REQUIREMENTS.md`;
- APIs -> `API_CONTRACTS.md`;
- routing -> `ROUTING_SPEC_DRAFT.md`;
- build configurations -> `BUILD_CONFIG_MATRIX.md`;
- UI -> `UI_UX_CONTRACT.md` / `UI_IMPLEMENTATION_MAP.md`;
- test evidence -> `TESTING.md` / `TEST_TRACEABILITY.md`;
- implementation sequence -> `IMPLEMENTATION_BLUEPRINT.md` / `ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` / `ROADMAP.md`;
- final package/flasher -> `USER_RELEASE_CONTRACT.md` / `FLASHER_RELEASE_CONTRACT.md`.

When two normative documents disagree, stop and resolve the owning contract/ADR before production code is merged.
