# SalesTag Product Requirements Document (PRD)

## Goals and Background Context

### Goals
- Increase average team conversion rates by 15-25% within 6 months through data-driven sales coaching
- Reduce new representative ramp-up time from 90 days to 60 days via real-world interaction analysis  
- Achieve 90% daily adoption among field representatives through professional, discrete recording solution
- Enable systematic documentation and scaling of successful sales techniques across organizations
- Transform sales training from role-playing to real customer interaction-based coaching

### Background Context

Door-to-door sales representatives face a critical disconnect between field performance and organizational learning, with sales managers unable to observe actual customer interactions. This leads to ineffective training programs based on role-playing rather than real-world scenarios, where representatives struggle to self-assess pitch effectiveness and often repeat unsuccessful approaches without feedback.

SalesTag addresses this problem through a conversation-activated recording system built on ESP32-S3 hardware with dual microphones, enabling automated capture of sales interactions via a professional badge form factor. The solution transforms the $40 billion door-to-door sales industry's training approach from intuition-based to data-driven, providing both representatives and managers with authentic interaction analysis for performance improvement.

### Change Log
| Date | Version | Description | Author |
|------|---------|-------------|---------|
| 2025-08-21 | 1.0 | Initial PRD creation from Project Brief | John (PM) |

## Requirements

### Functional Requirements

1. **FR1:** The device SHALL record audio in 10-second chunks when activated via physical button press
2. **FR2:** The device SHALL store recorded audio chunks locally on microSD card with timestamp metadata
3. **FR3:** The device SHALL establish BLE 5.0 connection with bonded mobile devices within 30-foot range
4. **FR4:** The device SHALL transfer queued audio files to paired mobile device via BLE when connection is available
5. **FR5:** The device SHALL provide LED status indicators for recording state, battery level, and BLE connectivity
6. **FR6:** The device SHALL enter deep sleep mode between recording sessions to conserve battery
7. **FR7:** The mobile companion app SHALL display audio files with playback controls for review and sync with Supabase backend when online
8. **FR8:** The mobile companion app SHALL show real-time battery status and device connectivity state
9. **FR9:** The device SHALL monitor battery voltage and provide low battery warnings via LED indicator
10. **FR10:** The system SHALL maintain professional badge form factor suitable for customer-facing interactions
11. **FR11:** The device SHALL encrypt audio files using AES-256 before storage and transmission
12. **FR12:** The mobile app SHALL provide file management capabilities (delete, export, organize recordings) with offline-first operation and cloud sync

### Non-Functional Requirements

1. **NFR1:** The device SHALL operate continuously for minimum 5-8 hours on single battery charge under normal usage
2. **NFR2:** Audio recording quality SHALL be 16kHz/16-bit PCM suitable for clear speech recognition and coaching review
3. **NFR3:** BLE file transfer SHALL achieve >95% success rate for audio chunks under 1MB
4. **NFR4:** The device SHALL resume operation within 2 seconds from deep sleep when button is pressed
5. **NFR5:** Hardware cost SHALL remain under $150 per unit to maintain viable B2B economics
6. **NFR6:** The mobile app SHALL support iOS 12+ and Android 8+ devices with BLE peripheral capabilities
7. **NFR7:** Device SHALL withstand typical field conditions (temperature range -10°C to 50°C, light moisture resistance)
8. **NFR8:** System SHALL process and store audio locally without requiring internet connectivity for core recording functions, with optional Supabase sync when online
9. **NFR9:** BLE pairing SHALL use secure bonding with authentication to prevent unauthorized device access
10. **NFR10:** Device weight SHALL not exceed 100 grams to maintain comfortable all-day wearability

## User Interface Design Goals

### Overall UX Vision
The SalesTag ecosystem prioritizes **professional discretion** and **performance empowerment** through a coaching-first interface design. The physical device maintains minimal, status-only interaction (LED indicators, single button) to avoid customer distraction during sales interactions. The mobile companion app emphasizes **personal performance insights** over surveillance, positioning the representative as the primary beneficiary of their recorded interactions with optional sharing to managers.

### Key Interaction Paradigms
- **One-Touch Recording**: Single button press activates recording without complex menus or settings during field use
- **Passive Status Awareness**: LED indicators provide essential feedback (recording, battery, connectivity) without audio cues that could interrupt customer conversations
- **Coach-Athlete Relationship**: Mobile app interface frames managers as performance coaches rather than supervisors, with representatives controlling data sharing
- **Progressive Discovery**: App reveals insights gradually (basic playback → conversation highlights → performance patterns) to avoid overwhelming users

