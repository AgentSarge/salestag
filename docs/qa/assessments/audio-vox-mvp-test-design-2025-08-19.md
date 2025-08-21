# QA Test Design — Audio VOX MVP (2025-08-19)

## Scope

Story: `docs/stories/audio-vox-mvp.md`

## Test Matrix

- Levels: -60, -50, -40, -35, -30 dBFS
- Hold: 100 ms, 500 ms, 2000 ms
- Channels: L only, R only, both
- Pre/Post Roll: 0/500/1000 ms

## Scenarios (Given/When/Then)

1. Given VOX enabled and silence, When tone at -30 dBFS for 600 ms on L, Then recording starts and includes 500 ms pre-roll.
2. Given recording active, When level < -45 dBFS for 2 s, Then recording stops and includes 500 ms post-roll.
3. Given free space < threshold, When trigger occurs, Then no recording starts and error LED blinks; log entry present.
4. Given manual start, When VOX trigger occurs, Then VOX ignored during manual session; resumes arming after stop.
5. Given stereo input, When either channel crosses start threshold, Then VOX starts.

## Test Levels

- Unit: VOX state machine, level meter, config parsing
- Integration: Audio pipeline + WAV writer + SD I/O
- E2E: On-device sound source tests

## Data & Tools

- Synthetic WAV generators; sine bursts with envelopes
- On-device logs; external audio player; power monitor

## Exit Criteria

- 100% pass on scenarios above
- No clipped headers; no corrupted files after power-loss test
- Power draw within targets
