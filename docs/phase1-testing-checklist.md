# SalesTag Phase 1 Testing Checklist

## Overview

This document provides a comprehensive testing checklist for validating Phase 1 implementation against PRD requirements. Each test includes expected results and space for recording actual results.

**Test Environment Requirements:**
- ESP32-S3 development board
- MAX9814 microphone modules (2x) 
- MicroSD card (FAT32 formatted, 32GB)
- Button and LED connected per GPIO configuration
- USB-C cable for programming and power
- ESP-IDF v5.2.2 development environment

## Pre-Test Setup Validation

### ST-001: Hardware Connections
**Objective**: Verify all hardware connections match design specifications

| Component | Expected Connection | Actual Connection | Status |
|-----------|-------------------|-------------------|---------|
| Button | GPIO 4 (with pullup) | _____________ | ⬜ Pass ⬜ Fail |
| LED | GPIO 40 (with current limit resistor) | _____________ | ⬜ Pass ⬜ Fail |
| MIC1 | GPIO 9 (ADC1_CH3) | _____________ | ⬜ Pass ⬜ Fail |
| MIC2 | GPIO 12 (ADC1_CH6) | _____________ | ⬜ Pass ⬜ Fail |
| SD CS | GPIO 39 | _____________ | ⬜ Pass ⬜ Fail |
| SD MOSI | GPIO 35 | _____________ | ⬜ Pass ⬜ Fail |
| SD MISO | GPIO 37 | _____________ | ⬜ Pass ⬜ Fail |
| SD SCLK | GPIO 36 | _____________ | ⬜ Pass ⬜ Fail |

**Notes:** _________________________________

### ST-002: Build Environment Validation
**Objective**: Confirm ESP-IDF toolchain and project configuration

| Test Step | Expected Result | Actual Result | Status |
|-----------|----------------|---------------|---------|
| `idf.py --version` | ESP-IDF v5.2.2 | _____________ | ⬜ Pass ⬜ Fail |
| `idf.py set-target esp32s3` | Target set successfully | _____________ | ⬜ Pass ⬜ Fail |
| `idf.py build` | Build successful, no errors | _____________ | ⬜ Pass ⬜ Fail |
| Build size check | App size < 1.9MB (flash limit) | _____________ | ⬜ Pass ⬜ Fail |

**Build Output Analysis:**
- Total app size: _____________ bytes
- Largest components: _____________
- Build time: _____________ seconds
- Warnings count: _____________

## Functional Testing

### ST-003: System Initialization (PRD Baseline)
**Objective**: Validate system boots and initializes all components

**Test Procedure:**
1. Flash firmware: `idf.py flash`
2. Start monitor: `idf.py monitor`
3. Reset device and capture boot sequence

| Boot Stage | Expected Serial Output | Actual Output | Status |
|------------|----------------------|---------------|---------|
| Application Start | `=== SalesTag Audio Recording System v1.0 ===` | _____________ | ⬜ Pass ⬜ Fail |
| NVS Init | `NVS initialized` | _____________ | ⬜ Pass ⬜ Fail |
| UI Init | `UI initialized - button callback registered` | _____________ | ⬜ Pass ⬜ Fail |
| SD Storage | `SD card storage initialized successfully` | _____________ | ⬜ Pass ⬜ Fail |
| Recorder Init | `Recorder initialized - 16kHz, 16-bit, 2 channels` | _____________ | ⬜ Pass ⬜ Fail |
| Microphone ADC | `MIC1: GPIO 9 (ADC1_CH3) - MIC_DATA1`<br>`MIC2: GPIO 12 (ADC1_CH6) - MIC_DATA2` | _____________ | ⬜ Pass ⬜ Fail |
| Ready State | `=== Ready for Recording ===` | _____________ | ⬜ Pass ⬜ Fail |

**Boot Time Measurement:**
- Time from reset to "Ready for Recording": _____________ seconds
- Expected: < 5 seconds per reasonable startup time

**Error Conditions to Check:**
- [ ] SD card missing: Should show storage error but continue
- [ ] Microphone disconnected: Should show ADC warning but continue
- [ ] Button/LED disconnected: Should show GPIO error

