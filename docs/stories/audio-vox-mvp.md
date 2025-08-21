# Story: Audio VOX MVP

## Summary

Implement voice-activated recording (VOX) minimal viable feature: when input level exceeds a configurable threshold for a sustained duration, start recording; stop after configurable silence duration. Persist audio as WAV to SD per PRD conventions.

## Motivation

Reduce storage and power consumption by recording only when meaningful audio is present.

## Requirements

- Config keys:
  - `vox.enabled` (default: false)
  - `vox.start_threshold_dbfs` (default: -35 dBFS)
  - `vox.start_hold_ms` (default: 500 ms)
  - `vox.stop_threshold_dbfs` (default: -45 dBFS)
  - `vox.stop_hold_ms` (default: 2000 ms)
  - `vox.pre_roll_ms` (default: 500 ms)
  - `vox.post_roll_ms` (default: 500 ms)
- LED indicates VOX armed (slow pulse) and active recording (solid/blink per UI scheme)
- Works with stereo input; triggers on either channel
- Respects storage free-space guardrails; if below threshold, do not start
- Filenames follow timestamp pattern; auto-split still applies

## Acceptance Criteria

1. When `vox.enabled=true`, sustained level over `start_threshold_dbfs` for `start_hold_ms` begins recording; include `pre_roll_ms` audio.
2. Sustained level under `stop_threshold_dbfs` for `stop_hold_ms` stops recording; include `post_roll_ms` audio.
3. No recordings start when free space < configured minimum.
4. LED patterns reflect VOX armed vs recording vs error states.
5. Manual button start/stop overrides VOX; VOX resumes arming after manual stop.

## Tasks

- Add VOX configuration keys (NVS + optional config.json)
- Implement rolling level meter (per-channel RMS/peak) in audio capture path
- Implement VOX state machine (Armed → Pre-Roll → Recording → Post-Roll)
- Integrate with WAV writer ring buffer to splice pre/post roll
- Update UI controller for VOX indications
- Add logs for VOX transitions (armed, start, stop, thresholds)
- Add integration with storage guardrails

## Risks & Considerations

- False triggers in noisy environments; tune defaults and smoothing window
- SD latency could drop pre-roll if buffer sizing insufficient
- Power: continuous metering impacts idle current; evaluate light-sleep strategy

## Definition of Done

- All acceptance criteria met
- Logs demonstrate VOX triggers and clean start/stop
- Test recordings validate pre/post roll and thresholds
- QA risk and test design docs completed
