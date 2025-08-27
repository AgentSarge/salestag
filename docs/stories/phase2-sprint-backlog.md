# Phase 2: Audio Recording - Sprint Backlog

## Sprint Overview

**Sprint Goal**: Transform the working SalesTag foundation (button + SD card) into a complete audio recording device

**Duration**: 3 weeks (August 26 - September 13, 2025)

**Success Criteria**: 
- Button press starts real audio recording
- Recording automatically stops after 10 seconds
- WAV files contain recognizable speech audio
- No regression in existing functionality

## Sprint Stories

### Week 1: Core Audio Implementation

#### Story 2.1: ADC Audio Capture Implementation
- **Priority**: Critical
- **Estimated**: 16 hours
- **Dependencies**: None (starts from working foundation)
- **Deliverable**: Real audio capture from dual microphones

**Key Tasks**:
- Replace audio_capture.c stub with real ADC continuous mode
- Configure dual microphone channels (16kHz sampling)
- Implement audio buffer management
- Integrate with existing button workflow

**Success Criteria**:
- ADC captures real audio instead of generating silence
- Dual microphone channels working simultaneously
- Audio data flows to existing WAV writing system

---

#### Story 2.2: Recording State Machine Integration
- **Priority**: Critical  
- **Estimated**: 8 hours
- **Dependencies**: Coordinates with Story 2.1
- **Deliverable**: Professional recording workflow management

**Key Tasks**:
- Replace simple main.c with full state machine
- Implement IDLE → RECORDING → STOPPING → IDLE workflow
- Integrate with button handling and LED feedback
- Add error state handling

**Success Criteria**:
- Button press properly starts recording state
- State machine coordinates audio capture with file writing
- LED feedback reflects recording states

### Week 2: Integration and Enhancement

#### Story 2.3: 10-Second Recording Timer
- **Priority**: High
- **Estimated**: 4 hours
- **Dependencies**: Requires Story 2.2 (state machine)
- **Deliverable**: Automatic recording termination

**Key Tasks**:
- Implement FreeRTOS timer for 10-second duration
- Integrate timer with recording state machine
- Add timer cancellation and error handling
- Optional LED countdown feedback

**Success Criteria**:
- Recording automatically stops after exactly 10 seconds
- Timer properly triggers state transitions
- WAV files match expected 10-second duration

---

#### Story 2.4: Real Audio Data WAV Writing
- **Priority**: Critical
- **Estimated**: 8 hours  
- **Dependencies**: Requires Story 2.1 (audio capture)
- **Deliverable**: WAV files with actual recorded speech

**Key Tasks**:
- Modify WAV writer to accept real audio data
- Implement audio streaming to SD card
- Optimize audio quality and format compliance
- Validate file compatibility with audio players

**Success Criteria**:
- WAV files contain clear, recognizable speech
- Files compatible with VLC, Windows Media Player, etc.
- File size matches expected duration (~320KB for 10 seconds)

### Week 3: Quality Assurance and Integration

#### Story 2.5: Audio Quality Validation & Integration Testing
- **Priority**: High
- **Estimated**: 8 hours
- **Dependencies**: Requires all other Phase 2 stories complete
- **Deliverable**: Validated, production-ready recording system

**Key Tasks**:
- Comprehensive end-to-end workflow testing
- Audio quality validation and standards establishment
- Multi-recording reliability testing
- Error condition handling validation

**Success Criteria**:
- >98% success rate for button → audio file workflow
- Audio quality suitable for speech recognition
- System handles multiple recordings reliably
- All Phase 1 functionality preserved

## Sprint Capacity and Timeline

### Weekly Breakdown

**Week 1 (Aug 26-30)**:
- Focus: Core audio capture and state machine
- Stories: 2.1 (16h) + 2.2 (8h) = 24 hours
- Milestone: Real audio recording starts and stops on button press

**Week 2 (Sep 2-6)**:
- Focus: Timer and audio file integration
- Stories: 2.3 (4h) + 2.4 (8h) = 12 hours  
- Milestone: 10-second recordings with real audio in playable WAV files

**Week 3 (Sep 9-13)**:
- Focus: Quality validation and testing
- Stories: 2.5 (8h) = 8 hours
- Milestone: Production-ready audio recording system

**Total Effort**: 44 hours over 3 weeks

### Risk Management

**High Risk Items**:
- **ADC Implementation Complexity**: If ADC continuous mode proves difficult, may need additional time
- **Audio Quality Issues**: MAX9814 microphone integration may require audio tuning
- **State Machine Integration**: Coordinating multiple components may reveal timing issues

**Mitigation Strategies**:
- Maintain working diagnostic build for rollback capability
- Test each story independently before integration
- Preserve Phase 1 functionality throughout development
- Regular integration testing to catch issues early

### Definition of Done (Sprint Level)

Phase 2 Sprint is complete when:

**Functional Requirements**:
- [ ] Button press starts real audio recording (not silence)
- [ ] Recording automatically stops after 10 seconds
- [ ] WAV files contain recognizable speech audio
- [ ] Dual microphone channels captured in stereo
- [ ] Files play correctly in standard audio applications

**Quality Requirements**:
- [ ] >98% success rate for recording workflow
- [ ] No audio dropouts or corruption
- [ ] File size matches expected duration (320KB ±5%)
- [ ] System handles 10+ sequential recordings reliably

**Integration Requirements**:
- [ ] All Phase 1 functionality preserved (button, LED, SD card, file naming)
- [ ] Build system produces working firmware
- [ ] No memory leaks or resource issues
- [ ] Error conditions handled gracefully

**Documentation Requirements**:
- [ ] Testing procedures documented
- [ ] Quality standards established
- [ ] Integration patterns validated for Phase 3

## Story Dependencies

```
Foundation (Phase 1) ✅
    ↓
Story 2.1 (ADC Audio) → Story 2.4 (WAV Audio Data)
    ↓                        ↓
Story 2.2 (State Machine) → Story 2.5 (Validation)
    ↓
Story 2.3 (Timer) --------→ Story 2.5 (Validation)
```

**Critical Path**: Stories 2.1 → 2.2 → 2.3 must complete in sequence
**Parallel Work**: Story 2.4 can develop alongside 2.2/2.3 once 2.1 provides audio data

## Success Metrics

### Technical Metrics
- **Audio Capture Rate**: 16kHz stereo sampling achieved
- **Recording Accuracy**: 10-second duration ±100ms
- **File Size**: 320KB ±5% for 10-second recordings  
- **Success Rate**: >98% button press → valid audio file

### Quality Metrics
- **Speech Clarity**: Words clearly distinguishable at normal speaking volume
- **Audio Fidelity**: Suitable for sales interaction review
- **File Compatibility**: Opens correctly in VLC, Windows Media Player, QuickTime
- **System Reliability**: No crashes or hangs during recording

### Integration Metrics
- **Phase 1 Preservation**: All existing functionality continues working
- **Error Handling**: Graceful recovery from all tested error conditions
- **Resource Management**: No memory leaks over 20+ recordings
- **Build Compatibility**: Firmware builds and flashes without errors

This sprint backlog provides the complete roadmap for transforming your working SalesTag foundation into a fully functional audio recording device in 3 weeks, with concrete deliverables, success criteria, and risk management strategies.