### ST-004: Basic GPIO Functionality 
**Objective**: Validate button input and LED output work correctly

| Test Step | Expected Behavior | Actual Behavior | Status |
|-----------|-------------------|-----------------|---------|
| Initial LED state | LED OFF (idle state) | _____________ | ⬜ Pass ⬜ Fail |
| Button press detection | Serial: `Button pressed - current state: 0` | _____________ | ⬜ Pass ⬜ Fail |
| LED response time | LED ON within 100ms of button press | _____________ | ⬜ Pass ⬜ Fail |
| Button debouncing | No multiple triggers from single press | _____________ | ⬜ Pass ⬜ Fail |

**Button Response Timing:**
- Measured response time: _____________ ms
- Expected: < 100ms per responsive UI requirement

### ST-005: Recording Session Workflow (PRD FR1 Validation)
**Objective**: Test complete 10-second recording cycle per PRD requirements

**Test Procedure:**
1. Ensure system in idle state (LED OFF)
2. Press button once
3. Observe 10-second recording cycle
4. Verify automatic stop

| Test Phase | Expected Behavior | Actual Behavior | Status |
|------------|-------------------|-----------------|---------|
| Recording Start | Serial: `Starting 10-second recording session #1`<br>LED: ON immediately | _____________ | ⬜ Pass ⬜ Fail |
| Recording Active | Serial: `Recording started - auto-stop in 10 seconds`<br>LED: Solid ON | _____________ | ⬜ Pass ⬜ Fail |
| 10-Second Timeout | Serial: `10-second recording timeout reached`<br>LED: Turns OFF | _____________ | ⬜ Pass ⬜ Fail |
| Session Complete | Serial: `Recording session #1 complete: XXXX bytes, 10000 ms` | _____________ | ⬜ Pass ⬜ Fail |
| Return to Idle | Serial: Shows ready for next recording<br>LED: OFF | _____________ | ⬜ Pass ⬜ Fail |

**Timing Measurements:**
- Actual recording duration: _____________ seconds
- Expected: 10.0 ± 0.1 seconds
- Button to LED response: _____________ ms
- Stop command to LED off: _____________ ms

### ST-006: Manual Recording Stop
**Objective**: Test manual stop functionality during recording

**Test Procedure:**
1. Start recording with button press
2. After 3-5 seconds, press button again
3. Verify manual stop works

| Test Step | Expected Behavior | Actual Behavior | Status |
|-----------|-------------------|-----------------|---------|
| Second button press | Serial: `Manual stop requested during recording` | _____________ | ⬜ Pass ⬜ Fail |
| Recording stops | LED turns OFF, recording ends | _____________ | ⬜ Pass ⬜ Fail |
| Duration tracking | Reports actual duration (< 10 seconds) | _____________ | ⬜ Pass ⬜ Fail |

**Manual Stop Timing:**
- Time from second press to stop: _____________ ms
- Expected: < 500ms

## Audio System Validation

### ST-007: Microphone ADC Readings (Hardware Validation)
**Objective**: Verify microphone ADC channels are receiving audio signals

**Test Procedure:**
1. Ensure quiet environment
2. Start recording
3. Speak loudly near microphones
4. Observe ADC readings in serial output

| Microphone | Silence Reading | Speech Reading | Signal Range | Status |
|------------|----------------|----------------|--------------|---------|
| MIC1 (GPIO 9) | _____________ | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| MIC2 (GPIO 12) | _____________ | _____________ | _____________ | ⬜ Pass ⬜ Fail |

**Expected Values:**
- Silence: ~2048 ± 100 (12-bit ADC center)
- Speech: Significant deviation from 2048 (>200 ADC counts)
- Range: 0-4095 (12-bit ADC full scale)

**Audio Signal Quality Checks:**
- [ ] MIC1 responds to speech near left side
- [ ] MIC2 responds to speech near right side  
- [ ] Both microphones show correlated but independent signals
- [ ] No clipping (values not stuck at 0 or 4095)

### ST-008: WAV File Generation (PRD FR2 Validation)
**Objective**: Verify audio files are created with correct format per PRD NFR2

