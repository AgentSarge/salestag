# Project Brief: SalesTag

## Executive Summary

SalesTag is a wearable IoT device designed as a smart badge for door-to-door sales representatives, enabling automated recording and real-time upload of sales pitches via Bluetooth Low Energy. The device addresses the critical problem of sales training, performance analysis, and lead qualification by capturing authentic sales interactions without requiring complex user intervention. 

Built on ESP32-S3 Mini hardware with dual MAX9814 microphones, the device features intelligent voice activation, chunked recording with 10-second clips, and power-efficient sleep modes to support 5-8 hours of field operation. The primary value proposition is transforming sales team performance through data-driven insights, automated lead scoring, and real-time coaching capabilities delivered through a companion mobile application.

## Problem Statement

Door-to-door sales representatives face a critical disconnect between their field performance and organizational learning. Sales managers cannot observe actual customer interactions, leading to ineffective training programs based on role-playing rather than real-world scenarios. Representatives struggle to self-assess their pitch effectiveness, often repeating unsuccessful approaches without feedback. 

Current solutions fail because smartphone recording apps require manual activation (often forgotten during high-pressure interactions), are unprofessional when visible to customers, drain battery life, and provide no real-time analysis. The result is a $40 billion door-to-door sales industry operating with minimal data-driven optimization, where top performers' techniques remain undocumented and bottom performers receive generic training rather than personalized coaching.

The urgency stems from increasing customer skepticism toward door-to-door sales, requiring more sophisticated and personalized approaches that can only be developed through systematic analysis of successful interactions.

## Proposed Solution

SalesTag addresses the core problem of sales performance visibility through a **conversation-activated recording system** that balances automation with user control and legal compliance. When the device detects conversation-level audio using low-power voice activity detection (VAD), it begins recording while simultaneously prompting for verbal consent disclosure, ensuring transparency with customers and legal protection for organizations.

The solution's strategic differentiators position it as a **field conversation intelligence platform**: (1) **Professional Specialization** - Dedicated badge device eliminates smartphone distractions and maintains professional appearance during customer interactions, (2) **Coaching-First Architecture** - 10-second intelligent chunks enable progressive analysis focused on performance improvement rather than surveillance, and (3) **Compliance-as-Feature** - Built-in consent workflows and documented interactions provide legal protection while creating coaching opportunities.

This approach succeeds where smartphone apps fail by removing activation cognitive load through specialized hardware, providing transparent customer interactions that build trust rather than suspicion, and creating a scalable platform applicable beyond door-to-door to real estate, field services, and customer-facing roles. The solution transforms potential legal liability into competitive advantage through systematic documentation and continuous learning systems that benefit representatives, managers, and customers.

## Target Users

### Primary User Segment: Door-to-Door Sales Representatives

Field sales representatives aged 25-45 working for companies in solar installation, home security, telecommunications, and home improvement services. These professionals spend 6-8 hours daily visiting 15-30 prospects, earning commission-based income ranging from $40K-$120K annually. They currently use smartphones for basic CRM logging but lack systematic performance feedback.

Current behaviors include memorizing pitch scripts, adapting approaches based on intuition rather than data, and receiving coaching through role-playing sessions that don't reflect real customer objections. Their primary pain points are inconsistent performance without understanding why certain approaches work, difficulty remembering effective responses to common objections, and lack of objective feedback on communication effectiveness.

Their goals center on increasing conversion rates to boost income, developing confidence through validated successful techniques, and reducing the emotional toll of frequent rejection by understanding what works and what doesn't.

### Secondary User Segment: Sales Managers & Coaches

Regional and district sales managers (ages 35-55) responsible for teams of 5-25 field representatives, typically earning $80K-$150K base plus team performance bonuses. These professionals currently rely on self-reported metrics, occasional ride-alongs, and generic training programs to develop their teams. They spend significant time in reactive coaching sessions addressing problems after they occur rather than preventing them.

Their current workflow involves weekly one-on-ones reviewing call counts and conversion rates without insight into conversation quality, conducting expensive in-person coaching that covers limited scenarios, and struggling to scale successful techniques across diverse team members. Key pain points include inability to identify why top performers succeed, difficulty providing specific feedback without observational data, and challenges in creating personalized development plans for underperforming representatives.

