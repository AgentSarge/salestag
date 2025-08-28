# 📱 Mobile Team Integration Guide - BLE Data Integrity

## **EXECUTIVE SUMMARY**
The ESP32 firmware now implements robust data integrity protection. Mobile app must be updated to handle new BLE packet format with CRC32C validation and corruption detection.

---

## **1. NEW BLE PACKET FORMAT**

### **Updated Packet Structure**
```swift
// Swift/iOS
struct BLEChunk {
    let protoVer: UInt16        // 0x0001
    let seq: UInt16             // Sequence number (increments by 1)
    let fileId: UInt32          // File/session ID
    let offset: UInt32          // Byte offset in file
    let payloadLen: UInt16      // Number of payload bytes
    let flags: UInt16           // 0=mid, 1=start, 2=end, 3=single
    // payload: Data[payloadLen]
    // crc32c: UInt32           // CRC32C over header+payload
}
```

```kotlin
// Kotlin/Android
data class BLEChunk(
    val protoVer: UShort,       // 0x0001u
    val seq: UShort,            // Sequence number
    val fileId: UInt,           // File/session ID
    val offset: UInt,           // Byte offset in file
    val payloadLen: UShort,     // Payload length
    val flags: UShort,          // Chunk type flags
    val payload: ByteArray,     // Actual data
    val crc32c: UInt            // CRC32C checksum
)
```

### **Packet Layout (Little-Endian)**
```
Offset | Size | Field        | Description
-------|------|--------------|---------------------------
0      | 2    | proto_ver    | Protocol version (0x0001)
2      | 2    | seq          | Sequence number
4      | 4    | file_id      | File/session identifier
8      | 4    | offset       | Byte offset in file
12     | 2    | payload_len  | Payload size (N bytes)
14     | 2    | flags        | Chunk flags
16     | N    | payload      | File data
16+N   | 4    | crc32c       | CRC32C checksum
```

---

## **2. CONTROL MESSAGES (UNCHANGED)**

### **File Control Characteristic (Write)**
```swift
enum FileControlCommand: UInt8 {
    case start = 0x01           // Start transfer (auto-select latest)
    case pause = 0x02           // Pause transfer
    case resume = 0x03          // Resume transfer
    case stop = 0x04            // Stop transfer
    case listFiles = 0x05       // List available files
    case selectFile = 0x06      // Select specific file
    case startWithFilename = 0x07 // Start with specific filename
    case resend = 0x08          // NEW: Request resend of specific seq
}
```

### **NEW: Resend Command Format**
```swift
struct ResendCommand {
    let command: UInt8 = 0x08   // CMD_RESEND
    let fileId: UInt32          // File ID from chunk
    let seq: UInt16             // Sequence number to resend
}
```

---

## **3. CRC32C IMPLEMENTATION**

### **iOS/Swift Implementation**
```swift
import Foundation

class CRC32C {
    private static let polynomial: UInt32 = 0x1EDC6F41
    private static var table: [UInt32] = {
        var table = Array<UInt32>(repeating: 0, count: 256)
        for i in 0..<256 {
            var crc = UInt32(i)
            for _ in 0..<8 {
                crc = (crc & 1) != 0 ? (crc >> 1) ^ polynomial : (crc >> 1)
            }
            table[i] = crc
        }
        return table
    }()
    
    static func calculate(_ data: Data) -> UInt32 {
        var crc: UInt32 = 0xFFFFFFFF
        for byte in data {
            let index = Int((crc ^ UInt32(byte)) & 0xFF)
            crc = table[index] ^ (crc >> 8)
        }
        return ~crc
    }
    
    static func update(_ crc: UInt32, data: Data) -> UInt32 {
        var crc = ~crc
        for byte in data {
            let index = Int((crc ^ UInt32(byte)) & 0xFF)
            crc = table[index] ^ (crc >> 8)
        }
        return ~crc
    }
}
```

### **Android/Kotlin Implementation**
```kotlin
object CRC32C {
    private const val POLYNOMIAL = 0x1EDC6F41u
    
    private val table = UIntArray(256) { i ->
        var crc = i.toUInt()
        repeat(8) {
            crc = if ((crc and 1u) != 0u) (crc shr 1) xor POLYNOMIAL else (crc shr 1)
        }
        crc
    }
    
    fun calculate(data: ByteArray): UInt {
        var crc = 0xFFFFFFFFu
        for (byte in data) {
            val index = ((crc xor byte.toUInt()) and 0xFFu).toInt()
            crc = table[index] xor (crc shr 8)
        }
        return crc.inv()
    }
    
    fun update(crc: UInt, data: ByteArray): UInt {
        var crc = crc.inv()
        for (byte in data) {
            val index = ((crc xor byte.toUInt()) and 0xFFu).toInt()
            crc = table[index] xor (crc shr 8)
        }
        return crc.inv()
    }
}
```

---

## **4. PACKET PARSING & VALIDATION**