### Core Screens and Views
- **Device Status Dashboard**: Real-time battery, storage, and connectivity status with recording history
- **Recording Library**: Chronological list of audio sessions with timestamps, duration, and basic metadata
- **Playback Interface**: Audio player with waveform visualization, playback speed controls, and note-taking capability  
- **Performance Insights**: Individual conversion tracking, coaching notes, and improvement suggestions (Phase 2)
- **Sharing Controls**: Representative-controlled interface for selecting recordings to share with managers
- **Device Settings**: BLE pairing management, recording preferences, and battery optimization settings

### Accessibility: WCAG AA
Target WCAG AA compliance to support diverse sales representative populations, including larger touch targets for field use with gloves, high contrast modes for outdoor visibility, and voice-over compatibility for audio-focused workflows.

### Branding
Professional, trustworthy aesthetic emphasizing **performance enhancement** rather than monitoring. Clean, modern interface with coaching-oriented language ("Your Performance Hub", "Conversation Insights", "Growth Opportunities") and blue/green color palette suggesting growth and reliability. Avoid surveillance-associated red colors or intimidating corporate aesthetics.

### Target Device and Platforms: Web Responsive
Mobile-first design optimized for iOS and Android smartphones used by field representatives. Primary usage occurs on-the-go during breaks between customer visits, requiring thumb-friendly navigation and quick information access. Desktop web interface for managers provides expanded analytics and team overview capabilities (Phase 2).

## Data Architecture & Schema Requirements

### Audio File Metadata Structure

**Device Storage (JSON format on SD card):**
```json
{
  "file_id": "st_20250821_143022_001",
  "timestamp_utc": "2025-08-21T14:30:22.123Z",
  "timestamp_local": "2025-08-21T07:30:22.123-07:00",
  "duration_ms": 10000,
  "file_size_bytes": 320000,
  "audio_format": {
    "sample_rate": 16000,
    "bit_depth": 16,
    "channels": 2,
    "compression": "pcm"
  },
  "device_info": {
    "device_id": "ST-001A2B3C",
    "firmware_version": "1.0.0",
    "battery_level": 75
  },
  "transfer_status": "pending",
  "checksum_crc32": "a1b2c3d4"
}
```

### Device Pairing Data Structure

**ESP32-S3 Flash Storage (encrypted):**
```c
typedef struct {
    uint8_t bonded_device_mac[6];
    uint8_t bonded_device_irk[16];
    uint32_t pairing_timestamp;
    char device_name[32];
    uint8_t ltk[16];  // Long Term Key
    bool pairing_active;
    uint32_t last_connection;
} device_pairing_t;
```

**Mobile App Storage (Local SQLite + Supabase Sync):**
```sql
-- Local pairing data (SQLite - not synced to cloud for security)
CREATE TABLE local_paired_devices (
    device_id TEXT PRIMARY KEY,
    device_mac TEXT NOT NULL,
    device_name TEXT NOT NULL,
    pairing_timestamp INTEGER NOT NULL,
    last_connected INTEGER,
    is_active INTEGER DEFAULT 1,
    ltk_hash TEXT NOT NULL, -- Local only for security
    supabase_device_id TEXT, -- Reference to cloud record
    created_at INTEGER DEFAULT (strftime('%s','now'))
);
```

### Mobile App Database Schema

