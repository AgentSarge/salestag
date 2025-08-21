#!/usr/bin/env python3
"""
Simple WAV Download Script for SalesTag ESP32-S3
Directly downloads WAV files without going through the API.
"""

import requests
import os
from datetime import datetime

def download_wav_file(ip_address, filename="rec_0001.wav"):
    """Download a WAV file directly from the ESP32-S3"""
    
    url = f"http://{ip_address}/download?file={filename}"
    
    try:
        print(f"🔍 Attempting to download: {filename}")
        print(f"🌐 URL: {url}")
        
        # Download the file
        response = requests.get(url, timeout=10)
        
        if response.status_code == 200:
            # Create timestamped filename
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            output_filename = f"{timestamp}_{filename}"
            
            # Save the file
            with open(output_filename, 'wb') as f:
                f.write(response.content)
            
            file_size = len(response.content)
            print(f"✅ Successfully downloaded: {output_filename}")
            print(f"📊 File size: {file_size:,} bytes")
            
            return True
        else:
            print(f"❌ Failed to download: HTTP {response.status_code}")
            print(f"Response: {response.text}")
            return False
            
    except Exception as e:
        print(f"❌ Error downloading file: {e}")
        return False

def main():
    # Your ESP32-S3 IP address
    esp_ip = "192.168.1.194"
    
    print("🎵 SalesTag Simple WAV Downloader")
    print("==================================")
    print(f"🌐 ESP32-S3 IP: {esp_ip}")
    print("")
    
    # Try to download the WAV file
    success = download_wav_file(esp_ip, "rec_0001.wav")
    
    if success:
        print("\n🎉 Download completed successfully!")
    else:
        print("\n💥 Download failed. Check ESP32-S3 status.")

if __name__ == "__main__":
    main()
