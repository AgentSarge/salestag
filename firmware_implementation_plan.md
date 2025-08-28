# 🎯 Firmware Implementation Plan - BLE Corruption Fix

## **EXECUTIVE SUMMARY**
Comprehensive plan to eliminate BLE data corruption with exact packet specs, validation checks, and code snippets for immediate implementation.

---

## **1. TARGET PACKET FORMAT FOR GATT FILE DATA**

### **New BLE Chunk Structure**
```c
struct __attribute__((packed)) BleChunk {
  uint16_t proto_ver;        // 0x0001
  uint16_t seq;              // increments by 1 per chunk
  uint32_t file_id;          // or rolling session id
  uint32_t offset;           // byte offset in file
  uint16_t payload_len;      // N bytes of data following
  uint16_t flags;            // 0=mid, 1=start, 2=end, 3=single
  // payload bytes [N]
  // crc32c of header+payload except this crc field
  uint32_t crc32c;           
};
```

### **Specifications:**
- **Endianness:** Little-endian for all fields
- **CRC:** CRC32C (Castagnoli) over everything from proto_ver through last payload byte
- **MTU:** Prefer 247. Effective notify payload = MTU - 3. With MTU 247, send up to 244 bytes per notification

---

## **2. CHARACTERISTICS AND ROLES**

### **BLE Service Structure:**
- **File Data:** Notify-only. Streams `BleChunk`
- **File Control:** Write-only. App requests retransmit for specific `seq` or `offset`
- **File Status:** Notify-only. Device raises counters and error codes

### **Control Messages:**
```c
CMD_START {file_id}
CMD_RESEND {file_id, seq}
CMD_STOP {file_id}
```

---

## **3. SANITY CHECKS (CRITICAL - IMPLEMENT FIRST)**

### **Pre-Transmission Validation:**
```c
// Header magic validation
if (header_magic != 0x52415741) {
    ESP_LOGE(TAG, "Invalid RAW header: 0x%08X", header_magic);
    integrity_counters.header_magic_mismatch++;
    return ESP_ERR_INVALID_ARG;
}

// ADC range validation  
if (mic_sample > 4095) {
    ESP_LOGW(TAG, "ADC out of range: %d (max 4095)", mic_sample);
    integrity_counters.adc_oob_count++;
    return ESP_ERR_INVALID_ARG;
}

// 0xFFFF detection (corruption indicator)
if (mic_sample == 0xFFFF) {
    ESP_LOGW(TAG, "Detected 0xFFFF corruption");
    integrity_counters.ffff_count++;
    return ESP_ERR_INVALID_ARG;
}

// Timestamp monotonicity
if (timestamp_ms < last_timestamp_ms) {
    ESP_LOGW(TAG, "Timestamp regression: %lu -> %lu", last_timestamp_ms, timestamp_ms);
    integrity_counters.timestamp_gaps++;
}

// Sample count continuity
if (sample_count != expected_sample_count) {
    ESP_LOGW(TAG, "Sample count gap: expected %lu, got %lu", expected_sample_count, sample_count);
    integrity_counters.seq_gaps_detected++;
}
```

---

## **4. DEVICE-SIDE INTEGRITY COUNTERS**

### **Status Monitoring Structure:**
```c
typedef struct {
    uint32_t chunks_sent;
    uint32_t chunks_resent;
    uint32_t crc_fail_local;      // before sending
    uint32_t crc_fail_remote;     // from app resend requests
    uint32_t seq_gaps_detected;
    uint32_t adc_oob_count;       // samples outside 0..4095
    uint32_t ffff_count;          // samples equal to 0xFFFF
    uint32_t header_magic_mismatch;
    uint32_t timestamp_gaps;
} integrity_counters_t;

static integrity_counters_t integrity_counters = {0};
```

---

## **5. MTU, CONNECTION, AND TIMING**

### **ESP-IDF NimBLE Configuration:**
```c
void ble_init(void) {
    // Negotiate MTU early
    ble_att_set_preferred_mtu(247);
}

// In GAP connect event, check final MTU
static int gap_event(struct ble_gap_event *ev, void *arg) {
    if (ev->type == BLE_GAP_EVENT_MTU) {
        ESP_LOGI(TAG, "Negotiated MTU: %d", ev->mtu.value);
        // Adjust packet size based on MTU
    }
    return 0;
}
```

### **Backpressure Management:**
- **Target throughput:** 15-60 kB/s (realistic on most phones)
- **Notification rate:** 30-120 notifications/second with 244-byte payloads
- **Queue depth:** Maximum 2-4 notifications in flight

---

## **6. RETRANSMIT FLOW**

### **Error Recovery Protocol:**
1. App validates CRC32C per chunk
2. If bad/missing `seq`, app writes `CMD_RESEND {file_id, seq}`
3. Device resends chunk using same `seq` and `offset`
4. After 3 consecutive failures for same `seq`, device aborts stream

---