**Supabase PostgreSQL Tables (with offline SQLite sync):**
```sql
-- Users and authentication (managed by Supabase Auth)
CREATE TABLE profiles (
    id UUID REFERENCES auth.users(id) PRIMARY KEY,
    email TEXT UNIQUE NOT NULL,
    full_name TEXT,
    organization_id UUID REFERENCES organizations(id),
    role TEXT DEFAULT 'representative',
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- Organizations for team management
CREATE TABLE organizations (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name TEXT NOT NULL,
    subscription_tier TEXT DEFAULT 'basic',
    created_at TIMESTAMP DEFAULT NOW()
);

-- Audio recordings table (synced to local SQLite)
CREATE TABLE recordings (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    user_id UUID REFERENCES profiles(id) NOT NULL,
    file_id TEXT UNIQUE NOT NULL,
    device_id TEXT NOT NULL,
    timestamp_utc TIMESTAMP NOT NULL,
    timestamp_local TIMESTAMP NOT NULL,
    duration_ms INTEGER NOT NULL,
    file_size_bytes INTEGER NOT NULL,
    local_file_path TEXT,
    cloud_storage_url TEXT,
    transfer_status TEXT DEFAULT 'pending',
    sync_status TEXT DEFAULT 'local', -- 'local', 'syncing', 'synced'
    audio_quality JSONB,
    notes TEXT,
    tags TEXT[], -- PostgreSQL array type
    is_shared BOOLEAN DEFAULT FALSE,
    shared_with UUID[] DEFAULT ARRAY[]::UUID[],
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- User annotations and coaching data (synced to local SQLite)
CREATE TABLE recording_annotations (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    recording_id UUID REFERENCES recordings(id) NOT NULL,
    user_id UUID REFERENCES profiles(id) NOT NULL,
    timestamp_ms INTEGER NOT NULL, -- Position in audio
    annotation_type TEXT NOT NULL, -- 'note', 'bookmark', 'coaching_tip', 'manager_feedback'
    content TEXT NOT NULL,
    is_private BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- Device status and sync tracking
CREATE TABLE devices (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id TEXT UNIQUE NOT NULL,
    user_id UUID REFERENCES profiles(id) NOT NULL,
    device_name TEXT NOT NULL,
    last_sync_timestamp TIMESTAMP,
    pending_files_count INTEGER DEFAULT 0,
    total_recordings INTEGER DEFAULT 0,
    storage_used_bytes BIGINT DEFAULT 0,
    battery_level INTEGER,
    firmware_version TEXT,
    is_active BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- File transfer queue and status (local SQLite only)
CREATE TABLE transfer_queue (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id TEXT NOT NULL,
    file_id TEXT NOT NULL,
    priority INTEGER DEFAULT 0,
    retry_count INTEGER DEFAULT 0,
    last_attempt TIMESTAMP,
    error_message TEXT,
    status TEXT DEFAULT 'queued', -- 'queued', 'transferring', 'completed', 'failed'
    created_at TIMESTAMP DEFAULT NOW()
);

-- Row Level Security (RLS) Policies
ALTER TABLE recordings ENABLE ROW LEVEL SECURITY;
ALTER TABLE recording_annotations ENABLE ROW LEVEL SECURITY;
ALTER TABLE devices ENABLE ROW LEVEL SECURITY;

-- Users can only access their own recordings
CREATE POLICY "Users can view own recordings" ON recordings
    FOR SELECT USING (user_id = auth.uid());

-- Users can only access recordings shared with them
CREATE POLICY "Users can view shared recordings" ON recordings
    FOR SELECT USING (auth.uid() = ANY(shared_with));

-- Organizations can view team member recordings if explicitly shared
CREATE POLICY "Organization access to shared recordings" ON recordings
    FOR SELECT USING (
        EXISTS (
            SELECT 1 FROM profiles p1, profiles p2 
            WHERE p1.id = auth.uid() 
            AND p2.id = recordings.user_id
            AND p1.organization_id = p2.organization_id
            AND recordings.is_shared = TRUE
        )
    );
```

**Local SQLite Schema (Mobile App - Offline First):**
```sql
-- Simplified local schema that syncs with Supabase
CREATE TABLE local_recordings (
    id TEXT PRIMARY KEY, -- UUID from Supabase
    file_id TEXT UNIQUE NOT NULL,
    device_id TEXT NOT NULL,
    timestamp_utc TEXT NOT NULL,
    duration_ms INTEGER NOT NULL,
    local_file_path TEXT NOT NULL,
    sync_status TEXT DEFAULT 'pending', -- 'pending', 'synced', 'conflict'
    notes TEXT,
    tags TEXT, -- JSON array
    created_at TEXT DEFAULT (datetime('now')),
    last_modified TEXT DEFAULT (datetime('now'))
);

CREATE TABLE sync_queue (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    table_name TEXT NOT NULL,
    record_id TEXT NOT NULL,
    operation TEXT NOT NULL, -- 'INSERT', 'UPDATE', 'DELETE'
    data TEXT, -- JSON payload
    retry_count INTEGER DEFAULT 0,
    created_at TEXT DEFAULT (datetime('now'))
);
```

## Integration Testing Specifications

### Hardware-in-the-Loop (HIL) Testing Architecture

