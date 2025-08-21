#!/usr/bin/env python3
"""
Auto-Download Script for SalesTag ESP32-S3
Automatically downloads WAV files from the web interface and saves them to the repo root.
"""

import requests
import json
import os
import time
from datetime import datetime
import argparse
from pathlib import Path

class SalesTagDownloader:
    def __init__(self, base_url, output_dir="."):
        self.base_url = base_url.rstrip('/')
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
        self.session = requests.Session()
        self.downloaded_files = set()
        
        # Load list of already downloaded files
        self.downloaded_list_file = self.output_dir / ".downloaded_files.txt"
        self.load_downloaded_list()
    
    def load_downloaded_list(self):
        """Load list of already downloaded files to avoid re-downloading"""
        if self.downloaded_list_file.exists():
            with open(self.downloaded_list_file, 'r') as f:
                self.downloaded_files = set(line.strip() for line in f if line.strip())
    
    def save_downloaded_list(self):
        """Save list of downloaded files"""
        with open(self.downloaded_list_file, 'w') as f:
            for filename in self.downloaded_files:
                f.write(f"{filename}\n")
    
    def get_file_list(self):
        """Get list of WAV files from the ESP32-S3"""
        try:
            response = self.session.get(f"{self.base_url}/api/files")
            if response.status_code == 200:
                data = response.json()
                files = data.get('files', [])
                if files:
                    return files
                else:
                    # If API returns empty list, try direct download of known files
                    print("📝 API returned empty file list, trying direct download...")
                    return [{"name": "rec_0001.wav", "size": "Unknown"}]
            else:
                print(f"❌ Failed to get file list: HTTP {response.status_code}")
                # Fallback to direct download
                print("📝 Trying direct download fallback...")
                return [{"name": "rec_0001.wav", "size": "Unknown"}]
        except Exception as e:
            print(f"❌ Error getting file list: {e}")
            # Fallback to direct download
            print("📝 Trying direct download fallback...")
            return [{"name": "rec_0001.wav", "size": "Unknown"}]
    
    def download_file(self, filename):
        """Download a single WAV file"""
        if filename in self.downloaded_files:
            print(f"⏭️  Skipping {filename} (already downloaded)")
            return False
        
        try:
            print(f"⬇️  Downloading {filename}...")
            
            # Download the file
            response = self.session.get(f"{self.base_url}/download?file={filename}")
            if response.status_code != 200:
                print(f"❌ Failed to download {filename}: HTTP {response.status_code}")
                return False
            
            # Create timestamped filename
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            safe_filename = filename.replace('.wav', '').replace(' ', '_')
            output_filename = f"{timestamp}_{safe_filename}.wav"
            output_path = self.output_dir / output_filename
            
            # Save the file
            with open(output_path, 'wb') as f:
                f.write(response.content)
            
            # Get file size
            file_size = len(response.content)
            print(f"✅ Downloaded {filename} -> {output_filename} ({file_size:,} bytes)")
            
            # Mark as downloaded
            self.downloaded_files.add(filename)
            self.save_downloaded_list()
            
            return True
            
        except Exception as e:
            print(f"❌ Error downloading {filename}: {e}")
            return False
    
    def download_all_files(self):
        """Download all available WAV files"""
        print(f"🔍 Checking for new files at {self.base_url}...")
        
        files = self.get_file_list()
        if not files:
            print("📭 No files found or error getting file list")
            return 0
        
        print(f"📁 Found {len(files)} file(s):")
        for file_info in files:
            print(f"   • {file_info['name']} ({file_info['size']})")
        
        # Download new files
        downloaded_count = 0
        for file_info in files:
            filename = file_info['name']
            if self.download_file(filename):
                downloaded_count += 1
        
        if downloaded_count == 0:
            print("✨ All files are already downloaded!")
        else:
            print(f"🎉 Downloaded {downloaded_count} new file(s)!")
        
        return downloaded_count
    
    def monitor_and_download(self, interval=5):
        """Continuously monitor for new files and download them"""
        print(f"🔄 Starting continuous monitoring (checking every {interval} seconds)...")
        print(f"📁 Files will be saved to: {self.output_dir.absolute()}")
        print(f"🌐 Monitoring: {self.base_url}")
        print("⏹️  Press Ctrl+C to stop")
        
        try:
            while True:
                self.download_all_files()
                time.sleep(interval)
                
        except KeyboardInterrupt:
            print("\n🛑 Monitoring stopped by user")
            print(f"📊 Total files downloaded: {len(self.downloaded_files)}")

def main():
    parser = argparse.ArgumentParser(description="Auto-download WAV files from SalesTag ESP32-S3")
    parser.add_argument("--url", "-u", 
                       default="http://192.168.1.194",
                       help="Base URL of ESP32-S3 (default: http://192.168.1.194)")
    parser.add_argument("--output", "-o", 
                       default=".",
                       help="Output directory (default: current directory)")
    parser.add_argument("--once", "-1", 
                       action="store_true",
                       help="Download once and exit (don't monitor continuously)")
    parser.add_argument("--interval", "-i", 
                       type=int, default=5,
                       help="Check interval in seconds (default: 5)")
    
    args = parser.parse_args()
    
    # Create downloader
    downloader = SalesTagDownloader(args.url, args.output)
    
    if args.once:
        # Download once
        downloader.download_all_files()
    else:
        # Monitor continuously
        downloader.monitor_and_download(args.interval)

if __name__ == "__main__":
    main()
