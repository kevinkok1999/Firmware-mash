# Contributing

## Branch model

- `main`: release/stable history only.
- `develop`: integration branch for reviewed preparatory and implementation work.
- `feature/*`: bounded production features.
- `experiment/*`: research that must not silently become stable.
- `fix/*`: scoped fixes.

Prefer small pull requests with one architectural purpose.

## Before writing code

Read `docs/ARCHITECTURE.md`, `docs/PREBUILD_CHECKLIST.md`, `docs/LICENSING.md` and the relevant ADRs. Do not introduce a second routing authority or bypass the airtime/security/reliability boundaries.

## Evidence in PRs

Every technical PR should state:

- what changed;
- feature status: STABLE/BETA/LAB candidate;
- evidence: PROVEN/SIMULATED/EXPERIMENTAL;
- tests executed and actual results;
- RAM/flash impact where relevant;
- LoRa airtime/control-traffic impact where relevant;
- upstream provenance for reused/adapted code;
- failure/rollback path.

## Commit style

Use concise conventional-style subjects where practical, for example:

- `feat(routing): add bounded route set`
- `test(sim): cover primary relay failure`
- `fix(reliability): cap alternate-path retries`
- `docs(adr): record ESP-NOW adapter decision`

## Architecture changes

Changes to wire format, routing semantics, security boundaries, licensing strategy, feature stability or task ownership require an ADR in `docs/adr/`.

## Safety rules

Do not merge code that silently increases transmit power, bypasses regional airtime controls, creates unbounded retries/flooding, weakens encryption, or labels compile-only behavior as hardware validated.