### **iOS/Swift Packet Parser**
```swift
struct BLEPacketParser {
    static func parse(_ data: Data) -> BLEChunk? {
        guard data.count >= 20 else { return nil } // Minimum header + CRC
        
        let protoVer = data.withUnsafeBytes { $0.load(fromByteOffset: 0, as: UInt16.self) }
        let seq = data.withUnsafeBytes { $0.load(fromByteOffset: 2, as: UInt16.self) }
        let fileId = data.withUnsafeBytes { $0.load(fromByteOffset: 4, as: UInt32.self) }
        let offset = data.withUnsafeBytes { $0.load(fromByteOffset: 8, as: UInt32.self) }
        let payloadLen = data.withUnsafeBytes { $0.load(fromByteOffset: 12, as: UInt16.self) }
        let flags = data.withUnsafeBytes { $0.load(fromByteOffset: 14, as: UInt16.self) }
        
        guard data.count == 16 + Int(payloadLen) + 4 else { return nil }
        
        let payload = data.subdata(in: 16..<(16 + Int(payloadLen)))
        let receivedCRC = data.withUnsafeBytes { 
            $0.load(fromByteOffset: 16 + Int(payloadLen), as: UInt32.self) 
        }
        
        // Validate CRC
        let headerData = data.subdata(in: 0..<16)
        var calculatedCRC = CRC32C.update(0xFFFFFFFF, data: headerData)
        calculatedCRC = CRC32C.update(calculatedCRC, data: payload)
        calculatedCRC = ~calculatedCRC
        
        guard calculatedCRC == receivedCRC else {
            print("❌ CRC mismatch: expected \(String(format: "0x%08X", receivedCRC)), got \(String(format: "0x%08X", calculatedCRC))")
            return nil
        }
        
        return BLEChunk(
            protoVer: protoVer,
            seq: seq,
            fileId: fileId,
            offset: offset,
            payloadLen: payloadLen,
            flags: flags,
            payload: payload,
            crc32c: receivedCRC
        )
    }
}
```

### **Android/Kotlin Packet Parser**
```kotlin
object BLEPacketParser {
    fun parse(data: ByteArray): BLEChunk? {
        if (data.size < 20) return null // Minimum header + CRC
        
        val buffer = ByteBuffer.wrap(data).order(ByteOrder.LITTLE_ENDIAN)
        
        val protoVer = buffer.getShort(0).toUShort()
        val seq = buffer.getShort(2).toUShort()
        val fileId = buffer.getInt(4).toUInt()
        val offset = buffer.getInt(8).toUInt()
        val payloadLen = buffer.getShort(12).toUShort()
        val flags = buffer.getShort(14).toUShort()
        
        if (data.size != 16 + payloadLen.toInt() + 4) return null
        
        val payload = data.sliceArray(16 until 16 + payloadLen.toInt())
        val receivedCRC = buffer.getInt(16 + payloadLen.toInt()).toUInt()
        
        // Validate CRC
        val headerData = data.sliceArray(0 until 16)
        var calculatedCRC = CRC32C.update(0xFFFFFFFFu, headerData)
        calculatedCRC = CRC32C.update(calculatedCRC, payload)
        calculatedCRC = calculatedCRC.inv()
        
        if (calculatedCRC != receivedCRC) {
            Log.e("BLE", "❌ CRC mismatch: expected ${receivedCRC.toString(16)}, got ${calculatedCRC.toString(16)}")
            return null
        }
        
        return BLEChunk(protoVer, seq, fileId, offset, payloadLen, flags, payload, receivedCRC)
    }
}
```

---

## **5. FILE RECONSTRUCTION & VALIDATION**

### **iOS/Swift File Assembler**
```swift
class FileAssembler {
    private var chunks: [UInt16: BLEChunk] = [:]
    private var expectedSeq: UInt16 = 0
    private var fileData = Data()
    
    func addChunk(_ chunk: BLEChunk) -> AssemblerResult {
        // Store chunk
        chunks[chunk.seq] = chunk
        
        // Process sequential chunks
        while let nextChunk = chunks[expectedSeq] {
            fileData.append(nextChunk.payload)
            chunks.removeValue(forKey: expectedSeq)
            expectedSeq += 1
            
            // Check if this is the end chunk
            if nextChunk.flags == 2 || nextChunk.flags == 3 { // END or SINGLE
                return .complete(fileData)
            }
        }
        
        return .needMore
    }
    
    func getMissingSequences() -> [UInt16] {
        var missing: [UInt16] = []
        for seq in 0..<expectedSeq {
            if chunks[seq] == nil {
                missing.append(seq)
            }
        }
        return missing
    }
}

enum AssemblerResult {
    case needMore
    case complete(Data)
}
```

