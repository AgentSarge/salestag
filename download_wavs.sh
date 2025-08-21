#!/bin/bash

# SalesTag WAV Auto-Download Script
# Automatically downloads WAV files from your ESP32-S3

echo "🎵 SalesTag WAV Auto-Downloader"
echo "================================"

# Check if Python 3 is available
if ! command -v python3 &> /dev/null; then
    echo "❌ Python 3 is required but not installed"
    exit 1
fi

# Check if requests module is available
if ! python3 -c "import requests" &> /dev/null; then
    echo "📦 Installing required Python packages..."
    pip3 install -r requirements.txt
fi

# Default ESP32-S3 IP address (from your network)
ESP_IP="192.168.1.194"

echo "🌐 ESP32-S3 IP: $ESP_IP"
echo "📁 Files will be saved to: $(pwd)"
echo ""

# Run the auto-download script
echo "🚀 Starting auto-download..."
python3 auto_download.py --url "http://$ESP_IP" --output "."
