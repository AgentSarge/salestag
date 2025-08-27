# SalesTag Hardware Validation Guide

## Overview

This guide provides electrical and signal validation procedures for SalesTag hardware before software testing. Use with multimeter, oscilloscope, or logic analyzer for comprehensive validation.

## Pre-Power Validation

### Power Supply Verification

| Test Point | Expected Value | Measured Value | Status |
|------------|----------------|----------------|---------|
| 3.3V Rail | 3.3V ±0.1V | _____________ | ⬜ Pass ⬜ Fail |
| 5V USB Rail | 5.0V ±0.25V | _____________ | ⬜ Pass ⬜ Fail |
| Ground Continuity | 0Ω resistance | _____________ | ⬜ Pass ⬜ Fail |

### GPIO Connectivity Test

**With ESP32-S3 Powered OFF:**

| GPIO Pin | Expected Resistance to GND | Measured Resistance | Status |
|----------|---------------------------|-------------------|---------|
| GPIO 4 (Button) | >10kΩ (pullup) | _____________ | ⬜ Pass ⬜ Fail |
| GPIO 40 (LED) | >1kΩ (LED series R) | _____________ | ⬜ Pass ⬜ Fail |
| GPIO 9 (MIC1) | >10kΩ (high impedance) | _____________ | ⬜ Pass ⬜ Fail |
| GPIO 12 (MIC2) | >10kΩ (high impedance) | _____________ | ⬜ Pass ⬜ Fail |

## Power-On Validation

### Boot Current Measurement

| Boot Phase | Expected Current | Measured Current | Duration | Status |
|------------|------------------|------------------|----------|---------|
| Initial Boot | 150-300mA | _____________ | 0-2s | ⬜ Pass ⬜ Fail |
| Idle State | 50-100mA | _____________ | Steady | ⬜ Pass ⬜ Fail |
| Recording Active | 100-200mA | _____________ | 10s | ⬜ Pass ⬜ Fail |

### GPIO Signal Levels

**With System Booted and Idle:**

| GPIO | Expected Voltage | Measured Voltage | Status |
|------|------------------|------------------|---------|
| GPIO 4 (Button unpressed) | 3.3V | _____________ | ⬜ Pass ⬜ Fail |
| GPIO 4 (Button pressed) | 0V | _____________ | ⬜ Pass ⬜ Fail |
| GPIO 40 (LED OFF) | 0V | _____________ | ⬜ Pass ⬜ Fail |
| GPIO 40 (LED ON) | 3.3V | _____________ | ⬜ Pass ⬜ Fail |

## Audio System Validation

### Microphone Power and Bias

**MAX9814 Module Validation:**

| Test Point | Expected Value | MIC1 Measured | MIC2 Measured | Status |
|------------|----------------|---------------|---------------|---------|
| VCC Input | 3.3V ±0.1V | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| Output DC Bias | 1.6V ±0.2V | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| Output AC (silence) | <10mV RMS | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| Output AC (speech) | >50mV RMS | _____________ | _____________ | ⬜ Pass ⬜ Fail |

### ADC Input Validation

**ESP32-S3 ADC Input Levels:**

| Condition | Expected ADC Reading | MIC1 (GPIO 9) | MIC2 (GPIO 12) | Status |
|-----------|---------------------|----------------|----------------|---------|
| Silence | 2048 ±200 (12-bit) | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| Moderate Speech | 1500-2500 range | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| Loud Speech | 1000-3000 range | _____________ | _____________ | ⬜ Pass ⬜ Fail |

**Note:** ADC readings displayed in serial monitor during recording sessions.

## SD Card Interface Validation

### SPI Signal Validation

**With Oscilloscope on SPI Pins During SD Card Access:**

| Signal | Expected Frequency | Measured Frequency | Signal Quality | Status |
|--------|-------------------|-------------------|----------------|---------|
| SCLK (GPIO 36) | ~10MHz | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| MOSI (GPIO 35) | Data sync with SCLK | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| MISO (GPIO 37) | Data sync with SCLK | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| CS (GPIO 39) | Active low select | _____________ | _____________ | ⬜ Pass ⬜ Fail |

### SD Card Power and Detection

| Test | Expected Behavior | Actual Behavior | Status |
|------|-------------------|-----------------|---------|
| Card inserted detection | Mount successful | _____________ | ⬜ Pass ⬜ Fail |
| Card removal detection | Error handling | _____________ | ⬜ Pass ⬜ Fail |
| Card power consumption | <100mA additional | _____________ | ⬜ Pass ⬜ Fail |

## Signal Timing Validation

### Button Response Timing

**With Logic Analyzer or Oscilloscope:**

| Signal Path | Expected Timing | Measured Timing | Status |
|-------------|----------------|-----------------|---------|
| Button press to GPIO 4 low | <1ms | _____________ | ⬜ Pass ⬜ Fail |
| GPIO 4 low to LED response | <100ms | _____________ | ⬜ Pass ⬜ Fail |
| Recording start to LED ON | <500ms | _____________ | ⬜ Pass ⬜ Fail |
| 10-second timeout accuracy | 10.0s ±0.1s | _____________ | ⬜ Pass ⬜ Fail |

