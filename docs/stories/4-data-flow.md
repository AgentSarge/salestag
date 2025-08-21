# 4. Data Flow

Button → App State Machine → (start) Audio Capture → PCM buffers → WAV Writer → FATFS (SD)

Optional: Time Sync (BLE/Wi‑Fi) → RTC → Filename timestamps
