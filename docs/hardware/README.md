# SalesTag Hardware Documentation

## Overview

The SalesTag is a compact IoT audio recording device designed for portable, high-quality stereo audio capture with wireless connectivity. The device features dual microphones, onboard storage, and battery-powered operation.

## Key Specifications

- **Dimensions**: Compact PCB design optimized for portability
- **Power**: USB-C rechargeable with integrated battery management
- **Audio**: Dual-channel stereo recording with adjustable gain
- **Storage**: MicroSD card slot for local data storage
- **Connectivity**: WiFi and Bluetooth via ESP32-S3
- **Interface**: LED status indicator and tactile buttons

## Core Components

### Microcontroller

- **Part**: ESP32-S3-MINI-1-N8 (U1)
- **Manufacturer**: Espressif Systems
- **Features**:
  - Dual-core Xtensa LX7 @ 240MHz
  - 512KB SRAM, 8MB Flash
  - WiFi 802.11 b/g/n
  - Bluetooth 5.0 LE
  - 45 programmable GPIOs
- **Package**: Surface mount module
- **Datasheet**: [device_hardware_info/Datasheets/esp32-s3-mini-1 datasheet.pdf](../../device_hardware_info/Datasheets/esp32-s3-mini-1%20datasheet.pdf)

### Audio System

#### Microphone Amplifiers

- **Part**: MAX9814ETD+T (U3, U19)
- **Manufacturer**: Analog Devices/Maxim
- **Quantity**: 2 (stereo setup)
- **Features**:
  - Low-noise microphone amplifier
  - Automatic gain control (AGC)
  - Variable gain control via external resistors
  - Low power consumption
- **Package**: TDFN-14
- **Datasheet**: [device_hardware_info/Datasheets/MAX9814 Microphone datasheet.pdf](../../device_hardware_info/Datasheets/MAX9814%20Microphone%20datasheet.pdf)

#### Microphones

- **Part**: GMI4015P-66DB (MIC1, MIC2)
- **Manufacturer**: INGHAi
- **Type**: Electret condenser microphones
- **Sensitivity**: -66dB
- **Package**: Through-hole, 4.0mm diameter

### Storage

- **Component**: MicroSD Card Slot (CARD1)
- **Part**: TF-102-15
- **Manufacturer**: XUNPU
- **Interface**: SPI connection to ESP32-S3
- **Signals**: CS (IO39), MOSI (IO35), SCLK (IO36), MISO (IO37)

### Power Management

#### USB-C Connector

- **Part**: TYPE-C 16PIN 2MD(073) (USB1)
- **Features**: Charging and data interface
- **Power Input**: 5V via USB-C

#### Battery Charger

- **Part**: MCP73831T-2ACI/OT (U2)
- **Manufacturer**: Microchip
- **Features**:
  - Single-cell Li-Ion/Li-Polymer charger
  - Constant current/constant voltage operation
  - Thermal regulation
  - Status output (STAT signal)

#### Voltage Regulator

- **Part**: HE9073A33MR (U4)
- **Manufacturer**: HEERMICR
- **Output**: 3.3V regulated supply
- **Package**: SOT-23-3

### User Interface

#### Status LED

- **Part**: XL-1608UBC-04 (LED1)
- **Type**: Blue LED
- **Package**: 0603
- **Connection**: GPIO40 via current limiting resistor

#### Control Buttons

- **Part**: TS-1088-AR02016 (SW1, SW2)
- **Type**: Tactile switches
- **Functions**:
  - SW1: Enable/Reset (CHIP_PU)
  - SW2: User button (IO4)

## Power System

### Power Rails

- **VDC**: Battery/USB input voltage
- **CHRG5V**: 5V from USB charging
- **+3V3**: 3.3V regulated supply for digital circuits

### Power Consumption (Estimated)

- **Active Recording**: ~100-150mA @ 3.3V
- **WiFi Transmission**: ~200-250mA peak
- **Standby**: ~10-20mA
- **Deep Sleep**: <1mA

