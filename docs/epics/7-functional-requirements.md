# 7. Functional Requirements

## 7.1 Recording

- Start/stop via SW2 (short press toggles)
- Optional auto‑split by duration (5–60 min, configurable)
- Stereo PCM; fallback to mono if one channel unavailable
- WAV container (PCM 16‑bit LE); default 16 kHz (22.05/32/44.1 kHz as advanced)
- Filenames: `YYYYMMDD_HHMMSS_{NN}.wav`; RTC from SNTP/BLE; counter if no time
- Graceful stop on low battery or user stop; valid WAV header always
- Optional LED clip indication; clipping logged

## 7.2 Storage

- Auto‑mount FATFS at boot; re‑mount on reinsertion
- Free‑space check before record; configurable min free space (default 100 MB)
- Optional auto‑prune oldest files (off by default)

## 7.3 UI

- LED patterns: idle, recording, error, low battery, mounting
- Buttons:
  - SW2 short: start/stop record
  - SW2 long (>5 s): provisioning mode (BLE/Wi‑Fi AP)
  - SW1: ESP32 enable/boot

## 7.4 Connectivity & Provisioning (optional in MVP)

- BLE service: device info, time sync, config R/W, record control
- Wi‑Fi provisioning (SoftAP + captive portal or BLE)
- SNTP time sync when Wi‑Fi connected; time via BLE otherwise

## 7.5 Power Management

- States: Active Record, Idle, Light Sleep, Deep Sleep
- Auto sleep after N minutes idle; wake on SW2 or timer

## 7.6 OTA (post‑MVP)

- OTA via Wi‑Fi using ESP‑IDF OTA with image verification
