# Flasher and Release Engineering Contract

## Purpose

Define the path from one validated Firmware-mash commit to one reproducible T-Deck Plus release package that can be flashed without manual binary assembly.

This contract does not claim that a release is safe merely because it compiles. Hardware-facing STABLE claims require the mapped hardware evidence.

## Release artifact goal

A release pipeline must produce one coherent package for one declared board/region/tier containing at minimum:

```text
firmware merged image or equivalent supported flash set
bootloader/partition/app artifacts where required
release-manifest.json
SHA256SUMS
release notes / evidence tier
recovery instructions and recovery artifact/path
one-click/web-flasher descriptor where supported
```

Exact flash offsets and partition addresses come from the measured pinned build and generated partition metadata. They are never guessed in documentation.

## Reproducibility

A release is identified by:

- Firmware-mash git SHA;
- pinned foundation SHA;
- ESP-IDF/toolchain version;
- target board;
- region profile;
- partition-table revision/hash;
- protocol/wire version;
- enabled feature tier;
- evidence tier.

A clean checkout using the documented toolchain must reproduce the same logical release inputs. Where deterministic byte-for-byte output is not achievable because of tool metadata, the difference must be documented rather than hidden.

## Release tiers

### STABLE

Contains only features with required automated plus hardware evidence. Must preserve:

- stock T-Deck Plus boot/use without microSD;
- LoRa backbone;
- durable MessageStore;
- validated reliability/multipath features;
- EnergyManager without RF-harvest dependency;
- simple local messaging UI;
- recovery path.

Optional IP/ESP-NOW/custody features may be included in STABLE only when their promotion gates have passed.

### BETA

May enable hardware-tested but not yet STABLE hybrid features. The manifest must state BETA clearly.

### LAB

May expose experimental providers/transports. A LAB package cannot be labelled as the normal stable flasher image.

## Required pipeline gates

Before producing a distributable STABLE artifact:

1. verify branch/tag/release source is approved;
2. verify baseline/provenance/toolchain pins;
3. compile `CFG-LORA-STABLE`;
4. compile every enabled optional feature configuration;
5. run mapped host/unit/simulator tests;
6. verify required real-hardware evidence references for every STABLE hardware-facing feature;
7. verify flash/RAM/PSRAM/resource budgets;
8. verify partition table leaves required recovery/growth margins;
9. verify no forbidden secret/private material is packaged;
10. generate release manifest and hashes;
11. generate supported flash descriptor;
12. verify recovery procedure against the exact release layout.

Missing evidence rejects STABLE publication. It does not get converted into an optimistic warning.

## Release manifest

`release-manifest.json` conceptually includes:

```text
product
board
git_sha
foundation_sha
version
build_timestamp
toolchain
region
feature_tier
evidence_tier
protocol_version
partition_revision
artifacts[] {name, sha256, size, flash_offset_if_applicable}
required_accessories[]
known_limitations[]
recovery_artifact
```

Fields become exact only from real build output.

## One-click flashing contract

The intended user experience is:

```text
open supported flasher
-> select/confirm T-Deck Plus + region
-> connect device
-> Flash
-> integrity/flash result shown
-> reboot
-> first-run onboarding
```

The user does not manually concatenate binaries or calculate offsets.

The flasher must consume generated release metadata rather than hard-coded stale offsets where possible.

## Recovery

Before STABLE release, recovery must cover at least:

- failed/incomplete flash;
- bad application image;
- configuration reset without unnecessary identity loss where architecture permits;
- return to a known-good image through the documented USB/bootloader path.

If A/B OTA is later enabled, image verification and rollback become separate mandatory release gates.

## Integrity and provenance

- SHA-256 hashes are generated for distributed binary artifacts.
- Release notes identify the exact source commit and evidence tier.
- Third-party notices/licenses required by reused code are included.
- No custom cryptographic primitive is introduced for the flasher or release system.

## CI/CD rule

A release workflow may be prepared before production source exists, but must fail closed when required source, baseline approval or evidence is absent. It must never fabricate a successful release artifact from placeholder data.

## Acceptance tests

```text
RELSE-001 clean-checkout release package creation
RELSE-003 manifest hashes match distributed artifacts
RELSE-004 flasher descriptor matches generated partition/build metadata
RELSE-005 STABLE release rejected when hardware evidence is missing
RELSE-006 recovery path verified against exact release package
RELSE-007 no manual binary assembly required by normal user
RELSE-008 artifact maps uniquely to commit/board/region/tier
```
