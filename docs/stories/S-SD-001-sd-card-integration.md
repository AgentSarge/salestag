# S-SD-001: SD Card Integration and WAV Recording

## Epic

E0: Bringup and Controls

## Story Type

Feature

## Priority

High

## Summary

Integrate SD card storage to enable WAV file recording, completing the R0 button-to-record functionality.

## User Story

As a user, I want to press the button to start recording audio, see the LED turn on, press again to stop, and have a WAV file saved to the SD card that I can access.

## Acceptance Criteria

- **A1**: SD card mounts successfully at `/sdcard` on boot
- **A2**: `/sdcard/rec` directory created automatically if it doesn't exist
- **A3**: Button press starts recording and saves to `/sdcard/rec/rec_0001.wav`
- **A4**: Second button press stops recording and finalizes WAV file
- **A5**: WAV file is valid PCM format with correct header and non-zero length
- **A6**: LED remains on during entire recording period
- **A7**: Device handles SD card removal/insertion gracefully

## Tasks

- **T1**: Update storage module to use FATFS with SD card instead of SPIFFS
- **T2**: Configure correct SPI pins (GPIO35=MOSI, GPIO37=MISO, GPIO36=SCLK, GPIO39=CS)
- **T3**: Implement SD card mount/unmount with error handling
- **T4**: Create `/sdcard/rec` directory structure
- **T5**: Integrate WAV writer with SD card storage
- **T6**: Update recorder to use SD card path
- **T7**: Add SD card status monitoring and recovery
- **T8**: Test with actual SD card hardware

## Definition of Done

- SD card mounts successfully on boot
- Button press starts recording to SD card
- WAV file created and accessible
- All acceptance criteria met
- QA gate passes
- Evidence captured in docs/qa/assessments/

## Dependencies

- R0 bring-up complete (S-REC-001)
- Hardware SD card slot available
- ESP-IDF FATFS and SDMMC components

## Out of Scope

- Multiple file management
- File rotation
- BLE file transfer
- VOX triggering

## Risk Level

Medium - Hardware integration complexity

## Story Points

5

## Status

In Development - Phase 1 Complete, Phase 2 In Progress

## Planning Complete

- ✅ Story created and prioritized
- ✅ PRD updated with R1 scope
- ✅ Architecture updated with SD card strategy
- ✅ QA Risk Assessment completed (Risk Level: Medium-High)
- ✅ QA Test Design completed with comprehensive test plan
- ✅ All mitigations identified and documented