**Test Environment Setup:**
- ESP32-S3 development boards with identical hardware configuration as production
- Dedicated test mobile devices (iOS/Android) with known BLE capabilities
- Automated test harness controlling both device firmware and mobile app
- RF isolation chamber for consistent BLE signal testing
- Power supply monitoring for battery life validation

**BLE Communication Test Framework:**
```python
# Test automation framework structure
class BLEIntegrationTestSuite:
    def test_device_discovery_pairing(self):
        """Validate device advertising and secure pairing process"""
        # 1. Reset ESP32-S3 to unpaired state
        # 2. Start mobile app discovery
        # 3. Verify device appears in scan results
        # 4. Execute pairing with PIN verification
        # 5. Confirm bonding storage on both sides
        pass
    
    def test_file_transfer_reliability(self):
        """Validate chunked file transfer under various conditions"""
        # Test scenarios:
        # - 10-second audio files (baseline)
        # - Connection interruption and resume
        # - Multiple file queue processing
        # - Transfer during low battery conditions
        # - Signal strength variation (1m, 5m, 10m, 30m)
        pass
        
    def test_concurrent_operations(self):
        """Validate recording during BLE transfer"""
        # - Start file transfer
        # - Initiate new recording
        # - Verify recording quality unaffected
        # - Confirm transfer resumes after recording
        pass
```

**Automated Test Scenarios:**

1. **Connection Reliability Matrix:**
   - Distance: 1m, 5m, 10m, 15m, 30m
   - Interference: Clean RF, WiFi present, multiple BLE devices
   - Duration: 30 seconds, 5 minutes, 30 minutes continuous connection
   - Success criteria: >95% connection success rate at ≤15m

2. **File Transfer Validation:**
   - File sizes: 160KB (10s), 480KB (30s), 960KB (60s)
   - Transfer conditions: Optimal signal, marginal signal, interrupted signal
   - Retry mechanisms: Automatic resume, manual retry, queue reordering
   - Success criteria: >95% transfer success rate with <3 retry attempts

3. **Power Management Integration:**
   - Record → Transfer → Deep Sleep cycle validation
   - Battery level impact on BLE transmission power
   - Low battery behavior during critical operations
   - Wake-up latency under various sleep durations

**Continuous Integration Integration:**
```yaml
# GitHub Actions HIL Testing Pipeline
name: Hardware Integration Tests
on: [push, pull_request]

jobs:
  hil_tests:
    runs-on: [self-hosted, esp32-test-rig]
    steps:
      - name: Flash Test Firmware
        run: esptool.py write_flash 0x0 test_firmware.bin
      - name: Reset Test Environment  
        run: python reset_test_devices.py
      - name: Execute BLE Test Suite
        run: pytest tests/integration/ble_tests.py --junit-xml=results.xml
      - name: Power Consumption Validation
        run: python validate_power_profile.py
```

## Error Handling & Recovery Mechanisms

### BLE Connectivity Failure Scenarios

**1. Initial Pairing Failures:**
- **Scenario**: Device discovery timeout, pairing rejection, PIN mismatch
- **Device Behavior**: Continue advertising for 5 minutes, then deep sleep for 1 hour
- **Mobile App Behavior**: Show clear error messages, retry options, troubleshooting guidance
- **Recovery**: Manual retry button, device reset instructions, support contact

**2. Connection Loss During Transfer:**
- **Detection**: BLE connection timeout after 30 seconds of inactivity
- **Device Behavior**: Mark current transfer as "interrupted", maintain transfer queue state
- **Mobile App Behavior**: Show connection lost notification, background reconnection attempts
- **Recovery**: Automatic reconnection every 60 seconds for 10 minutes, resume transfer from last successful chunk

**3. File Transfer Corruption:**
- **Detection**: CRC32 checksum mismatch on received file
- **Device Behavior**: Retain original file, mark transfer as "retry_required"
- **Mobile App Behavior**: Request file retransmission, increment retry counter
- **Recovery**: Maximum 3 retry attempts, then mark as "manual_intervention_required"

**4. Storage Full Conditions:**
- **Device Storage**: Stop recording new audio, indicate storage full via LED pattern
- **Mobile Storage**: Show storage warning at 90% capacity, auto-cleanup oldest files
- **Recovery**: User-initiated cleanup, automatic cloud backup (Phase 2), SD card replacement

### Error State Recovery Matrix