### **RAW File Validation**
```swift
struct RAWFileValidator {
    static func validate(_ data: Data) -> ValidationResult {
        guard data.count >= 32 else { return .invalid("File too short") }
        
        // Check magic number "RAWA" (0x52415741 little-endian)
        let magic = data.withUnsafeBytes { $0.load(as: UInt32.self) }
        guard magic == 0x52415741 else {
            let bytes = data.prefix(4).map { String(format: "%02X", $0) }.joined(separator: " ")
            return .invalid("Invalid magic: \(bytes), expected: 41 57 41 52")
        }
        
        // Parse header
        let version = data.withUnsafeBytes { $0.load(fromByteOffset: 4, as: UInt32.self) }
        let sampleRate = data.withUnsafeBytes { $0.load(fromByteOffset: 8, as: UInt32.self) }
        let totalSamples = data.withUnsafeBytes { $0.load(fromByteOffset: 12, as: UInt32.self) }
        
        // Validate structure
        let expectedSize = 32 + Int(totalSamples) * 10
        guard data.count == expectedSize else {
            return .invalid("Size mismatch: got \(data.count), expected \(expectedSize)")
        }
        
        // Validate sample data
        var corruptionCount = 0
        let sampleData = data.subdata(in: 32..<data.count)
        
        for i in stride(from: 0, to: sampleData.count, by: 10) {
            let micSample = sampleData.withUnsafeBytes { 
                $0.load(fromByteOffset: i, as: UInt16.self) 
            }
            
            if micSample > 4095 || micSample == 0xFFFF {
                corruptionCount += 1
            }
        }
        
        let corruptionPercent = Double(corruptionCount) / Double(totalSamples) * 100.0
        
        return .valid(ValidationInfo(
            totalSamples: totalSamples,
            sampleRate: sampleRate,
            version: version,
            corruptionCount: corruptionCount,
            corruptionPercent: corruptionPercent
        ))
    }
}

enum ValidationResult {
    case valid(ValidationInfo)
    case invalid(String)
}

struct ValidationInfo {
    let totalSamples: UInt32
    let sampleRate: UInt32
    let version: UInt32
    let corruptionCount: Int
    let corruptionPercent: Double
}
```

---

## **6. ERROR HANDLING & RESEND LOGIC**

### **iOS/Swift Resend Manager**
```swift
class ResendManager {
    private let maxRetries = 3
    private var retryCount: [UInt16: Int] = [:]
    
    func requestResend(fileId: UInt32, seq: UInt16) -> Data {
        let command: UInt8 = 0x08 // CMD_RESEND
        var data = Data()
        data.append(command)
        data.append(contentsOf: withUnsafeBytes(of: fileId.littleEndian) { Data($0) })
        data.append(contentsOf: withUnsafeBytes(of: seq.littleEndian) { Data($0) })
        
        retryCount[seq] = (retryCount[seq] ?? 0) + 1
        
        print("🔄 Requesting resend: seq=\(seq), attempt=\(retryCount[seq] ?? 0)")
        return data
    }
    
    func shouldAbort(seq: UInt16) -> Bool {
        return (retryCount[seq] ?? 0) >= maxRetries
    }
    
    func reset() {
        retryCount.removeAll()
    }
}
```

---

## **7. INTEGRATION CHECKLIST**

### **Phase 1: Update BLE Parsing**
- [ ] Implement new packet structure parsing
- [ ] Add CRC32C validation
- [ ] Update file reconstruction logic
- [ ] Add resend request capability

### **Phase 2: Add Validation**
- [ ] Implement RAW file header validation
- [ ] Add corruption detection and reporting
- [ ] Update UI to show transfer statistics
- [ ] Add retry/abort logic for failed transfers

### **Phase 3: Testing**
- [ ] Test with known good files
- [ ] Test with intentionally corrupted packets
- [ ] Verify resend functionality
- [ ] Performance testing with large files

---

## **8. BACKWARDS COMPATIBILITY**

### **Protocol Detection**
```swift
func detectProtocolVersion(_ data: Data) -> ProtocolVersion {
    guard data.count >= 2 else { return .legacy }
    
    let protoVer = data.withUnsafeBytes { $0.load(as: UInt16.self) }
    return protoVer == 0x0001 ? .v2WithCRC : .legacy
}

enum ProtocolVersion {
    case legacy     // Original format without CRC
    case v2WithCRC  // New format with CRC32C
}
```

---

## **9. EXPECTED IMPROVEMENTS**

### **Data Integrity**
- ✅ **0% corrupted headers** (RAWA stays RAWA)
- ✅ **0 invalid ADC values** (no more 0xFFFF or >4095)
- ✅ **Reliable transfer** (CRC catches bit flips)
- ✅ **Automatic recovery** (resend on corruption)

### **User Experience**
- ✅ **Transfer progress** with corruption statistics
- ✅ **Automatic retry** on temporary failures
- ✅ **Clear error messages** when files are corrupted
- ✅ **Faster transfers** (no need to re-transfer entire files)

---

## **🚀 DEPLOYMENT TIMELINE**

1. **Week 1**: Implement packet parsing and CRC validation
2. **Week 2**: Add file validation and resend logic
3. **Week 3**: Testing and UI updates
4. **Week 4**: Production deployment

**The ESP32 firmware is ready now - mobile team can begin implementation immediately!** 📱✨
