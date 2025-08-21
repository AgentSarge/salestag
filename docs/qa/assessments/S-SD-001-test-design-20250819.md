# QA Test Design: S-SD-001 SD Card Integration

## Story Information

- **Story**: S-SD-001 SD Card Integration and WAV Recording
- **Epic**: E0 Bringup and Controls
- **Design Date**: 2025-08-19
- **Designer**: Quinn (Test Architect)

## Test Strategy Overview

Comprehensive testing of SD card integration covering hardware compatibility, data integrity, error handling, and user scenarios. Focus on validating all acceptance criteria and risk mitigations.

## Test Categories

### 1. Unit Tests

**Target**: Individual SD card and WAV writer functions
**Tools**: ESP-IDF unit test framework
**Coverage**:

- SD card mount/unmount functions
- Directory creation and validation
- WAV header generation and validation
- Error handling and recovery functions

**Test Cases**:

- `test_sd_mount_success`: Valid SD card mounts correctly
- `test_sd_mount_failure`: Graceful handling of mount failures
- `test_directory_creation`: `/sdcard/rec` directory created automatically
- `test_wav_header_validity`: WAV headers are properly formatted
- `test_error_recovery`: System recovers from SD card errors

### 2. Integration Tests

**Target**: End-to-end button-to-WAV pipeline
**Tools**: Hardware-in-loop testing, serial monitoring
**Coverage**:

- Complete recording workflow
- File system operations
- LED state management
- Error propagation

**Test Cases**:

- `test_button_to_wav_pipeline`: Complete recording cycle
- `test_multiple_recordings`: Multiple start/stop cycles
- `test_led_state_sync`: LED matches recording state
- `test_file_accessibility`: WAV files accessible after recording

### 3. Hardware Tests

**Target**: Physical SD card and hardware interactions
**Tools**: Multiple SD card types, power cycling, insertion/removal
**Coverage**:

- SD card compatibility
- Power loss scenarios
- Hardware stress conditions
- Environmental factors

**Test Cases**:

- `test_sd_card_compatibility`: Various card brands and capacities
- `test_power_loss_recovery`: Power loss during recording
- `test_card_removal`: Graceful handling of card removal
- `test_card_insertion`: Automatic detection and mounting
- `test_spi_timing`: SPI bus performance and reliability

### 4. Performance Tests

**Target**: System performance and resource usage
**Tools**: Power monitoring, timing measurements, memory analysis
**Coverage**:

- Mount latency
- Write performance
- Power consumption
- Memory usage

**Test Cases**:

- `test_mount_latency`: SD card ready within 2 seconds
- `test_write_throughput`: Sustained WAV file writing
- `test_power_consumption`: Power draw during SD operations
- `test_memory_usage`: Memory footprint of SD integration

## Test Execution Plan

### Phase 1: Development Testing

**Duration**: 2-3 days
**Focus**: Unit tests and basic integration
**Deliverables**: Core functionality working, basic error handling

**Test Sequence**:

1. SD card mount/unmount unit tests
2. Directory creation validation
3. Basic WAV writer integration
4. Button-to-WAV pipeline testing

### Phase 2: Hardware Validation

**Duration**: 1-2 days
**Focus**: Hardware compatibility and stress testing
**Deliverables**: Hardware compatibility matrix, stress test results

**Test Sequence**:

1. Multiple SD card type testing
2. Power loss scenario testing
3. Card insertion/removal testing
4. SPI bus performance validation

### Phase 3: System Integration

**Duration**: 1-2 days
**Focus**: End-to-end system validation
**Deliverables**: Complete system validation, performance metrics

**Test Sequence**:

1. Complete user workflow testing
2. Performance benchmarking
3. Error scenario validation
4. User experience validation

## Evidence Collection

### Required Evidence Files

1. **`docs/qa/assessments/logs/S-SD-001-build.txt`**: Build logs
2. **`docs/qa/assessments/logs/S-SD-001-monitor.txt`**: Device operation logs
3. **`docs/qa/assessments/samples/S-SD-001-first.wav`**: First WAV file from SD card
4. **`docs/qa/assessments/hardware/S-SD-001-sd-test-results.md`**: Hardware compatibility results

### Evidence Requirements

- **Build Evidence**: Successful compilation with SD card components
- **Runtime Evidence**: SD card mount logs, directory creation, WAV file creation
- **Hardware Evidence**: Multiple SD card compatibility, power loss recovery
- **Performance Evidence**: Mount latency, write performance, power consumption

## Acceptance Criteria Validation

### A1: SD Card Mount Success

**Test**: `test_sd_mount_success`
**Validation**: Mount logs show successful FATFS initialization
**Evidence**: Monitor logs showing `/sdcard` mount point

### A2: Directory Creation

**Test**: `test_directory_creation`
**Validation**: `/sdcard/rec` directory exists after boot
**Evidence**: Directory listing showing `/sdcard/rec` structure

### A3: Button-to-Record Pipeline

**Test**: `test_button_to_wav_pipeline`
**Validation**: Button press starts recording to SD card
**Evidence**: Monitor logs showing recording start and file creation

### A4: Recording Stop

**Test**: `test_recording_stop`
**Validation**: Second button press stops recording and finalizes WAV
**Evidence**: WAV file with proper headers and non-zero length

### A5: WAV File Validity

**Test**: `test_wav_file_validity`
**Validation**: WAV file is valid PCM format with correct headers
**Evidence**: WAV file plays correctly on desktop, headers verified

### A6: LED State Management

**Test**: `test_led_state_sync`
**Validation**: LED remains on during entire recording period
**Evidence**: Monitor logs showing LED_ON/LED_OFF timing

### A7: Error Handling

**Test**: `test_error_handling`
**Validation**: Device handles SD card issues gracefully
**Evidence**: Error logs and recovery behavior documented

## Risk Mitigation Validation

### High Risk Mitigations

1. **SD Card Mount Failures**: Test graceful fallback implementation
2. **SPI Bus Conflicts**: Validate SPI2_HOST configuration and timing
3. **WAV File Corruption**: Test power loss scenarios and file recovery

### Medium Risk Mitigations

1. **SD Card Compatibility**: Test with multiple card types and capacities
2. **User Error Handling**: Test card removal/insertion scenarios
3. **Power Management**: Monitor power consumption during SD operations

## Test Environment Requirements

### Hardware Requirements

- ESP32-S3 development board with SD card slot
- Multiple SD card types (Class 4, Class 10, UHS-I)
- Power supply with current monitoring capability
- USB serial connection for monitoring

### Software Requirements

- ESP-IDF v5.2 development environment
- WAV file analysis tools (ffprobe, hex editor)
- Power monitoring and logging tools
- Test automation framework

## Success Criteria

- All acceptance criteria validated with evidence
- All high-risk mitigations implemented and tested
- Performance requirements met (mount latency <2s, write throughput sustained)
- Error handling graceful and user-friendly
- Hardware compatibility across multiple SD card types

## Next Steps

1. **Development**: Implement SD card integration with error handling
2. **Unit Testing**: Run unit tests for core functionality
3. **Integration Testing**: Test button-to-WAV pipeline
4. **Hardware Validation**: Test with multiple SD card types
5. **Performance Testing**: Validate performance requirements
6. **Gate Review**: Complete QA review and gate decision