```c
// ESP32-S3 Error State Machine
typedef enum {
    ERROR_NONE = 0,
    ERROR_BLE_PAIRING_FAILED,
    ERROR_BLE_CONNECTION_LOST,
    ERROR_TRANSFER_TIMEOUT,
    ERROR_STORAGE_FULL,
    ERROR_BATTERY_CRITICAL,
    ERROR_AUDIO_HARDWARE_FAULT,
    ERROR_FILESYSTEM_CORRUPT
} error_state_t;

typedef struct {
    error_state_t current_error;
    uint32_t error_timestamp;
    uint8_t retry_count;
    uint32_t recovery_action;
    bool user_intervention_required;
} error_context_t;
```

**Recovery Action Specifications:**

| Error State | Automatic Recovery | Manual Recovery | User Notification |
|-------------|-------------------|-----------------|-------------------|
| BLE Pairing Failed | Retry advertising (3x) | Device reset button | "Pairing failed - try again" |
| Connection Lost | Auto-reconnect (10x) | Force reconnect in app | "Connection lost - reconnecting" |
| Transfer Timeout | Resume transfer (3x) | Skip file, continue queue | "File transfer failed" |
| Storage Full | Stop recording | Delete old files | "Storage full - manage files" |
| Battery Critical | Deep sleep mode | Charge device | "Low battery - charge soon" |
| Audio Hardware | Restart audio subsystem | Device restart | "Audio error - restart device" |
| Filesystem Corrupt | Attempt repair | Format SD card | "Storage error - data may be lost" |

**User Experience During Error States:**

1. **Progressive Error Indication:**
   - **Level 1**: Subtle LED indication (yellow flash) - minor issues
   - **Level 2**: Persistent LED pattern (red blink) - attention needed  
   - **Level 3**: Mobile app notification + LED - immediate action required

2. **Error Recovery Workflows:**
   - **Quick Fix**: In-app troubleshooting with step-by-step guidance
   - **Advanced Recovery**: Device reset procedures, manual file management
   - **Support Escalation**: Error code reporting, log file export

3. **Graceful Degradation:**
   - **BLE Unavailable**: Continue local recording, queue for later transfer
   - **Mobile App Offline**: Device operates independently, sync when available
   - **Partial Transfer**: Resume from checkpoint, don't lose completed portions

## Supabase Integration Strategy

### Phase 1 (MVP) - Local-First Foundation
**Core Functionality**: All MVP features operate offline-first with local SQLite storage and BLE communication. Supabase integration provides:
- User authentication and profile management
- Device registration and basic cloud backup
- Foundation for team features in Phase 2

### Phase 2 - Team Collaboration & Analytics  
**Enhanced Features**: Leverage Supabase's real-time capabilities and Edge Functions:
- **Real-time sync**: Recording metadata sync across team devices
- **Edge Functions**: Audio transcription pipeline using AssemblyAI integration
- **Team management**: Manager dashboards with performance insights
- **Advanced storage**: Cloud audio storage with Supabase Storage + CDN

### Supabase Feature Utilization

**Database & Real-time:**
- PostgreSQL for structured data (recordings, annotations, teams)
- Real-time subscriptions for team collaboration features
- Row Level Security (RLS) for data privacy and access control

**Authentication:**
- Supabase Auth for user sign-up, login, and session management
- Social auth integration (Google, Microsoft) for enterprise adoption
- JWT tokens for secure mobile app authentication

**Storage:**
- Supabase Storage for cloud audio file backup and sharing
- CDN for fast audio streaming to management dashboards
- Automatic thumbnail generation for waveform previews

**Edge Functions:**
- Transcription processing pipeline triggered by audio uploads
- Performance analytics calculation (conversion rates, talk time analysis)
- Integration webhooks for CRM systems and sales tools
- Email notifications for coaching insights and team updates

### Offline-First Architecture Benefits

1. **Field Reliability**: Representatives never lose functionality due to poor connectivity
2. **Performance**: Local data access provides instant app responsiveness
3. **Cost Optimization**: Selective cloud sync reduces bandwidth and storage costs
4. **Privacy Control**: Sensitive recordings stay local unless explicitly shared
5. **Gradual Migration**: Phase 2 features enhance rather than replace local functionality

### Data Sync Strategy

