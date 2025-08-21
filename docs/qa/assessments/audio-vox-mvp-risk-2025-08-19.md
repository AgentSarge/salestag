# QA Risk Assessment — Audio VOX MVP (2025-08-19)

## Context

Story: `docs/stories/audio-vox-mvp.md`

## Risk Summary (Probability × Impact; 1–9)

- SD Throughput Starvation During Pre/Post Roll: 6
- False Triggers in Noisy Environments: 6
- Missed Triggers at Low Levels: 4
- Battery Drain from Continuous Metering: 5
- Clock/RTC Not Set (filenames/cadence issues): 3
- Config Drift Between NVS and config.json: 3

## Top Risks & Mitigations

1. SD Throughput Starvation
   - Mitigation: Double-buffer DMA; size ring buffer for pre/post roll; fsync only on stop/split.
2. False Triggers
   - Mitigation: RMS smoothing window; hysteresis (start/stop thresholds); tune defaults; optional noise floor calibration.
3. Battery Drain
   - Mitigation: Reduce sample rate in armed idle; light sleep with periodic metering; disable Wi‑Fi/BLE.

## Checkpoints

- Mid-dev: Verify throughput and buffer occupancy under VOX start/stop storms
- Pre-merge: Noise-floor tests across environments

## Evidence to Collect

- Buffer metrics logs
- Power draw measurements in armed vs recording
- Test recordings with annotated trigger times