## GPIO Assignments

| GPIO      | Function       | Component         |
| --------- | -------------- | ----------------- |
| IO0       | Boot/Program   | SW1 (via pull-up) |
| IO4       | User Button    | SW2               |
| IO35      | SPI MOSI       | SD Card           |
| IO36      | SPI SCLK       | SD Card           |
| IO37      | SPI MISO       | SD Card           |
| IO39      | SPI CS         | SD Card           |
| IO40      | Status LED     | LED1              |
| MIC_DATA1 | Audio Input 1  | U3 Output         |
| MIC_DATA2 | Audio Input 2  | U19 Output        |
| GAIN1     | Gain Control 1 | U3 Gain           |
| GAIN2     | Gain Control 2 | U19 Gain          |
| A_R1      | Reference 1    | U3 Reference      |
| A_R2      | Reference 2    | U19 Reference     |
| STAT      | Charge Status  | U2 Status         |

## PCB Design

### Layer Stack

- **Layers**: 4-layer PCB
- **Top**: Component placement and routing
- **Inner 1 & 2**: Power and ground planes
- **Bottom**: Additional routing and components

### Design Files Location

- **Schematic**: [device_hardware_info/Schematic.pdf](../../device_hardware_info/Schematic.pdf)
- **3D Renders**:
  - [Front View](../../device_hardware_info/3D_PCB_front.png)
  - [Back View](../../device_hardware_info/3D_PCB_back.png)
- **EasyEDA Project**: [device_hardware_info/EasyEDA pro/Sound Recorder IoT updated.epro](../../device_hardware_info/EasyEDA%20pro/Sound%20Recorder%20IoT%20updated.epro)

## Manufacturing Files

All production-ready files are located in `device_hardware_info/Production/`:

### Gerber Files

- Complete set of Gerber files for PCB fabrication
- Located in: [device_hardware_info/Production/Gerber/](../../device_hardware_info/Production/Gerber/)
- Includes drill files, paste mask, solder mask, and all copper layers

### Assembly Files

- **BOM**: [device_hardware_info/Production/BOM.csv](../../device_hardware_info/Production/BOM.csv)
- **Pick and Place**: [device_hardware_info/Production/PickAndPlace.csv](../../device_hardware_info/Production/PickAndPlace.csv)
- **Assembly Package**: [device_hardware_info/Production/Gerber.zip](../../device_hardware_info/Production/Gerber.zip)

### Component Sourcing

All components are sourced from LCSC (www.lcsc.com) with part numbers specified in the BOM. Alternative suppliers:

- **Digi-Key**: For prototyping and small quantities
- **Mouser**: Alternative electronic component supplier
- **Arrow**: For ESP32 modules and Analog Devices components

## Testing and Validation

### Recommended Test Points

- Power rails verification (3.3V, 5V)
- Audio signal integrity
- SPI communication with SD card
- WiFi/Bluetooth connectivity
- Battery charging functionality

### Programming Interface

- **Method**: USB-C connector with built-in USB-to-serial
- **Boot Mode**: Hold SW1 (ENABLE) during power-up
- **Tools**: ESP-IDF, Arduino IDE, or PlatformIO

## Design Considerations

### Audio Quality

- Dedicated analog ground planes
- Proper microphone bias networks
- Adjustable gain for different recording scenarios
- Low-noise power supply design

### Power Efficiency

- Efficient switching regulator
- Deep sleep modes supported
- Battery monitoring capabilities

### Thermal Management

- Thermal relief on power components
- Adequate copper pour for heat dissipation
- Component placement optimized for airflow

## Future Revisions

### Potential Improvements

- Add external antenna connector option
- Include hardware audio filtering
- Add more user interface elements
- Consider USB audio interface capability

---

**Last Updated**: December 2024  
**Hardware Revision**: v1.0  
**Document Version**: 1.0
