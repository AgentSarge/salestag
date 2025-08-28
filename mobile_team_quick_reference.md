# 📱 Mobile Team Quick Reference - BLE Integration

## **TL;DR - What Changed**

1. **New packet format** with CRC32C integrity checking
2. **Resend capability** for corrupted packets  
3. **File validation** to catch corruption early
4. **Corruption statistics** for monitoring

---

## **🔥 CRITICAL CHANGES**

### **OLD Packet Format (Remove This)**
```
[seq][len][eof][payload] // 5-byte header, no integrity
```

### **NEW Packet Format (Implement This)**
```
[proto_ver][seq][file_id][offset][payload_len][flags][payload][crc32c]
     2        2      4        4         2        2      N       4    = 20+N bytes
```

---

## **📋 IMPLEMENTATION CHECKLIST**

### **Phase 1 (Critical - Week 1)**
- [ ] **Update packet parsing** - Handle new 16-byte header + CRC
- [ ] **Add CRC32C validation** - Reject packets with bad checksums
- [ ] **Implement resend requests** - Send CMD_RESEND(file_id, seq) on CRC fail
- [ ] **Update file reconstruction** - Handle out-of-order packets with sequence numbers

### **Phase 2 (Important - Week 2)**  
- [ ] **Add file validation** - Check RAW header magic "RAWA" (bytes: 41 57 41 52)
- [ ] **Corruption detection** - Count ADC values >4095 or ==0xFFFF
- [ ] **Retry logic** - Max 3 retries per sequence, then abort
- [ ] **Statistics UI** - Show corruption count, retry count, transfer speed

### **Phase 3 (Enhancement - Week 3)**
- [ ] **Performance optimization** - Parallel processing, efficient CRC
- [ ] **Error reporting** - Detailed corruption analysis
- [ ] **Protocol detection** - Support both old and new formats during transition

---

## **💻 CODE SNIPPETS**

### **CRC32C (Copy-Paste Ready)**

**iOS/Swift:**
```swift
// Add this class to your project
class CRC32C {
    private static let table: [UInt32] = {
        var table = Array<UInt32>(repeating: 0, count: 256)
        for i in 0..<256 {
            var crc = UInt32(i)
            for _ in 0..<8 {
                crc = (crc & 1) != 0 ? (crc >> 1) ^ 0x1EDC6F41 : (crc >> 1)
            }
            table[i] = crc
        }
        return table
    }()
    
    static func calculate(_ data: Data) -> UInt32 {
        var crc: UInt32 = 0xFFFFFFFF
        for byte in data {
            crc = table[Int((crc ^ UInt32(byte)) & 0xFF)] ^ (crc >> 8)
        }
        return ~crc
    }
}
```

**Android/Kotlin:**
```kotlin
object CRC32C {
    private val table = UIntArray(256) { i ->
        var crc = i.toUInt()
        repeat(8) { crc = if ((crc and 1u) != 0u) (crc shr 1) xor 0x1EDC6F41u else (crc shr 1) }
        crc
    }
    
    fun calculate(data: ByteArray): UInt {
        var crc = 0xFFFFFFFFu
        for (byte in data) {
            crc = table[((crc xor byte.toUInt()) and 0xFFu).toInt()] xor (crc shr 8)
        }
        return crc.inv()
    }
}
```

### **Packet Validation (Essential)**

**iOS/Swift:**
```swift
func validatePacket(_ data: Data) -> Bool {
    guard data.count >= 20 else { return false }
    
    let payloadLen = data.withUnsafeBytes { $0.load(fromByteOffset: 12, as: UInt16.self) }
    guard data.count == 16 + Int(payloadLen) + 4 else { return false }
    
    let headerAndPayload = data.prefix(16 + Int(payloadLen))
    let receivedCRC = data.withUnsafeBytes { $0.load(fromByteOffset: 16 + Int(payloadLen), as: UInt32.self) }
    let calculatedCRC = CRC32C.calculate(headerAndPayload)
    
    return calculatedCRC == receivedCRC
}
```

