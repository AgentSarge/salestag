# MVP Mobile App Brainstorming Session

**Session Date:** 2025-01-27
**Facilitator:** Business Analyst Mary
**Participant:** User
**Duration:** 30 minutes (maximum)
**Purpose:** Define comprehensive mobile app MVP for automated agent development

## Session Objective

Define the complete mobile app MVP requirements that will feed into the automated BMAD-method agent workflow for rapid development and deployment.

## MVP Definition Session - Starting Now

**Core Purpose Identified from Codebase:**

**Primary Problem:** Door-to-door sales representatives can't effectively review and improve their sales performance because managers can't observe actual customer interactions, leading to ineffective training based on role-playing rather than real scenarios.

**Single Core Function:** Enable sales representatives to **record, review, and manage their actual customer conversations** through a SalesTag smart badge device that automatically transfers audio to their mobile phone for private review and optional sharing with managers.

**Essential Value Proposition:** Transform sales training from guesswork to data-driven improvement by capturing real customer interactions for coaching analysis.

## MVP Mobile App Requirements - Based on PRD Analysis

**Target Users:** Door-to-door sales representatives who wear SalesTag smart badge devices

**Core Mobile App Functions:**
1. **BLE Device Management** - Pair with SalesTag badge, monitor battery/status
2. **Audio File Reception** - Receive 10-second audio chunks via BLE transfer
3. **Recording Library** - Chronological list of conversations with playback
4. **Audio Playback** - Play/pause, speed control, waveform visualization
5. **File Management** - Organize, search, tag, delete recordings
6. **Supabase Sync** - Cloud backup and team sharing capabilities

**Key Technical Requirements:**
- React Native (iOS + Android)
- BLE 5.0 connectivity with SalesTag device
- Local SQLite + Supabase cloud sync
- Professional UI suitable for field use
- Offline-first operation

## Next MVP Definition Questions:

**Question 2 - BLE Connection Experience - ✅ ANSWERED:**

**Selected:** ✅ **Simple One-Time Setup** - Pair once, auto-connect forever
**Rejected:** ❌ Manual Control - User chooses when to connect

**MVP BLE Experience:**
- One-time secure pairing with PIN verification during initial setup
- Automatic background connection whenever SalesTag device is in range
- Persistent connection state - no user intervention required after initial pairing
- Silent reconnection after temporary disconnections
- Connection status visible but not requiring user management

**Question 3 - Audio Features Priority - ✅ ANSWERED:**

**Selected:** ✅ **Minimal Day 1 Approach** - Ship fast, improve automatically

**🚀 Critical for MVP (Day 1):**
- ✅ Basic play/pause controls
- ✅ Audio file list with timestamps
- ✅ Basic file transfer and storage

**🔄 Nice-to-Have (Agents add later):**
- ❌ Waveform visualization → Later version
- ❌ Variable speed playback → Later version  
- ❌ Note-taking on recordings → Later version
- ❌ Search and filtering → Later version
- ❌ Recording sharing/export → Later version

**Question 4 - Device Status Information - ✅ ANSWERED:**

**Selected:** ✅ **Battery + Connection Critical** - Minimum viable status info

**🚀 Essential for MVP (Day 1):**
- ✅ Battery percentage/level (prevent lost recordings due to dead device)
- ✅ BLE connection status (confirm recordings are transferring)

**🔄 Nice-to-Have (Agents add later):**
- ❌ Storage space remaining → Later version
- ❌ Last sync time → Later version
- ❌ Recording count/history → Later version

**Question 5 - UI Structure - ✅ ANSWERED:**

**Selected:** ✅ **Single Screen Dashboard** - Everything on one screen

**MVP UI Structure:**
- ✅ Single screen with all functionality
- ✅ Battery/connection status at top
- ✅ Recording list with timestamps below
- ✅ Tap recording → inline player controls appear
- ✅ Zero navigation complexity
- ✅ Maximum simplicity for automated agent development

---

## 🎯 **MVP DEFINITION COMPLETE - 30 Minutes**

### Core SalesTag Mobile App MVP:

**Purpose:** Mobile companion app for SalesTag smart badge - record, review, and manage sales conversations

**Essential Features (Day 1):**
1. **One-time BLE pairing** with automatic reconnection
2. **Single screen dashboard** with status + recording list  
3. **Basic audio playback** (play/pause only)
4. **Battery and connection monitoring**
5. **Local file storage** with BLE transfer from device

**Technical Foundation:**
- React Native (iOS + Android)
- BLE 5.0 connectivity
- Local SQLite storage
- Supabase auth + cloud sync foundation
- Offline-first operation

**User Workflow:**
1. Pair SalesTag device once during setup
2. Device auto-connects when in range
3. Recordings transfer automatically via BLE
4. View recordings in chronological list
5. Tap to play with basic controls

**Development Strategy:**
- Build working prototype Day 1
- Agents enhance with advanced features later
- Minimal human intervention after MVP definition