**Bidirectional Sync Logic:**
```typescript
// Mobile app sync service
class SupabaseSync {
  async syncRecordings() {
    // 1. Upload new local recordings to Supabase
    const localRecordings = await getUnsyncedRecordings();
    for (const recording of localRecordings) {
      await this.uploadRecording(recording);
    }
    
    // 2. Download shared recordings from team members
    const sharedRecordings = await supabase
      .from('recordings')
      .select('*')
      .eq('is_shared', true)
      .neq('user_id', currentUserId);
      
    await this.storeSharedRecordings(sharedRecordings);
    
    // 3. Sync annotations and coaching feedback
    await this.syncAnnotations();
  }
}
```

**Conflict Resolution:**
- Last-write-wins for annotations and notes
- User confirmation required for recordings with conflicting metadata
- Automatic backup of conflicted versions to prevent data loss

## Technical Assumptions

### Repository Structure: Monorepo
Single repository containing ESP32-S3 firmware, React Native mobile app, and shared documentation. This approach simplifies development coordination for the small 2-3 engineer team while maintaining clear separation between embedded firmware and mobile application codebases through organized directory structure.

### Service Architecture
**Offline-First with Supabase Backend**: Core functionality operates entirely offline with ESP32-S3 device and mobile app handling recording, storage, and basic playback locally. Supabase provides PostgreSQL database with real-time subscriptions, authentication, and Edge Functions for serverless processing. This ensures field reliability while enabling seamless team collaboration and advanced analytics in Phase 2.

### Testing Requirements
**Unit + Integration Testing**: ESP32-S3 firmware requires hardware-in-the-loop testing for audio capture, BLE communication, and power management validation. React Native mobile app needs unit tests for audio processing logic and integration tests for BLE connectivity. Manual field testing essential for battery life, audio quality, and real-world usage scenarios given the physical device nature.

### Additional Technical Assumptions and Requests

**Hardware Platform:**
- ESP32-S3 Mini with dual-core processor for audio processing and BLE communication concurrency
- Dual MAX9814 microphone amplifiers for improved audio capture quality and directional sensitivity
- MicroSD card storage (minimum 32GB) for offline audio buffering and backup
- USB-C charging with battery voltage monitoring via built-in ADC
- Professional badge housing with button, LED indicators, and clip/lanyard attachment

**Firmware Framework:**
- ESP-IDF with FreeRTOS for real-time audio processing, power management, and BLE operations
- Hardware-accelerated audio compression to minimize storage requirements and BLE transfer times
- Deep sleep implementation with context-aware wake patterns based on usage history
- Secure boot and encrypted flash storage for device integrity and audio protection

**Mobile Application:**
- React Native for cross-platform iOS/Android development with native BLE modules
- Supabase client integration for offline-first data sync and authentication
- Local audio processing capabilities for compression and basic analysis
- SQLite local database with Supabase real-time sync for recording metadata and user preferences
- Native audio playback with waveform visualization and playback speed controls

**Connectivity & Security:**
- BLE 5.0 with secure pairing and AES-256 encryption for all audio transfers
- Bonded device architecture preventing unauthorized access to recordings
- Local-first data model with Supabase offline-first sync for cloud backup and team features
- Supabase Auth for user authentication and Row Level Security (RLS) for data protection
- Certificate-based device authentication for enterprise deployment security

**Development & Deployment:**
- Continuous integration for firmware builds with hardware testing automation where possible  
- Over-the-air (OTA) firmware update capability for field device maintenance
- Mobile app deployment through standard iOS App Store and Google Play Store channels
- Supabase Edge Functions for serverless backend processing (transcription, analytics)
- Enterprise device provisioning system for bulk deployment and configuration management
- Supabase database migrations and environment management for production deployment

## Epic List

**Epic 1: Foundation & Device Core** 
Establish ESP32-S3 firmware foundation with basic recording, storage, and power management while setting up development infrastructure and mobile app skeleton.

**Epic 2: BLE Communication & Mobile Integration**
Implement secure BLE pairing, file transfer protocol, and mobile companion app with audio playback and device management capabilities.

**Epic 3: Professional Deployment & Field Optimization**
Complete badge housing integration, battery optimization, field testing validation, and enterprise deployment preparation for pilot customer rollout.

## Epic Details

### Epic 1: Foundation & Device Core
Establish ESP32-S3 firmware foundation with basic recording, storage, and power management while setting up development infrastructure and mobile app skeleton. This epic delivers a standalone recording device capable of audio capture and local storage, providing immediate value for manual audio review while establishing the technical foundation for subsequent mobile integration.

