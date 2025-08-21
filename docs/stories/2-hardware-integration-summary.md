# 2. Hardware Integration Summary

- MCU: ESP32‑S3‑MINI‑1‑N8
- Audio Front‑End: 2× MAX9814 -> ESP32 ADC (I2S ADC mode)
- Storage: microSD over SPI (CS IO39, MOSI IO35, SCLK IO36, MISO IO37)
- UI: LED (IO40), SW2 button (IO4), SW1 enable/boot
- Power: USB‑C, MCP73831 charger, 3.3V regulator
  See `docs/hardware/README.md` for pinouts and details.
