# SalesTag Smart Badge – Firmware PRD

## 1. Overview

SalesTag is a portable IoT audio‑recording badge built on the ESP32‑S3 with dual microphones via MAX9814 preamps, microSD storage, USB‑C charging, LED/status indicators, and user buttons. This PRD defines the v1 firmware scope.

Reference hardware: see `docs/hardware/README.md`.

## 2. Goals

- Reliable voice‑quality recording (stereo) to microSD with robust file handling
- Simple user interaction (button to record/stop, LED for status)
- Low‑power operation suitable for a wearable badge
- Optional wireless control and time sync; OTA‑ready platform

## 3. Non‑Goals (v1)

- Cloud backend or mobile app beyond basic provisioning
- On‑device ML beyond simple level/clip detection
- Complex lossy codecs (e.g., Opus) in MVP

## 4. Users & Scenarios

- Sales reps, interviewers, field staff needing portable recording
- Ad‑hoc voice notes, meeting capture, on‑the‑go interviews

## 5. Hardware Summary (condensed)

- MCU: ESP32‑S3‑MINI‑1‑N8
- Audio: 2× MAX9814 mic amps + 2× electret mics
- Storage: microSD (SPI — CS IO39, MOSI IO35, SCLK IO36, MISO IO37)
- UI: LED1 (IO40), SW1 Enable/Boot, SW2 user button (IO4)
- Power: USB‑C, MCP73831 charger, 3.3V LDO
  See `docs/hardware/README.md` for full details.

## 6. Constraints & Assumptions

- ADC capture (I2S/ADC mode) for two analog channels
- FATFS on microSD; remove card only when not recording
- Battery powered; minimize active current; sleep states available
- ESP‑IDF toolchain preferred (or PlatformIO ESP‑IDF)

## 7. Functional Requirements

### 7.1 Recording

- Start/stop via SW2 (short press toggles)
- Optional auto‑split by duration (5–60 min, configurable)
- Stereo PCM; fallback to mono if one channel unavailable
- WAV container (PCM 16‑bit LE); default 16 kHz (22.05/32/44.1 kHz as advanced)
- Filenames: `YYYYMMDD_HHMMSS_{NN}.wav`; RTC from SNTP/BLE; counter if no time
- Graceful stop on low battery or user stop; valid WAV header always
- Optional LED clip indication; clipping logged

### 7.2 Storage

- Auto‑mount FATFS at boot; re‑mount on reinsertion
- Free‑space check before record; configurable min free space (default 100 MB)
- Optional auto‑prune oldest files (off by default)

### 7.3 UI

- LED patterns: idle, recording, error, low battery, mounting
- Buttons:
  - SW2 short: start/stop record
  - SW2 long (>5 s): provisioning mode (BLE/Wi‑Fi AP)
  - SW1: ESP32 enable/boot

### 7.4 Connectivity & Provisioning (optional in MVP)

- BLE service: device info, time sync, config R/W, record control
- Wi‑Fi provisioning (SoftAP + captive portal or BLE)
- SNTP time sync when Wi‑Fi connected; time via BLE otherwise

### 7.5 Power Management

- States: Active Record, Idle, Light Sleep, Deep Sleep
- Auto sleep after N minutes idle; wake on SW2 or timer

### 7.6 OTA (post‑MVP)

- OTA via Wi‑Fi using ESP‑IDF OTA with image verification

## 8. Non‑Functional

- Robustness: no corruption on power loss (periodic header updates, fsync on split/stop)
- Throughput: sustain stereo 16‑bit at selected sample rate
- Latency: record start < 500 ms from button
- Battery life: targets — >8 h idle, >2–4 h continuous record (to be validated)
- Security: encrypted provisioning channels; TLS for OTA/upload when added

## 9. Configuration

Stored in NVS and optional `/config.json` on SD.

- audio.sample_rate (16000)
- audio.bits_per_sample (16)
- audio.channels (1|2)
- rec.auto_split_minutes (0=off)
- storage.min_free_mb (100)
- ui.led_brightness (0–100)
- power.idle_sleep_minutes
- provisioning.mode (off|ble|ap)

## 10. Telemetry & Logs

- Local text log on SD: boot, errors, record sessions, battery state
- Optional BLE log stream in provisioning mode

