# 🎵 SalesTag ESP32-S3 Audio Recorder

A complete audio recording system with ESP32-S3, featuring WiFi connectivity, web interface, and automatic file download capabilities.

## ✨ Features

- **🎙️ Audio Recording**: Continuous 440Hz sine wave generation for testing
- **💾 SD Card Storage**: WAV files saved to SD card with proper headers
- **🌐 WiFi Connectivity**: Connects to your network + creates backup access point
- **🖥️ Web Interface**: Manage recordings through any web browser
- **📥 Auto-Download**: Python script automatically downloads WAV files to your computer
- **🔧 Button Control**: Physical button for start/stop recording

## 🚀 Quick Start

### 1. Hardware Setup
- ESP32-S3 development board
- SD card (formatted as FAT32)
- Button connected to GPIO 4
- LED connected to GPIO 40

### 2. WiFi Configuration
Edit `software_v1/main/main.c` with your WiFi credentials:
```c
#define WIFI_SSID "YourWiFiSSID"
#define WIFI_PASSWORD "YourWiFiPassword"
```

### 3. Build and Flash
```bash
cd software_v1
source /Users/self/esp-idf/export.sh
idf.py build
idf.py flash
```

### 4. Monitor Output
```bash
idf.py monitor
```

## 🌐 Web Interface Access

### Option 1: Your WiFi Network (Recommended)
- **IP Address**: Check monitor output for your device's IP
- **URL**: `http://YOUR_IP_ADDRESS`
- **Features**: Full file management, recording control

### Option 2: SalesTag Access Point (Backup)
- **SSID**: `SalesTag_AP`
- **Password**: `salestag123`
- **URL**: `http://192.168.4.1`

## 📥 Auto-Download System

### Setup
1. **Install Python dependencies**:
   ```bash
   python3 -m venv venv
   source venv/bin/activate
   pip install requests
   ```

2. **Run the auto-download script**:
   ```bash
   # Download once
   python3 auto_download.py --once
   
   # Monitor continuously
   python3 auto_download.py
   
   # Custom options
   python3 auto_download.py --url http://YOUR_IP --interval 10
   ```

### Script Options
- `--url`: ESP32-S3 IP address (default: 192.168.1.194)
- `--output`: Output directory (default: current directory)
- `--once`: Download once and exit
- `--interval`: Check interval in seconds (default: 5)

### Features
- **🔄 Continuous Monitoring**: Automatically checks for new files
- **⏭️ Smart Skipping**: Won't re-download existing files
- **📅 Timestamped Names**: Files saved as `20240820_143022_rec_0001.wav`
- **💾 Progress Tracking**: Maintains list of downloaded files

## 🎯 Usage Examples

### Record Audio
1. **Physical Button**: Press button to start/stop recording
2. **Web Interface**: Use start/stop buttons in browser
3. **Monitor**: Watch real-time status in terminal

### Download Files
1. **Automatic**: Run `python3 auto_download.py` for continuous monitoring
2. **Manual**: Use web interface to download individual files
3. **Batch**: Script handles multiple files automatically

### File Management
- **View Files**: Web interface shows all recordings
- **Download**: One-click download through web interface
- **Delete**: Remove files from SD card when no longer needed

## 📁 Project Structure

```
salestag/
├── software_v1/           # ESP32-S3 firmware
│   ├── main/             # Main application code
│   │   ├── main.c        # Application entry point
│   │   ├── recorder.c    # Audio recording logic
│   │   ├── web_server.c  # Web interface server
│   │   ├── wifi_manager.c # WiFi connectivity
│   │   └── wav_writer.c  # WAV file handling
│   └── CMakeLists.txt    # Build configuration
├── auto_download.py       # Python auto-download script
├── simple_download.py     # Simple direct download script
├── download_wavs.sh       # Shell script wrapper
├── requirements.txt       # Python dependencies
└── README.md             # This file
```

## 🔧 Troubleshooting

### Common Issues

1. **WiFi Connection Failed**
   - Check credentials in `main.c`
   - Verify network availability
   - Use backup AP mode if needed

2. **Web Interface Not Accessible**
   - Check IP address in monitor output
   - Verify WiFi connection status
   - Try accessing backup AP at `192.168.4.1`

3. **Auto-Download Script Issues**
   - Install Python dependencies: `pip install requests`
   - Check ESP32-S3 IP address
   - Verify network connectivity

4. **Recording Problems**
   - Check SD card is properly inserted
   - Verify SD card is formatted as FAT32
   - Monitor for error messages

### Debug Commands

```bash
# Check ESP32-S3 status
idf.py monitor

# Test web interface
curl http://YOUR_IP/

# Test file API
curl http://YOUR_IP/api/files

# Test direct download
curl http://YOUR_IP/download?file=rec_0001.wav
```

## 🚀 Future Enhancements

- **☁️ Cloud Storage**: Supabase integration for backup
- **📱 Bluetooth**: Direct file transfer to mobile devices
- **🎵 Real Audio**: Microphone input instead of test tone
- **🔊 Audio Processing**: Effects, compression, analysis
- **📊 Analytics**: Recording statistics and insights

## 📞 Support

For issues or questions:
1. Check the troubleshooting section above
2. Review monitor output for error messages
3. Verify hardware connections and SD card
4. Test individual components (WiFi, web server, recording)

## 📄 License

This project is open source. Feel free to modify and distribute according to your needs.

---

**🎵 Happy Recording!** 🎵