### Audio Sampling Timing

**During Active Recording:**

| Parameter | Expected Value | Measured Value | Status |
|-----------|----------------|----------------|---------|
| Sample rate | 16kHz ±1% | _____________ | ⬜ Pass ⬜ Fail |
| Sample period | 62.5μs ±1% | _____________ | ⬜ Pass ⬜ Fail |
| Channel timing | Simultaneous L/R | _____________ | ⬜ Pass ⬜ Fail |

## Temperature and Environmental Testing

### Operating Temperature Range

| Test Condition | Target Range | Measured Performance | Status |
|----------------|--------------|---------------------|---------|
| Room temperature (20°C) | Full functionality | _____________ | ⬜ Pass ⬜ Fail |
| Cold operation (0°C) | Full functionality | _____________ | ⬜ Pass ⬜ Fail |
| Warm operation (40°C) | Full functionality | _____________ | ⬜ Pass ⬜ Fail |

**Note:** PRD specifies -10°C to 50°C operating range for field conditions.

### Vibration and Mechanical Stress

| Test | Expected Behavior | Actual Behavior | Status |
|------|-------------------|-----------------|---------|
| Light tapping | No connection issues | _____________ | ⬜ Pass ⬜ Fail |
| Cable flexing | Stable USB connection | _____________ | ⬜ Pass ⬜ Fail |
| Component seating | No loose connections | _____________ | ⬜ Pass ⬜ Fail |

## Advanced Signal Analysis

### Audio Signal Quality Analysis

**Using Audio Spectrum Analyzer or Computer Analysis:**

| Frequency Range | Expected Response | Measured Response | Status |
|-----------------|-------------------|------------------|---------|
| 300-3400Hz (voice) | Flat ±3dB | _____________ | ⬜ Pass ⬜ Fail |
| 50-100Hz (noise floor) | <-40dB | _____________ | ⬜ Pass ⬜ Fail |
| >8kHz (aliasing) | <-30dB | _____________ | ⬜ Pass ⬜ Fail |

### Channel Matching

| Parameter | Specification | Measured Difference | Status |
|-----------|---------------|-------------------|---------|
| Gain matching | <1dB difference | _____________ | ⬜ Pass ⬜ Fail |
| Phase matching | <10° at 1kHz | _____________ | ⬜ Pass ⬜ Fail |
| Noise floor | <5dB difference | _____________ | ⬜ Pass ⬜ Fail |

## Troubleshooting Guide

### Common Hardware Issues

| Symptom | Probable Cause | Validation Test | Solution |
|---------|----------------|-----------------|----------|
| No audio capture | Microphone power | Check VCC at MAX9814 | Verify 3.3V supply |
| One channel silent | Wiring issue | Check GPIO continuity | Rewire connection |
| High noise floor | Ground loop | Check ground connections | Single point ground |
| SD card errors | SPI timing | Check SCLK frequency | Verify 10MHz operation |
| Button not responsive | Pull-up issue | Measure GPIO 4 voltage | Check internal pullup |
| LED not working | Current limiting | Check LED current | Verify series resistor |

### Signal Quality Issues

| Issue | Validation Method | Expected Fix |
|-------|------------------|--------------|
| Audio distortion | Oscilloscope on ADC input | Reduce microphone gain |
| Timing jitter | Logic analyzer on SPI | Check crystal oscillator |
| Power supply noise | AC couple scope on VCC | Add decoupling capacitors |
| Ground bounce | Differential scope measurement | Improve ground plane |

## Test Equipment Requirements

### Essential Equipment

- **Digital Multimeter**: For DC voltage and resistance measurements
- **Oscilloscope**: For signal timing and quality analysis (minimum 100MHz)
- **Function Generator**: For audio signal injection testing
- **Power Supply**: For current consumption measurement

### Recommended Equipment

- **Logic Analyzer**: For digital signal timing validation
- **Audio Spectrum Analyzer**: For audio quality assessment
- **Signal Generator**: For calibrated audio test signals
- **Environmental Chamber**: For temperature testing

### Software Tools

- **Audio Analysis**: Audacity, Adobe Audition, or similar
- **Spectrum Analysis**: MATLAB, Python scipy, or dedicated tools
- **Data Logging**: Serial monitor with data capture capability

## Validation Completion

### Hardware Validation Checklist

- [ ] All power supply voltages within specification
- [ ] GPIO connectivity verified for all signals
- [ ] Microphone power and bias levels correct
- [ ] SD card SPI interface functioning
- [ ] Button and LED response verified
- [ ] Audio signal path validated
- [ ] Timing measurements within tolerance
- [ ] No short circuits or wiring errors detected

### Sign-off

**Hardware Validation Date:** _____________  
**Validated By:** _____________  
**Equipment Used:** _____________  
**Board Revision:** _____________  

**Overall Hardware Status:** ⬜ PASS ⬜ FAIL ⬜ CONDITIONAL

**Notes:** _________________________________________________________________
_________________________________________________________________________