**Test Procedure:**
1. Complete at least 3 recording sessions
2. Remove SD card and examine files on computer
3. Analyze file format and content

| File Check | Expected Result | Actual Result | Status |
|------------|----------------|---------------|---------|
| File creation | Files appear in `/sdcard/rec/` directory | _____________ | ⬜ Pass ⬜ Fail |
| File naming | Sequential: `recording_001.wav`, `recording_002.wav` | _____________ | ⬜ Pass ⬜ Fail |
| File size | ~320KB for 10-second stereo recording | _____________ | ⬜ Pass ⬜ Fail |
| Sample rate | 16000 Hz | _____________ | ⬜ Pass ⬜ Fail |
| Bit depth | 16-bit | _____________ | ⬜ Pass ⬜ Fail |
| Channels | 2 (stereo) | _____________ | ⬜ Pass ⬜ Fail |
| Format | PCM WAV | _____________ | ⬜ Pass ⬜ Fail |

**File Analysis Commands:**
```bash
# On computer with SD card:
ls -la /Volumes/SDCARD/rec/          # Check file listing
file recording_001.wav               # Verify file type
ffprobe recording_001.wav           # Audio format analysis
hexdump -C recording_001.wav | head # WAV header inspection
```

**File Analysis Results:**
- Total files created: _____________
- Average file size: _____________ bytes
- WAV header validation: ⬜ Pass ⬜ Fail
- Audio playback test: ⬜ Pass ⬜ Fail

### ST-009: Audio Quality Assessment
**Objective**: Validate recorded audio meets coaching review requirements

**Test Procedure:**
1. Record speech at various distances (1ft, 3ft, 6ft)
2. Record in different environments (quiet, moderate noise)
3. Analyze audio quality on computer

| Test Condition | Speech Intelligibility | Background Noise | Overall Quality | Status |
|----------------|----------------------|------------------|-----------------|---------|
| 1 foot distance | _____________ | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| 3 feet distance | _____________ | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| 6 feet distance | _____________ | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| Quiet room | _____________ | _____________ | _____________ | ⬜ Pass ⬜ Fail |
| Normal room noise | _____________ | _____________ | _____________ | ⬜ Pass ⬜ Fail |

**Quality Scale:** 1=Poor, 2=Fair, 3=Good, 4=Very Good, 5=Excellent
**Target:** ≥3 for coaching review requirements

## Error Handling and Recovery

### ST-010: SD Card Error Handling
**Objective**: Test system behavior when SD card is unavailable

| Test Scenario | Expected Behavior | Actual Behavior | Status |
|---------------|-------------------|-----------------|---------|
| No SD card inserted | Error message, LED indicates error | _____________ | ⬜ Pass ⬜ Fail |
| SD card removed during recording | Recording stops gracefully | _____________ | ⬜ Pass ⬜ Fail |
| SD card full | Storage full error, recording prevented | _____________ | ⬜ Pass ⬜ Fail |
| Corrupted SD card | Error detection and reporting | _____________ | ⬜ Pass ⬜ Fail |

### ST-011: System Recovery Testing
**Objective**: Validate system recovery from error states

| Recovery Test | Expected Behavior | Actual Behavior | Status |
|---------------|-------------------|-----------------|---------|
| Button press in error state | Attempts system reset to idle | _____________ | ⬜ Pass ⬜ Fail |
| Power cycle recovery | Clean restart, no corrupted state | _____________ | ⬜ Pass ⬜ Fail |
| Recording timeout during error | Graceful cleanup and state reset | _____________ | ⬜ Pass ⬜ Fail |

## Performance and Timing

### ST-012: System Performance Metrics
**Objective**: Measure system performance against PRD requirements

| Performance Metric | Target Value | Measured Value | Status |
|-------------------|--------------|----------------|---------|
| Boot time | < 5 seconds | _____________ | ⬜ Pass ⬜ Fail |
| Button response time | < 100ms | _____________ | ⬜ Pass ⬜ Fail |
| Recording start latency | < 500ms | _____________ | ⬜ Pass ⬜ Fail |
| Recording stop latency | < 500ms | _____________ | ⬜ Pass ⬜ Fail |
| 10-second timing accuracy | ±100ms | _____________ | ⬜ Pass ⬜ Fail |
| Memory usage (heap) | < 80% of available | _____________ | ⬜ Pass ⬜ Fail |

