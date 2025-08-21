# Story S-REC-001 — Button-to-Record with LED and WAV to /rec/rec_0001.wav

Status: Draft

## Summary

- User taps the button to start recording
- LED turns on while recording
- User taps again to stop
- A WAV file is written to local flash

## Acceptance Criteria

- A1: Press within 50 to 300 ms begins recording
- A2: LED is solid on for the entire recording period
- A3: Second press stops recording and LED turns off within 200 ms
- A4: A valid PCM WAV at 16 kHz mono exists at /rec/rec_0001.wav with correct header and nonzero length
- A5: Survives power loss during recording without bricking the device. Next boot mounts storage successfully

## Tasks

- T1: Define pin map in code and docs, enable internal pull as needed
- T2: Debounce logic with 5 to 20 ms window
- T3: Record state machine IDLE and RECORDING
- T4: Minimal I2S capture and file writer
- T5: LED driver tied to state machine
- T6: Logs: BOOT, BTN_DOWN, BTN_UP, REC_START, REC_STOP, WAV_BYTES, ERR_xxx

## Definition of Done

- Unit tests pass for debounce and state transitions
- Hardware smoke test produces a playable WAV
- Logs captured and saved to docs/qa/assessments/logs/
- QA gate decision recorded

## Out of Scope

- VOX, BLE config, retries, pre-roll

## QA Results

### Review Date: 2025-08-19

### Reviewed By: Quinn (Test Architect)

### Code Quality Assessment ✅ COMPLETED

**R0 BRING-UP SUCCESSFUL**: Core functionality implemented and validated on hardware.

**Evidence Captured:**

- Build successful with no compilation errors
- Device boots cleanly with proper GPIO initialization
- Button detection working (GPIO4 with pullup, 170ms response time)
- LED control working (GPIO40 output, clean toggle functionality)
- Device stable through multiple button press cycles

**Hardware Validation:**

- GPIO[4] configured as input with pullup (button)
- GPIO[40] configured as output (LED)
- Button debouncing effective (clean BTN_DOWN/BTN_UP detection)
- LED toggle responding to button presses (LED_ON/LED_OFF confirmed)

**Status: CORE R0 OBJECTIVES ACHIEVED** ✅

### Compliance Check

- Coding Standards: ✓ (scaffold aligned)
- Project Structure: ✓ (modules under software_v1/main)
- Testing Strategy: ✗ (unit/integration tests pending)
- All ACs Met: Pending bench validation

### Improvements Checklist

- [ ] Add unit tests for debounce and state transitions
- [ ] Collect on-device logs and sample WAV
- [ ] Measure LED off latency at stop

### Files Modified During Review

- Refer to implementation modules in `software_v1/main/`

### Gate Status

Gate: CONCERNS → qa.qaLocation/gates/E0.S-REC-001.yml
Risk profile: qa.qaLocation/assessments/S-REC-001-risk-2025-08-19.md
NFR assessment: qa.qaLocation/assessments/S-REC-001-nfr-2025-08-19.md
