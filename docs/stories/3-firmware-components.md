# 3. Firmware Components

- App Coordinator: boot, lifecycle, state machine owner
- Audio Capture Service: configures ADC/I2S, double‑buffered DMA, level/clip detection
- WAV Writer: ring buffer consumer that serializes PCM to WAV and does fsync on boundaries
- SD Storage Manager: mount/re‑mount, free‑space checks, pruning policy, filenames
- UI Controller: LED patterns, button debouncing and gestures
- Power Manager: state transitions (Idle/Record/Light/Deep), wake sources
- Time & Config: NVS + optional `/config.json`; SNTP or BLE time sync
- Connectivity (optional): BLE GATT; Wi‑Fi provisioning/AP when enabled
- OTA (post‑MVP): ESP‑IDF OTA hooks