Their primary goals focus on improving overall team conversion rates to meet revenue targets, reducing training costs through data-driven insights instead of trial-and-error coaching, and developing scalable systems that identify and replicate successful sales techniques across the entire organization.

## Goals & Success Metrics

### Business Objectives
- **Revenue Growth**: Increase average team conversion rates by 15-25% within 6 months of deployment
- **Training Efficiency**: Reduce new representative ramp-up time from 90 days to 60 days through data-driven coaching
- **Market Expansion**: Capture 5% market share in field conversation intelligence platform within 18 months
- **Customer Retention**: Achieve 85% annual renewal rate for enterprise clients through demonstrated ROI

### User Success Metrics
- **Representative Performance**: Individual conversion rate improvement of 20% or higher within 90 days
- **Coaching Effectiveness**: 80% of representatives report improved confidence and technique validation
- **Technology Adoption**: 90% daily usage rate among deployed representatives within 30 days
- **Data Quality**: 95% successful audio capture and upload rate during field operations

### Key Performance Indicators (KPIs)
- **Platform Usage**: Daily Active Devices (DAD) and monthly recording hours per device
- **Performance Impact**: Conversion rate delta (before/after) measured at individual and team levels
- **Customer Success**: Net Promoter Score (NPS) from both representatives and managers
- **Technical Reliability**: Device uptime percentage and successful BLE transfer rates
- **Business Growth**: Monthly Recurring Revenue (MRR) and Customer Lifetime Value (CLV)

## MVP Scope

### Core Features (Must Have)
- **Basic Recording System**: Button-activated voice recording with 10-second chunked storage to SD card for offline reliability
- **BLE Connectivity**: Bonded device pairing with priority queue file transfer to paired mobile device
- **Power Management**: Deep sleep modes between recordings with battery voltage monitoring and LED status indicators
- **Professional Form Factor**: Badge-style housing with discrete operation maintaining professional appearance during customer interactions
- **Mobile Companion App**: Basic audio playback, file management, and battery status display for representatives and managers

### Out of Scope for MVP
- Assembly AI backend integration for automated transcription and analysis
- Adaptive audio quality or dynamic chunk sizing based on connectivity
- Machine learning voice detection or keyword spotting capabilities
- Real-time coaching notifications or live analysis features
- Multi-device ecosystem integration with CRM systems
- Advanced analytics dashboard or performance benchmarking

### MVP Success Criteria
The MVP succeeds when field representatives can reliably record customer interactions without disrupting professional presentation, transfer recordings to mobile devices for review, and operate for full 8-hour shifts without technical intervention. Success is measured by 90% daily adoption among pilot users and consistent audio capture quality enabling manual review and coaching applications.

## Post-MVP Vision

### Phase 2 Features
Advanced voice detection capabilities including keyword spotting for automatic recording triggers, Assembly AI integration for real-time transcription and basic sentiment analysis, adaptive audio quality based on BLE connection strength, and expanded mobile app with conversation analytics and basic coaching insights. These features build on proven MVP functionality while adding intelligence layers.

### Long-term Vision
Transform SalesTag into the leading field conversation intelligence platform serving multiple industries beyond door-to-door sales. Within 12-18 months, the platform will offer real-time coaching suggestions, predictive lead scoring based on conversation patterns, and integration with major CRM systems. The vision includes expanding to real estate, field services, insurance, and customer support roles while maintaining the core professional, discrete operation that defines the platform.

### Expansion Opportunities
Geographic expansion to international markets with compliance frameworks for different privacy regulations, vertical-specific versions (RealEstateTag for property showings, ServiceTag for field technicians), enterprise analytics dashboard for sales organizations, and potential white-label licensing to sales training companies and CRM providers seeking conversation intelligence capabilities.

### Risk Mitigation Strategies Integrated into Vision

**Champion Dependency Solution**: Product positioning emphasizes "performance amplification" over "monitoring." Built-in privacy controls give representatives ownership of their data - they control which recordings get shared with managers and when. Coaching insights are delivered first to the individual, creating personal value before organizational benefit. Sales methodology targets rep champions who influence manager purchasing decisions.

**Data Storage Economics Solution**: Edge-first architecture processes audio locally on companion mobile app, uploading only compressed summaries and selected clips rather than raw audio. Tiered pricing based on value delivered (performance improvement) rather than storage consumed. Premium analytics features offset infrastructure costs while basic recording/playback remains cost-effective.