## 11. Acceptance Criteria (MVP)

1. Stereo WAV 16 kHz/16‑bit recorded to SD; playable on desktop
2. Start/stop by button with LED feedback; handles rapid toggles
3. Power loss during record yields playable file up to last seconds
4. Auto‑split works without audible gaps
5. Idle sleep, wake on button < 300 ms
6. BLE provisioning and time sync available if MVP‑enabled

## 12. Milestones

- M0: Bring‑up — SD, LED, button, basic ADC capture
- M1: WAV writer with DMA double buffering; sustained write
- M2: Record control, LED states, error handling, time‑based filenames
- M3: Power states, storage guardrails
- M4: BLE provisioning + time sync (optional MVP+)
- M5: OTA foundations (post‑MVP)

## 13. Open Questions

- Exact ADC channel mapping for MIC_DATA1/2 per schematic
- Final production sample rate choice (quality vs power)
- Default split duration and file size limits
- Battery gauge vs voltage thresholds for low‑battery behavior

## 14. R0 – Minimal Bring-up Scope ✅ COMPLETED

### R0 Goals

- Power on reliably ✅
- Button press starts recording ✅
- Button press again stops recording ✅
- LED on while recording ✅
- Save a single WAV file to local flash for MVP ✅

## 15. R1 – SD Card Integration Scope

### R1 Goals

- Mount SD card successfully on boot
- Create WAV files on SD card instead of internal flash
- Complete button-to-WAV recording pipeline
- Handle SD card errors gracefully

### Functional Requirements (R1)

- On boot, initialize SD card with FATFS at `/sdcard` mount point
- Create `/sdcard/rec` directory structure automatically
- Button press starts recording to `/sdcard/rec/rec_0001.wav`
- Second button press stops recording and finalizes WAV file
- LED remains on during entire recording period
- Handle SD card removal/insertion without crashing

### Non-Functional Requirements (R1)

- Mount latency: SD card ready within 2 seconds of boot
- File integrity: WAV files are valid PCM format with correct headers
- Error handling: graceful degradation when SD card unavailable
- Storage capacity: utilize full SD card space for recordings

### Acceptance Criteria (R1)

1. SD card mounts successfully at `/sdcard` on boot
2. `/sdcard/rec` directory created automatically if it doesn't exist
3. Button press starts recording and saves to `/sdcard/rec/rec_0001.wav`
4. Second button press stops recording and finalizes WAV file
5. WAV file is valid PCM format with correct header and non-zero length
6. LED remains on during entire recording period
7. Device handles SD card removal/insertion gracefully

### Functional Requirements (R0)

- On boot, initialize NVS, GPIO, and a simple filesystem on internal flash; SD card not required for R0.
- Short press on SW2 starts audio capture and writes a single WAV file to internal flash; subsequent short press stops and finalizes the same file.
- LED1 turns on within 100 ms of recording start and turns off within 100 ms of recording stop.
- If recording is already active, additional button presses are debounced and ignored except the stop press.
- On recording stop (user press), WAV header is finalized and file is closed successfully.

### Non-Functional Requirements (R0)

- Start latency: recording begins within 500 ms of button press.
- File integrity: resulting WAV is playable on desktop after normal stop; on unexpected reset during recording, file may be truncated but must remain recoverable by standard audio tools.
- Power: no deep-sleep required for R0; idle loop permitted.
- Simplicity: single fixed format WAV (mono, 16‑bit PCM, 16 kHz).

### Acceptance Criteria (R0)

1. From power-on, a single short press on SW2 begins recording; LED turns on and remains on.
2. A second short press stops recording; LED turns off; exactly one WAV file exists in internal flash and is playable.
3. Debounce: two rapid presses (<100 ms apart) do not start and stop erroneously.
4. Power-loss test: if device resets during recording, no boot crash occurs and partial file does not corrupt the filesystem.

### Risks (R0)

- Limited internal flash capacity; risk of running out of space.
- Flash wear from repeated writes; mitigate by keeping R0 usage minimal and single-file.
- Audio front-end gain/levels may clip without calibration; acceptable for R0.
- Button bounce leading to unintended toggles; addressed via debounce threshold.