#### Story 1.1: Project Infrastructure Setup
As a **developer**,
I want **established development environment with ESP-IDF toolchain, version control, and CI/CD pipeline**,
so that **the team can collaborate efficiently and deploy consistent firmware builds**.

**Acceptance Criteria:**
1. ESP-IDF development environment configured with toolchain version pinning
2. Git repository structure established with firmware, mobile app, and documentation directories
3. GitHub Actions CI pipeline builds and tests firmware for ESP32-S3 target
4. Documentation templates created for technical specifications and user guides
5. Development hardware procurement completed (ESP32-S3 dev boards, MAX9814 modules)

#### Story 1.2: Basic Audio Recording System
As a **sales representative**,
I want **to record audio by pressing a button on the device**,
so that **I can capture customer interactions without complex setup during field work**.

**Acceptance Criteria:**
1. Button press initiates audio recording using dual MAX9814 microphones
2. Audio captured at 16kHz/16-bit PCM quality suitable for speech recognition
3. Recording automatically stops after 10 seconds to create manageable chunks
4. LED indicator shows recording status (red during capture, green when complete)
5. Multiple consecutive recordings supported with timestamp differentiation

#### Story 1.3: Local Storage Management
As a **sales representative**,
I want **recorded audio stored reliably on the device**,
so that **my recordings are preserved even without mobile connectivity**.

**Acceptance Criteria:**
1. Audio files saved to microSD card with timestamp-based filenames
2. Storage capacity monitoring prevents overwriting when card approaches full capacity
3. File system corruption recovery ensures recordings remain accessible after power loss
4. Basic file metadata includes timestamp, duration, and audio quality parameters
5. Storage status indicated via LED patterns (blue blink for successful save)

#### Story 1.4: Power Management Foundation
As a **sales representative**,
I want **the device to operate efficiently throughout my workday**,
so that **I can rely on consistent recording capability during 8-hour field shifts**.

**Acceptance Criteria:**
1. Deep sleep mode activated automatically between recording sessions
2. Battery voltage monitoring via ADC with low battery warning (red LED flash)
3. Power consumption optimized for target 6+ hour continuous operation
4. USB-C charging circuit with charging status indication (orange LED during charge)
5. Wake-from-sleep functionality responds within 2 seconds of button press

#### Story 1.5: Mobile App Skeleton with Supabase Foundation
As a **sales representative**,
I want **a basic mobile companion app with user authentication**,
so that **I can securely access my recordings and prepare for team features**.

**Acceptance Criteria:**
1. React Native app scaffolding for iOS and Android with navigation structure
2. Supabase client integration with authentication flows (sign up, login, logout)
3. Local SQLite database setup with Supabase sync preparation
4. Device connection screen placeholder for BLE integration
5. Recording library screen layout with offline-first data display
6. Settings screen framework for device preferences and account management
7. App store deployment pipeline configured for beta testing distribution

### Epic 2: BLE Communication & Mobile Integration
Implement secure BLE pairing, file transfer protocol, and mobile companion app with audio playback and device management capabilities. This epic transforms the standalone device into an integrated system enabling representatives to review recordings, manage storage, and prepare for coaching applications.

#### Story 2.1: BLE Device Discovery and Pairing
As a **sales representative**,
I want **to securely pair my device with my mobile phone**,
so that **only I can access my recorded conversations**.

**Acceptance Criteria:**
1. ESP32-S3 advertises as "SalesTag" BLE peripheral with secure pairing requirements
2. Mobile app discovers and displays available SalesTag devices within range
3. Secure bonding process with PIN verification prevents unauthorized device access
4. Paired device information stored persistently on both device and mobile app
5. Connection status displayed via device LED and mobile app interface

#### Story 2.2: File Transfer Protocol
As a **sales representative**,
I want **my recordings automatically transferred to my phone when connected**,
so that **I can review conversations without manually handling SD cards**.

**Acceptance Criteria:**
1. BLE characteristic defined for audio file metadata exchange (filename, size, timestamp)
2. Chunked file transfer protocol handles audio files up to 1MB reliably
3. Transfer queue prioritizes newest recordings and resumes interrupted transfers
4. Progress indication shown on mobile app during file transfer operations
5. Automatic cleanup removes successfully transferred files from device storage

#### Story 2.3: Mobile Audio Playback
As a **sales representative**,
I want **to listen to my recorded conversations on my phone**,
so that **I can review my performance and identify improvement opportunities**.