**Smartphone Displacement Defense**: Double down on "dedicated device advantage" - smartphones will ALWAYS be distraction devices with calls, texts, and notifications interrupting customer focus. Professional badge form factor becomes more important as smartphone capabilities improve, creating clear separation between "work focus tool" and "communication device." Patent defensive positioning around discrete professional recording devices.

## Technical Considerations

### Platform Requirements
- **Target Platform**: ESP32-S3 Mini with dual MAX9814 microphone amplifiers, microSD storage, USB-C charging, and BLE 5.0 connectivity
- **Mobile Support**: iOS 12+ and Android 8+ for companion app with BLE peripheral support
- **Performance Requirements**: 5-8 hour continuous operation, 10-second audio chunk processing, reliable BLE transfer within 30-foot range

### Technology Preferences
- **Firmware**: ESP-IDF framework with FreeRTOS for real-time audio processing and power management
- **Audio Processing**: 16kHz/16-bit PCM capture with hardware-accelerated compression
- **Mobile App**: React Native for cross-platform development with native BLE modules
- **Cloud Infrastructure**: AWS with S3 for storage and Lambda for serverless processing (Phase 2)

### Architecture Considerations
- **Power-First Design**: Deep sleep states between recordings, context-aware wake patterns, battery voltage monitoring via ADC
- **Offline-First Operations**: Local SD card storage with BLE opportunistic sync, ensuring functionality without connectivity
- **Security Framework**: AES-256 audio encryption, secure BLE pairing with bonded devices only, local data protection

## Constraints & Assumptions

### Constraints
- **Budget**: Hardware cost target under $150 per unit for viable B2B sales economics, development budget requires bootstrapped/lean approach with iterative releases
- **Timeline**: MVP delivery within 4-6 months to capture market opportunity before competitive entries, requiring focused scope and proven technology choices
- **Resources**: Small development team (2-3 engineers) necessitates simple, well-documented technology stack and minimal custom hardware development
- **Technical**: ESP32-S3 Mini processing limitations require efficient algorithms, BLE bandwidth constraints limit real-time features, battery capacity determines feature complexity

### Key Assumptions
- Door-to-door sales representatives will prioritize income improvement over privacy concerns when value is demonstrated
- Sales managers will invest in performance tools that provide measurable ROI within 6-month evaluation periods
- BLE connectivity will be reliable within residential environments for file transfer operations
- Legal framework variations can be managed through user consent workflows rather than region-specific hardware
- Mobile app development complexity is manageable with React Native cross-platform approach
- Edge processing capabilities of modern smartphones sufficient for basic audio analysis and compression

## Risks & Open Questions

### Key Risks
- **Champion Dependency**: User-buyer misalignment where sales managers prioritize surveillance over performance coaching, creating adoption resistance among representatives
- **Data Storage Economics**: Audio processing and storage costs scaling faster than subscription revenue, particularly with cloud-based analytics features
- **Battery Life Reality Gap**: ESP32-S3 power consumption with dual microphone processing may not achieve 5-8 hour target in real-world conditions
- **BLE Reliability in Field**: Residential interference and smartphone connectivity issues causing data transfer failures and user frustration
- **Regulatory Compliance Variations**: State-by-state recording consent laws creating legal liability and deployment complexity

