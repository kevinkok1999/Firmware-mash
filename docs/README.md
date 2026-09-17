# Firmware-mash Documentation Index

This directory is the design and evidence source of truth before production firmware is admitted into the repository.

## Start here

- `ARCHITECTURE.md` — system architecture and stable invariants.
- `FEATURE_MANIFEST.md` — complete STABLE/BETA/LAB capability checklist.
- `IMPLEMENTATION_BLUEPRINT.md` — exact module and coding order after baseline approval.
- `ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` — master execution contract for a coding agent.
- `ENERGY_MANAGEMENT_CONTRACT.md` — EnergyManager, power-state, optional harvester-provider and RF isolation contract.
- `IP_GATEWAY_FEDERATION_CONTRACT.md` — Wi-Fi/cellular IP backhaul, gateway discovery/federation and same-chat transport-continuity contract.
- `UI_UX_CONTRACT.md` — smartphone-like T-Deck user experience.
- `UI_IMPLEMENTATION_MAP.md` — exact screens, navigation, UI state mapping and event bindings.
- `PACKET_DELIVERY_CONTRACT.md` — PacketId, ACK, delayed-delivery and exactly-once presentation semantics.
- `WIRE_PROTOCOL_CONTRACT.md` — transport-neutral wire layering, versioning and compatibility rules.
- `STANDALONE_TDECK_REQUIREMENTS.md` — no-microSD, internal-storage and recovery requirements.
- `MESSAGE_STORE_DESIGN.md` — durable internal queued-message storage contract.
- `BUILD_CONFIG_MATRIX.md` — canonical baseline, LoRa-stable, hybrid-beta, IP/gateway-beta, energy-lab and lab configurations.
- `TEST_TRACEABILITY.md` — requirement-to-test mapping and stable test IDs.
- `RESOURCE_BUDGET.md` — flash/RAM/PSRAM/persistence/OTA budget rules.
- `USER_RELEASE_CONTRACT.md` — one-flash stable-user requirement; users are not the engineering test harness.
- `ROADMAP.md` — staged implementation order.
- `PREBUILD_CHECKLIST.md` — hard gate before production source.

## Architecture and APIs

- `API_CONTRACTS.md` — internal interfaces and ownership rules.
- `ROUTING_SPEC_DRAFT.md` — transport-neutral routing and failover behavior.
- `REPOSITORY_LAYOUT.md` — source placement rules after the baseline gate unlocks production code.
- `ENERGY_MANAGEMENT_CONTRACT.md` — energy state, provider boundary and integration semantics.
- `IP_GATEWAY_FEDERATION_CONTRACT.md` — IP bearer, gateway, discovery, federation, security and fallback semantics.

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
- `CONTROLLER_REVIEW_2026-09-17.md` — final code-readiness contradiction/gap review.
- `ENERGY_INTEGRATION_REVIEW_2026-09-17.md` — embedded-power, RF, routing and release review for EnergyManager/Ambient RF Energy Assist.
- `IP_GATEWAY_REVIEW_2026-09-17.md` — embedded networking, DTN, gateway/security and product review for Internet-assisted federation.
- `RISK_REGISTER.md` — technical risk register and release gates.

## Testing

- `TESTING.md` — simulator, host and hardware evidence policy.
- `TEST_TRACEABILITY.md` — stable IDs and requirement-to-evidence mapping, including ENG/EHW and IP/GW/CELL tests.

## Architecture Decision Records

See `adr/README.md` and `adr/0001-*.md` onward. ADRs record decisions that should not be changed casually during implementation. ADR 0006 defines EnergyManager ownership and RF-harvesting isolation. ADR 0007 defines one IP backhaul transport plus gateway federation under HybridRouter.

## Documentation rule

Do not duplicate the same normative rule across multiple documents unless one file is explicitly an index or summary. Normative ownership is:

- architecture invariants -> `ARCHITECTURE.md` / ADRs;
- complete feature scope -> `FEATURE_MANIFEST.md`;
- API semantics -> `API_CONTRACTS.md`;
- routing semantics -> `ROUTING_SPEC_DRAFT.md`;
- energy-management/provider semantics -> `ENERGY_MANAGEMENT_CONTRACT.md`;
- IP/gateway/federation semantics -> `IP_GATEWAY_FEDERATION_CONTRACT.md`;
- packet/delivery semantics -> `PACKET_DELIVERY_CONTRACT.md`;
- wire/layering semantics -> `WIRE_PROTOCOL_CONTRACT.md`;
- storage/no-SD requirements -> `STANDALONE_TDECK_REQUIREMENTS.md`;
- MessageStore internals -> `MESSAGE_STORE_DESIGN.md`;
- build/feature configurations -> `BUILD_CONFIG_MATRIX.md`;
- UI behavior -> `UI_UX_CONTRACT.md` / `UI_IMPLEMENTATION_MAP.md`;
- resource limits -> `RESOURCE_BUDGET.md`;
- user-facing stable release requirement -> `USER_RELEASE_CONTRACT.md`;
- test evidence -> `TESTING.md` / `TEST_TRACEABILITY.md`;
- implementation sequence -> `IMPLEMENTATION_BLUEPRINT.md` / `ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` / `ROADMAP.md`.

When two normative documents disagree, stop and resolve the conflict with an ADR before production code is merged.