**Acceptance Criteria:**
1. Audio files displayed in chronological list with timestamp and duration
2. Playback controls include play/pause, scrubbing, and variable speed (0.75x to 2x)
3. Waveform visualization helps identify conversation segments and silence gaps
4. Basic note-taking capability allows comments linked to specific recordings
5. Share functionality enables sending selected recordings via email or messaging

#### Story 2.4: Device Status Dashboard
As a **sales representative**,
I want **real-time visibility into my device status**,
so that **I can ensure reliable operation before important customer meetings**.

**Acceptance Criteria:**
1. Battery percentage displayed with estimated runtime remaining
2. Storage capacity shown as used/available space with recording time equivalent
3. BLE connection status and signal strength indicator
4. Last successful sync timestamp and pending file count
5. Device settings accessible for recording preferences and power management

#### Story 2.5: File Management and Organization
As a **sales representative**,
I want **to organize and manage my recorded conversations**,
so that **I can find specific interactions for review and coaching purposes**.

**Acceptance Criteria:**
1. Recordings sortable by date, duration, and custom tags
2. Search functionality filters recordings by date range and notes content
3. Bulk operations support selecting multiple recordings for export or deletion
4. Export options include email attachment, cloud storage upload, and local file save
5. Data retention settings automatically delete recordings older than specified period

### Epic 3: Professional Deployment & Field Optimization
Complete badge housing integration, battery optimization, field testing validation, and enterprise deployment preparation for pilot customer rollout. This epic delivers production-ready hardware and software suitable for professional door-to-door sales environments with enterprise deployment capabilities.

#### Story 3.1: Professional Badge Housing Integration
As a **sales representative**,
I want **a professional-looking badge device**,
so that **I can wear it confidently during customer interactions without appearing unprofessional**.

**Acceptance Criteria:**
1. Custom PCB designed for badge form factor with integrated ESP32-S3, microphones, and charging
2. Professional housing design with discrete button, LED indicators, and clip/lanyard attachment
3. Device weight under 100 grams for comfortable all-day wearing
4. Moisture resistance rating suitable for outdoor field conditions
5. Housing design allows clear audio capture without acoustic interference

#### Story 3.2: Battery Life Optimization and Validation
As a **sales representative**,
I want **reliable all-day battery performance**,
so that **I never miss recording opportunities due to dead device**.

**Acceptance Criteria:**
1. Real-world battery testing achieves minimum 6-hour continuous operation under field conditions
2. Adaptive power management adjusts sleep patterns based on usage history
3. Battery calibration provides accurate remaining runtime estimates
4. Low battery warnings begin at 20% capacity with increasing urgency indicators
5. Fast charging capability provides 50% battery recovery in 30 minutes via USB-C

#### Story 3.3: Field Testing and Quality Validation
As a **sales manager**,
I want **validated device performance in real sales environments**,
so that **I can confidently deploy the solution to my team without operational disruptions**.

**Acceptance Criteria:**
1. Field testing completed with 5+ sales representatives over 30-day pilot period
2. Audio quality validation confirms speech clarity for coaching review purposes
3. BLE reliability testing demonstrates >95% successful file transfer rate in residential environments
4. User experience feedback incorporated into final app interface and device operation
5. Deployment documentation created for onboarding new users and troubleshooting

#### Story 3.4: Enterprise Deployment and Device Management
As a **sales manager**,
I want **scalable deployment and management capabilities**,
so that **I can efficiently roll out devices to my entire sales team**.

**Acceptance Criteria:**
1. Bulk device provisioning system for configuring multiple SalesTags with organizational settings
2. Device management dashboard shows team device status, battery levels, and usage statistics
3. Over-the-air firmware update capability enables remote maintenance and feature updates
4. Enterprise security compliance including device encryption and access audit logging
5. Integration hooks prepared for future CRM system connectivity and team analytics

#### Story 3.5: Pilot Customer Preparation and Support
As a **product manager**,
I want **complete customer onboarding and support systems**,
so that **pilot customers can successfully deploy and realize value from SalesTag solution**.

**Acceptance Criteria:**
1. Comprehensive user documentation including quick start guide and troubleshooting reference
2. Training materials for representatives and managers on device operation and coaching applications
3. Customer support process established with technical escalation procedures
4. Success metrics tracking system for measuring conversion rate improvements
5. Pilot customer feedback collection system for informing post-MVP development priorities