# Update, Migration and Rollback Contract

## Purpose

Prevent a release that flashes successfully but loses identity, strands pending messages, corrupts MessageStore, or cannot safely boot/rollback because on-device data schemas changed.

This contract applies to USB/web flashing and any future OTA path.

## Update classes

Every release declares one of these update classes in its manifest:

```text
NON_DESTRUCTIVE
  application update preserves identity/config/message data partitions

MIGRATING
  application update includes reviewed data/schema migration

RECOVERY
  explicit rescue path; may reset selected state only with clear user warning

FACTORY_RESET
  destructive user-requested erase; never the default update path
```

A normal stable upgrade is `NON_DESTRUCTIVE` or reviewed `MIGRATING`.

## Identity/config preservation

The standard flasher must not erase identity/configuration merely because it writes a new app image.

Before a destructive operation the UI must state exactly what will be lost. `Erase all flash` is an Advanced/Recovery action, not the normal Flash button.

## MessageStore schema/version rule

Persistent records are explicitly versioned.

Firmware must distinguish:

- schema it can read;
- schema it can migrate;
- schema too new/unknown to modify safely.

Unknown newer durable data fails safe. Older firmware after rollback must not blindly rewrite a newer store it cannot understand.

## Migration transaction

A migration cannot be an in-place unjournaled loop that leaves half the store converted after power loss.

The selected backend must provide an equivalent transactional pattern:

```text
inspect old schema
-> verify space/resources
-> begin migration generation
-> create/rewrite records with integrity/version markers
-> checkpoint durable completion
-> switch active schema/index
-> only then reclaim obsolete representation
```

A power cut at any injected point recovers either the last committed old state or a complete/new recoverable state according to the backend design.

Identity/config migration remains isolated from message-store migration.

## Rollback compatibility

Before a release is allowed to use automatic rollback, define whether persistent data remains readable by the rollback image.

Allowed strategies include:

1. backward-compatible record format;
2. old and new reader overlap for one migration window;
3. migration delayed until the new image has passed boot/self-tests and rollback commitment;
4. explicit no-automatic-rollback boundary with a separate recovery/migration plan.

Silent irreversible migration before the new app proves it can boot is forbidden.

## First-boot validation

A new release performs bounded first-boot checks before being considered healthy:

- partition/schema version recognized;
- identity/config readable;
- MessageStore recoverable;
- required LoRa device/board initialization reaches a safe state;
- no persistent boot loop caused by optional provider absence;
- health state can reach UI/recovery surface.

If A/B app rollback is enabled by the measured partition design, mark the new image valid only after the reviewed health milestone. Exact ESP-IDF rollback mechanics come from the pinned SDK.

## Partition table changes

Changing partition layout is a release migration, not a routine app update.

The release pipeline must compare the new layout with the previous supported layout and determine whether preserved partitions keep the same safe ranges/content semantics.

The flasher never guesses offsets. Generated metadata identifies exact build-derived offsets and whether the update is safely non-destructive.

If a partition migration cannot be made safely one-click, that version is not advertised as a normal non-destructive update.

## Board/chip/flash-size preflight

Before writing, the supported flasher verifies every property it can reliably inspect, including at minimum:

- ESP32-S3 target/chip family;
- expected flash size/layout compatibility where readable;
- release manifest board family;
- selected regulatory region/profile confirmation.

A mismatch aborts instead of attempting a best-effort flash to an unknown layout.

Board properties that cannot be auto-detected require explicit user confirmation rather than fabricated detection.

## Release authenticity

SHA-256 proves artifact integrity only relative to a trusted expected hash. Public STABLE distribution therefore also needs an authenticated release-metadata path, such as a maintained standard signature/attestation mechanism verified by the flasher/release process.

No custom signature algorithm is invented by Firmware-mash.

The signing/attestation key lifecycle, rotation and revocation process must be documented before public signed-update claims.

## Evidence manifest

The release package contains machine-readable feature/evidence metadata so the flasher can distinguish STABLE/BETA/LAB capabilities. It cannot advertise a hardware feature merely because the source compiled it.

## Failed flash/recovery

Recovery covers:

- disconnect/power interruption during app write;
- bad application image;
- partition/layout mismatch detected before write;
- corrupted config/message partition isolated from boot where possible;
- explicit return to known-good release through supported USB bootloader/recovery path.

The bootloader/recovery-critical range is never overwritten unnecessarily by a normal application-only update.

## Required tests

```text
UPD-001 normal upgrade preserves identity/config
UPD-002 normal upgrade preserves/reopens pending MessageStore records
UPD-003 power cut at every migration commit boundary recovers deterministic state
UPD-004 new image fails first-boot health -> safe rollback/recovery path
UPD-005 rollback image cannot corrupt unknown newer persistent schema
UPD-006 partition-layout mismatch rejected before destructive write
UPD-007 default one-click flash does not erase identity/data partitions
UPD-008 explicit factory reset clearly separates destructive flow
UPD-009 wrong chip/flash-layout preflight aborts
UPD-010 release metadata/artifact authenticity and SHA verification fail closed
UPD-011 feature evidence manifest cannot label missing hardware evidence STABLE
```

Exact migration implementation depends on the measured partition/MessageStore backend chosen after baseline.