### Open Questions
- What is the optimal audio quality vs. battery life trade-off for field deployment scenarios?
- How will customer reactions to recording disclosure affect sales conversion rates? (Note: 38 states follow one-party consent laws where recording participants don't require additional permission)
- Can BLE file transfer keep pace with audio generation during high-activity field days?
- What level of mobile app sophistication is required for initial user adoption and retention?
- How do we validate that conversation intelligence actually improves sales performance before building analytics features?

### Areas Needing Further Research
- **Power Consumption Testing**: Comprehensive battery life analysis with dual microphone operation under various usage patterns
- **Legal Compliance Framework**: State-by-state analysis of recording consent requirements for door-to-door sales scenarios
- **User Experience Validation**: Field testing of badge form factor, button interface, and professional appearance during customer interactions
- **Competitive Intelligence**: Analysis of existing sales training tools and conversation analytics platforms for positioning differentiation

## Appendices

### A. Initial Analysis Summary

Drawing from our comprehensive brainstorming session (documented in `docs/brainstorming-session-results.md`), key findings include:

**Technical Feasibility**: ESP32-S3 Mini hardware provides sufficient processing capability for dual-microphone audio capture, 10-second chunked recording, and BLE transfer operations. Power consumption analysis indicates 5-8 hour operation is achievable through intelligent sleep modes and context-aware processing.

**User Research Insights**: Door-to-door sales representatives identified inconsistent performance feedback as their primary pain point, with current role-playing training failing to address real customer objections. Sales managers confirmed inability to scale successful techniques across teams due to lack of conversation visibility.

**Market Analysis**: Field conversation intelligence represents an underserved market with existing solutions focused on inside sales or requiring smartphone apps that create customer-facing distractions. Professional badge form factor provides significant competitive differentiation.

**Legal Framework**: 38 states operate under one-party consent laws, significantly simplifying deployment complexity and eliminating mandatory customer disclosure requirements in most markets.

### B. Technical Documentation References

Primary stakeholder insights gathered during brainstorming sessions emphasized the critical importance of power efficiency, discrete professional operation, and coaching-focused positioning over surveillance applications. Technical constraints include incremental development approach, BLE-only connectivity, and cost targets under $150 per unit for B2B viability.

### C. Project Foundation

- SalesTag Hardware Documentation: `device_hardware_info/` directory containing ESP32-S3 schematics, BOM, and component specifications
- Brainstorming Session Results: `docs/brainstorming-session-results.md` with comprehensive feature analysis and prioritization
- ESP32-S3 Technical Reference Manual and MAX9814 microphone amplifier datasheets

## Next Steps

### Immediate Actions

1. **Hardware Prototype Development**: Build working proof-of-concept using ESP32-S3 Mini, dual MAX9814 microphones, and basic recording functionality to validate power consumption and audio quality assumptions

2. **Market Validation Research**: Conduct structured interviews with 10+ door-to-door sales representatives and 5+ sales managers to validate pain points, willingness to pay, and feature priorities identified in brainstorming

3. **Legal Compliance Review**: Engage legal counsel for comprehensive analysis of recording laws, liability considerations, and required consent frameworks across target markets

4. **Technical Architecture Validation**: Complete detailed power consumption calculations, BLE throughput testing, and mobile app development spike to validate 4-6 month MVP timeline

5. **Competitive Analysis**: Systematic review of existing sales training tools, conversation analytics platforms, and wearable recording devices to identify differentiation opportunities and threats

### Success Criteria for Validation Phase

**Technical Validation Success:**
- Hardware prototype achieves minimum 6-hour battery life with continuous dual-microphone operation
- BLE file transfer successfully handles 10-second audio chunks with <5% failure rate in residential environments  
- Audio quality meets professional coaching requirements (clear speech recognition for human review)
- Total hardware cost remains under $120 per unit at 1000+ unit volumes

**Market Validation Success:**
- 70%+ of interviewed sales representatives confirm performance feedback as top-3 pain point
- 60%+ express willingness to pay $50+ monthly for demonstrated performance improvement
- 80%+ of sales managers confirm current training inefficiency problems and budget availability
- At least 2 pilot customers commit to 30-day field trials with 10+ representatives each

**Risk Mitigation Validation:**
- Champion dependency: Representatives demonstrate enthusiasm for personal performance data ownership
- Legal compliance: Zero legal concerns identified in target markets after professional review
- Data economics: Edge processing approach reduces cloud costs to <20% of subscription revenue

**Go/No-Go Decision Criteria:**
Proceed to PRD development only if 80% of technical criteria and 70% of market criteria are met. If validation fails, pivot to alternative markets (real estate, field services) or reconsider smartphone app approach with differentiated value proposition.

### PM Handoff

This Project Brief provides the full context for SalesTag based on initial technical analysis and strategic brainstorming. The next phase requires transitioning to 'PRD Generation Mode' where detailed product requirements, user stories, and technical specifications are developed through systematic validation of the assumptions and hypotheses outlined in this brief.

The Product Manager should focus on validating the champion dependency risk mitigation strategy, conducting real user research to confirm the pain points and value propositions identified, and working with the technical team to prove the core hardware and software feasibility before proceeding with full development planning.