**Android/Kotlin:**
```kotlin
fun validatePacket(data: ByteArray): Boolean {
    if (data.size < 20) return false
    
    val payloadLen = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN).getShort(12).toInt()
    if (data.size != 16 + payloadLen + 4) return false
    
    val headerAndPayload = data.sliceArray(0 until 16 + payloadLen)
    val receivedCRC = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN).getInt(16 + payloadLen).toUInt()
    val calculatedCRC = CRC32C.calculate(headerAndPayload)
    
    return calculatedCRC == receivedCRC
}
```

### **Resend Request (Critical)**

**iOS/Swift:**
```swift
func requestResend(fileId: UInt32, seq: UInt16) -> Data {
    var data = Data([0x08]) // CMD_RESEND
    data.append(contentsOf: withUnsafeBytes(of: fileId.littleEndian) { Data($0) })
    data.append(contentsOf: withUnsafeBytes(of: seq.littleEndian) { Data($0) })
    return data
}
```

**Android/Kotlin:**
```kotlin
fun requestResend(fileId: UInt, seq: UShort): ByteArray {
    return ByteBuffer.allocate(7).order(ByteOrder.LITTLE_ENDIAN)
        .put(0x08.toByte()) // CMD_RESEND
        .putInt(fileId.toInt())
        .putShort(seq.toShort())
        .array()
}
```

---

## **🚨 BREAKING CHANGES**

### **What Will Break**
1. **Packet parsing** - New header format (16 bytes vs 5 bytes)
2. **File reconstruction** - Must handle sequence numbers and CRC validation
3. **Error handling** - Need resend capability for corrupted packets

### **What Stays The Same**
1. **BLE characteristics** - Same UUIDs and connection flow
2. **Control commands** - START, STOP, PAUSE still work (just added RESEND)
3. **File format** - RAW files have same structure, just better validation

---

## **📊 TESTING PRIORITIES**

### **Must Test (Week 1)**
1. **CRC validation** - Intentionally corrupt a packet, verify rejection
2. **Resend functionality** - Trigger resend, verify device responds
3. **File reconstruction** - Receive packets out of order, verify correct assembly

### **Should Test (Week 2)**  
1. **Large file transfers** - 1MB+ files with progress tracking
2. **Connection drops** - Resume transfer after BLE disconnect
3. **Corruption statistics** - Verify corruption counting is accurate

### **Nice to Test (Week 3)**
1. **Performance benchmarks** - Transfer speed with new format
2. **Memory usage** - Ensure no leaks during long transfers
3. **Edge cases** - Very small files, empty files, malformed packets

---

## **🎯 SUCCESS METRICS**

### **Before (Current Issues)**
- ❌ 16.7% corrupted samples (0xFFFF values)
- ❌ Header corruption ("RAWA" → "AWAR")
- ❌ No error recovery (must re-transfer entire files)
- ❌ No corruption detection

### **After (Expected Results)**
- ✅ 0% undetected corruption (CRC catches everything)
- ✅ Automatic recovery (resend only bad packets)
- ✅ Clean headers (explicit little-endian writes)
- ✅ Real-time corruption statistics

---

## **🆘 NEED HELP?**

### **Common Issues & Solutions**

**"CRC always fails"**
→ Check byte order (little-endian), verify header parsing

**"Packets arrive out of order"**  
→ Use sequence numbers for reconstruction, not arrival order

**"File validation fails"**
→ Check for magic bytes `41 57 41 52` at offset 0

**"Resend doesn't work"**
→ Verify CMD_RESEND format: `[0x08][file_id][seq]`

### **Contact Points**
- **Firmware questions**: Check `firmware_implementation_plan.md`
- **Protocol details**: See `mobile_team_integration_guide.md`  
- **Testing**: Use `ble_data_analyzer.py` for file validation

---

**🚀 The ESP32 firmware is ready - start with Phase 1 packet parsing!**
