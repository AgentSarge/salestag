# MAX9814 Single Microphone Implementation Guide

## Overview

This guide provides the correct implementation for using a single MAX9814 microphone amplifier with the ESP32-S3 for the SalesTag project.

## Hardware Connections

### MAX9814 to ESP32-S3 Wiring

```
MAX9814 Module    ESP32-S3 Connection
VCC ----------->  3.3V (VDD)
GND ----------->  GND
OUT ----------->  GPIO 9 (ADC1_CH3)
GAIN ---------->  3.3V (for 40dB gain - recommended)
SHDN ---------->  3.3V (enable shutdown)
```

### Pin Configuration Details

| MAX9814 Pin | ESP32-S3 Connection | Purpose | Notes |
|-------------|-------------------|---------|-------|
| VCC | 3.3V | Power supply | Use clean 3.3V rail |
| GND | GND | Ground | Common ground |
| OUT | GPIO 9 (ADC1_CH3) | Audio output | Single-ended analog |
| GAIN | 3.3V | Gain control | 40dB gain (optimal) |
| SHDN | 3.3V | Shutdown control | Keep enabled |

## MAX9814 Configuration Based on Datasheet

### Gain Settings (GAIN Pin)

According to the MAX9814 datasheet:

- **GAIN = VDD (3.3V)**: 40dB gain - **RECOMMENDED**
- **GAIN = GND**: 50dB gain - High sensitivity
- **GAIN = unconnected**: 60dB gain - Maximum sensitivity

### AGC (Automatic Gain Control)

The MAX9814 includes built-in AGC with these characteristics:
- **Attack Time**: 1.1ms (with 470nF CCT capacitor)
- **Release Ratio**: 1:500 (A/R = GND), 1:2000 (A/R = VDD), 1:4000 (A/R = unconnected)
- **Regulated Output**: 1.26V to 1.54V peak-to-peak

### Electrical Characteristics

- **Operating Voltage**: 2.7V to 5.5V (3.3V compatible)
- **Supply Current**: 3.1mA to 6mA typical
- **Input-Referred Noise**: 30 nV/√Hz
- **THD+N**: 0.04% typical
- **Dynamic Range**: 60dB

## Software Implementation

### ADC Configuration

The current implementation uses:
- **ADC Channel**: ADC1_CH3 (GPIO 9)
- **Sample Rate**: 16kHz
- **Resolution**: 12-bit (0-4095)
- **Attenuation**: 12dB (0-3.3V range)

### Audio Processing Pipeline

1. **Raw ADC Reading**: 12-bit values (0-4095)
2. **Voltage Conversion**: Convert to voltage (0-3.3V)
3. **DC Blocking**: Remove 1.25V DC bias
4. **Scaling**: Scale to 16-bit audio range (-32768 to +32767)
5. **Clipping Protection**: Prevent overflow

### Key Constants

```c
#define MAX9814_OUTPUT_VOLTAGE 2.0f    // 2Vpp output
#define MAX9814_DC_OFFSET 1.25f        // DC bias voltage
#define MAX9814_GAIN_DB 40.0f          // 40dB gain setting
#define ADC_REFERENCE_VOLTAGE 3.3f     // ESP32 ADC reference
```

## Testing and Validation

### Expected ADC Readings

| Condition | Expected ADC Value | Voltage | Notes |
|-----------|-------------------|---------|-------|
| Silence | ~1550 | ~1.25V | DC bias level |
| Quiet speech | 1200-1900 | 0.96V-1.54V | Low-level audio |
| Normal speech | 800-2300 | 0.64V-1.84V | Typical levels |
| Loud speech | 400-2700 | 0.32V-2.16V | High-level audio |

### Audio Quality Validation

- **Signal-to-Noise Ratio**: >60dB expected
- **Frequency Response**: 20Hz-20kHz
- **Distortion**: <0.1% THD+N
- **Dynamic Range**: 60dB

## Troubleshooting

### Common Issues

1. **No Audio Signal**
   - Check VCC connection (3.3V)
   - Verify SHDN pin is HIGH
   - Confirm ADC channel configuration

2. **Low Audio Levels**
   - Check GAIN pin connection
   - Verify microphone element
   - Adjust software scaling

3. **High Noise**
   - Check ground connections
   - Verify power supply stability
   - Check for EMI sources

4. **Clipping**
   - Reduce GAIN setting
   - Check AGC configuration
   - Verify ADC attenuation

### Debug Commands

```bash
# Monitor ADC values in real-time
idf.py monitor

# Check for audio capture initialization
grep "audio_capture" monitor_output

# Verify GPIO configuration
grep "GPIO.*9" monitor_output
```

## Performance Optimization

### For Best Audio Quality

1. **Use 40dB gain** (GAIN = VDD) for balanced performance
2. **Enable AGC** for dynamic range control
3. **Use DC blocking filter** in software
4. **Implement proper scaling** to prevent clipping
5. **Use continuous ADC mode** for consistent sampling

### Power Considerations

- **Supply Current**: ~4mA typical
- **Power Supply**: Clean 3.3V rail
- **Decoupling**: 0.1μF ceramic capacitor near VCC pin

## Integration with SalesTag

The single MAX9814 implementation is optimized for:
- **Voice recording** applications
- **Speech recognition** preprocessing
- **Audio quality** over quantity
- **Simplified hardware** design
- **Reduced power consumption**

This configuration provides excellent audio quality while maintaining simplicity and reliability for the SalesTag project.
