# Story S-IO-001 — Button and LED debounce proof

Status: Draft

## Story Statement

Demonstrate debounced button input on IO4 and LED output on IO40 with reliable BTN_DOWN/BTN_UP logs and LED toggle.

## Acceptance Criteria

- A1: BTN_DOWN emitted only once per physical press; no chatter within 20 ms window.
- A2: BTN_UP emitted once per release; no false up events.
- A3: LED changes state within 100 ms of debounced edge.

## Tasks

- Configure IO4 with pull-up, debounce 5–20 ms
- Map LED IO40, provide set/on/off API
- Log edges with timestamps; capture transcript to logs