## **7. RAW HEADER CORRECTNESS**

### **Safe Little-Endian Writes:**
```c
static void write_u32_le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v);
    p[1] = (uint8_t)(v >> 8);
    p[2] = (uint8_t)(v >> 16);
    p[3] = (uint8_t)(v >> 24);
}

void raw_header_write(uint8_t *buf, uint32_t total_samples, uint32_t start_ms, uint32_t end_ms) {
    write_u32_le(buf + 0,  0x52415741);  // "RAWA"
    write_u32_le(buf + 4,  1);           // version
    write_u32_le(buf + 8,  16000);       // sample_rate
    write_u32_le(buf + 12, total_samples);
    write_u32_le(buf + 16, start_ms);
    write_u32_le(buf + 20, end_ms);
    for (int i = 0; i < 4; ++i) write_u32_le(buf + 24 + 4*i, 0); // reserved
}
```

### **Header Validation:**
```c
// Expected bytes: 41 57 41 52 for "RAWA" in little-endian
uint8_t expected_magic[] = {0x41, 0x57, 0x41, 0x52};
if (memcmp(header_bytes, expected_magic, 4) != 0) {
    ESP_LOGE(TAG, "Header corruption detected");
    return ESP_ERR_INVALID_ARG;
}
```

---

## **8. KNOWN PITFALLS & SOLUTIONS**

### **"AWAR" instead of "RAWA":**
- **Cause:** Endianness swap or reversed memcpy
- **Fix:** Write bytes explicitly, use `__attribute__((packed))` structs

### **0xFFFF values:**
- **Cause:** Uninitialized memory, buffer overwrite
- **Fix:** Add guard that clamps to 4095, increment `ffff_count`

### **1.6 billion ms timestamps:**
- **Cause:** Struct misalignment, wrong offset reads
- **Fix:** Use `static_assert(sizeof(raw_audio_sample_t) == 10, "bad pack")`

---

## **9. IMPLEMENTATION SNIPPETS**

### **Packed Structs with Static Asserts:**
```c
typedef struct __attribute__((packed)) {
    uint32_t magic_number;    // 0x52415741
    uint32_t version;         // 1
    uint32_t sample_rate;     // 16000
    uint32_t total_samples;
    uint32_t start_timestamp;
    uint32_t end_timestamp;
    uint32_t reserved[4];
} raw_header_t;

_Static_assert(sizeof(raw_header_t) == 32, "RAW header must be 32 bytes");

typedef struct __attribute__((packed)) {
    uint16_t mic_sample;      // 0..4095
    uint32_t timestamp_ms;
    uint32_t sample_count;
} raw_sample_t;

_Static_assert(sizeof(raw_sample_t) == 10, "Sample must be 10 bytes");
```

### **CRC32C Implementation:**
```c
uint32_t crc32c_update(uint32_t crc, const uint8_t *data, size_t len);

uint32_t ble_chunk_crc(const uint8_t *hdr, size_t hdr_len,
                       const uint8_t *payload, size_t n) {
    uint32_t crc = 0xFFFFFFFFu;
    crc = crc32c_update(crc, hdr, hdr_len);
    crc = crc32c_update(crc, payload, n);
    return ~crc;
}
```

### **Send Loop with Backpressure:**
```c
// Only send next notify when previous confirms completion
// Throttle to 50-100 notifications per second
// Keep small queue of pending chunks keyed by `seq` for resends
```

---

## **10. APP-SIDE VALIDATION CHECKLIST**

- ✅ Validate RAW header bytes: `41 57 41 52` at offset 0
- ✅ Iterate samples in 10-byte steps only
- ✅ Reject chunks with bad CRC, issue `CMD_RESEND`
- ✅ Reject duplicate `seq` unless CRC matches (harmless resend)
- ✅ Count out-of-range ADC values and 0xFFFF occurrences

---

## **11. QUICK REPRODUCTION TEST**

### **3-Second Test Protocol:**
1. Generate 3 seconds of test data, write to SD
2. Compute CRC32C of first 4KB on-device, print result
3. Transfer same bytes over BLE with new packet format
4. App computes CRC32C of reassembled 4KB
5. **Results must match** - if not, diff by `seq` to find corrupt packet

---

## **🎯 IMPLEMENTATION PRIORITY**

### **Phase 1 (Immediate - 1-2 days):**
1. Add ADC value validation (0-4095 check)
2. Add 0xFFFF detection and counting
3. Add RAW header magic validation

### **Phase 2 (Short-term - 3-5 days):**
1. Implement new BLE packet format with CRC32C
2. Add integrity counters and monitoring
3. Implement MTU negotiation

### **Phase 3 (Medium-term - 1 week):**
1. Add retransmit protocol
2. Implement comprehensive testing
3. Performance optimization

---

**This plan provides exact specifications and code snippets to eliminate BLE corruption systematically. Focus on Phase 1 validation checks first - they will immediately identify where corruption originates.**
