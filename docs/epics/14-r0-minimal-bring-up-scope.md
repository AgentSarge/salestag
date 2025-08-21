# 14. R0 – Minimal Bring-up Scope

## R0 Goals

- Power on reliably
- Button press starts recording
- Button press again stops recording
- LED on while recording
- Save a single WAV file to local flash for MVP

## Functional Requirements (R0)

- On boot, initialize NVS, GPIO, and a simple filesystem on internal flash; SD card not required for R0.
- Short press on SW2 starts audio capture and writes a single WAV file to internal flash; subsequent short press stops and finalizes the same file.
- LED1 turns on within 100 ms of recording start and turns off within 100 ms of recording stop.
- If recording is already active, additional button presses are debounced and ignored except the stop press.
- On recording stop (user press), WAV header is finalized and file is closed successfully.

## Non-Functional Requirements (R0)

- Start latency: recording begins within 500 ms of button press.
- File integrity: resulting WAV is playable on desktop after normal stop; on unexpected reset during recording, file may be truncated but must remain recoverable by standard audio tools.
- Power: no deep-sleep required for R0; idle loop permitted.
- Simplicity: single fixed format WAV (mono, 16‑bit PCM, 16 kHz).

## Acceptance Criteria (R0)

1. From power-on, a single short press on SW2 begins recording; LED turns on and remains on.
2. A second short press stops recording; LED turns off; exactly one WAV file exists in internal flash and is playable.
3. Debounce: two rapid presses (<100 ms apart) do not start and stop erroneously.
4. Power-loss test: if device resets during recording, no boot crash occurs and partial file does not corrupt the filesystem.

## Risks (R0)

- Limited internal flash capacity; risk of running out of space.
- Flash wear from repeated writes; mitigate by keeping R0 usage minimal and single-file.
- Audio front-end gain/levels may clip without calibration; acceptable for R0.
- Button bounce leading to unintended toggles; addressed via debounce threshold.