**Performance Monitoring Commands:**
```bash
# In idf.py monitor:
# Look for heap usage reports
# Monitor task stack usage
# Check for memory leaks over multiple recording cycles
```

### ST-013: Multiple Recording Sessions
**Objective**: Test system stability over multiple recording cycles

**Test Procedure:** Complete 10 consecutive recording sessions

| Session # | Start Time | Duration | File Size | LED Response | Status |
|-----------|------------|----------|-----------|--------------|---------|
| 1 | _________ | _________ | _________ | _________ | ⬜ Pass ⬜ Fail |
| 2 | _________ | _________ | _________ | _________ | ⬜ Pass ⬜ Fail |
| 3 | _________ | _________ | _________ | _________ | ⬜ Pass ⬜ Fail |
| 4 | _________ | _________ | _________ | _________ | ⬜ Pass ⬜ Fail |
| 5 | _________ | _________ | _________ | _________ | ⬜ Pass ⬜ Fail |
| 6 | _________ | _________ | _________ | _________ | ⬜ Pass ⬜ Fail |
| 7 | _________ | _________ | _________ | _________ | ⬜ Pass ⬜ Fail |
| 8 | _________ | _________ | _________ | _________ | ⬜ Pass ⬜ Fail |
| 9 | _________ | _________ | _________ | _________ | ⬜ Pass ⬜ Fail |
| 10 | _________ | _________ | _________ | _________ | ⬜ Pass ⬜ Fail |

**Stability Metrics:**
- Memory leaks detected: ⬜ Yes ⬜ No
- Performance degradation: ⬜ Yes ⬜ No  
- File system corruption: ⬜ Yes ⬜ No
- System crashes/resets: _____________ count

## Test Summary

### Overall Test Results

| Test Category | Tests Passed | Tests Failed | Pass Rate |
|---------------|-------------|-------------|-----------|
| Pre-Test Setup | ___/2 | ___/2 | ___% |
| Functional Testing | ___/6 | ___/6 | ___% |
| Audio System | ___/3 | ___/3 | ___% |
| Error Handling | ___/2 | ___/2 | ___% |
| Performance | ___/2 | ___/2 | ___% |
| **TOTAL** | **___/15** | **___/15** | **___%** |

### Phase 1 Completion Criteria

**✅ PASS CRITERIA (All must pass):**
- [ ] System boots and initializes without errors
- [ ] Button press triggers recording within 100ms
- [ ] 10-second recording duration accuracy (±100ms)
- [ ] WAV files created with correct format (16kHz, 16-bit, stereo)
- [ ] Audio quality suitable for speech recognition at 3-foot distance
- [ ] LED status indication works correctly
- [ ] System handles multiple recording sessions without degradation
- [ ] Basic error recovery from common failure modes

**🚫 CRITICAL FAILURES (Any of these fails the phase):**
- [ ] System doesn't boot or crashes during normal operation
- [ ] No audio recorded or files corrupted
- [ ] Recording duration significantly wrong (>1 second error)
- [ ] Hardware damage or safety issues

### Recommendations for Next Phase

**If Phase 1 PASSES:**
- [ ] Proceed to Phase 2: Data & Storage Enhancement
- [ ] Focus areas for improvement: _________________________________
- [ ] Performance optimizations needed: _________________________________

**If Phase 1 FAILS:**
- [ ] Critical issues to resolve: _________________________________
- [ ] Hardware modifications needed: _________________________________
- [ ] Software fixes required: _________________________________

### Test Completion

**Test Date:** _____________  
**Tester:** _____________  
**Hardware Revision:** _____________  
**Firmware Version:** _____________  

**Overall Assessment:** ⬜ PASS ⬜ FAIL ⬜ CONDITIONAL PASS

**Notes and Observations:**
_________________________________________________________________________
_________________________________________________________________________
_________________________________________________________________________