# Licensing and Upstream Reuse Policy

## Project license

Firmware-mash is planned as an MIT-licensed project. This does not automatically make all upstream code reusable under MIT.

## Current upstream classification

| Project | Current known license | Direct source reuse in MIT core | Policy |
|---|---|---:|---|
| Bramble | MIT | Yes, with notice preservation | Preferred foundation candidate; track copied/modified files and preserve copyright/license notices. |
| MeshCore | MIT | Yes, with notice preservation | Reuse selectively where it improves the design; avoid unnecessary protocol duplication. |
| LoRaMesher | MIT | Yes, with notice preservation | Prefer isolated algorithms/ideas or small compatible components after technical review. |
| Meshtastic firmware | GPL-3.0 | Not silently | Treat primarily as an architecture/reference source unless project licensing is deliberately changed. Clean-room equivalent implementation is preferred for an MIT core. |
| Reticulum | Reticulum License | Review required | Use architecture concepts unless specific source use is separately reviewed against the current license conditions. |
| Pyxis | Verify before any reuse | Not assumed | Architecture/reference only until license and dependencies are re-audited. |
| MeshCom | Verify before any reuse | Not assumed | Architecture/reference only until current license is confirmed. |
| ESP-IDF / Espressif examples | Component-specific/open-source terms | Review per component | Prefer official APIs; preserve any required notices when example code is incorporated. |

## Required provenance record

Before merging any upstream-derived source, record:

- upstream repository;
- exact commit/tag;
- upstream file path;
- license at that commit;
- whether code is copied, modified, or only conceptually reimplemented;
- local destination path;
- required copyright/notice text;
- reviewer decision.

A future `THIRD_PARTY_NOTICES.md` must be generated from that record before the first public binary release.

## Clean-room rule

When a useful design is visible in a license-incompatible project, document the externally observable behavior/protocol idea first, then implement the local equivalent without copying protected source text. Keep the design note and implementation review separate where practical.

## No license laundering

Do not copy code from a GPL/custom-license repository into an MIT file and remove headers. Reformatting or renaming code does not change its license obligations.
