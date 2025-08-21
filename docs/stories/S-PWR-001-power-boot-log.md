# Story S-PWR-001 — Power on and boot log

Status: Draft

## Story Statement

On power-up, the device boots reliably and emits a BOOT log with firmware version and build info.

## Acceptance Criteria

- A1: Device powers on and reaches app_main without resets.
- A2: Serial monitor shows `BOOT` line including version and git hash.
- A3: No fatal errors on init; returns to idle.

## Tasks

- Emit BOOT log with version/hash
- Minimal init: NVS, GPIO
- Add README run recipe and capture log transcript to `docs/qa/assessments/logs/R0_build.txt`
