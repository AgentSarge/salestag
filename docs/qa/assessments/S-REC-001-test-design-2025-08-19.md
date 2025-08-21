# Test Design: S-REC-001 — Button-to-Record with LED and WAV

Date: 2025-08-19
Designer: Quinn (Test Architect)

## Test Strategy Overview

- Total test scenarios: 9
- Unit: 3, Integration: 4, HIL (E2E): 2
- Priorities: P0: 4, P1: 4, P2: 1

## Scenarios

### Unit (P0)

- U1 Debounce edges: press/release with jitter → single BTN_DOWN/BTN_UP
- U2 State machine: IDLE↔RECORDING toggles only on debounced tap
- U3 LED driver: state drives LED within 100 ms budget

### Integration (P0/P1)

- I1 One-minute capture: generated PCM → WAV header and size correct (P0)
- I2 File finalization: header fields updated on stop; playable (P0)
- I3 LED timing measurement from logs REC_START/REC_STOP events (P1)
- I4 Error path: stop when not recording is no-op (P2)

### Hardware-in-Loop (P1)

- H1 Five start-stop cycles: LED timing < 200 ms; files playable
- H2 Power yank during recording: reboot and remount; FS intact

## Evidence

- Logs: `docs/qa/assessments/logs/R0_build.txt`, `docs/qa/assessments/logs/R0_monitor.txt`
- Sample WAV: `docs/qa/assessments/samples/R0_first.wav`

## Run Recipe

```bash
# Build & flash
cd software_v1
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/tty.usbmodem* flash monitor | tee ../docs/qa/assessments/logs/R0_monitor.txt
```

Steps:

1. Tap button (start), confirm LED on and REC_START log
2. Wait ~10s, tap again (stop), confirm LED off and REC_STOP
3. Copy /rec/rec_0001.wav and attach as R0_first.wav
4. Yank power during active recording; reboot; verify mount success
