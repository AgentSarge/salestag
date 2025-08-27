#!/usr/bin/env python3
"""
SalesTag BLE Audio Receiver
Connects to SalesTag device via BLE and receives raw audio files for cloud upload.
"""

import asyncio
import struct
import os
import time
from datetime import datetime
import logging
from bleak import BleakClient, BleakScanner
from bleak.backends.characteristic import BleakGATTCharacteristic

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

# BLE device configuration
DEVICE_NAME = "SalesTag-Audio"
SERVICE_UUID = "12345678-1234-1234-1234-123456789abc"
CHARACTERISTIC_UUID = "87654321-4321-4321-4321-cba987654321"

# File transfer configuration
DOWNLOAD_DIR = "received_audio"
CHUNK_SIZE = 240  # Match ESP32-S3 chunk size

class SalesTagAudioReceiver:
    def __init__(self):
        self.client = None
        self.current_file = None
        self.current_filename = None
        self.file_size = 0
        self.bytes_received = 0
        self.transfer_start_time = None
        
        # Create download directory
        os.makedirs(DOWNLOAD_DIR, exist_ok=True)
        
    async def scan_for_device(self):
        """Scan for SalesTag device"""
        logger.info(f"Scanning for device: {DEVICE_NAME}")
        
        devices = await BleakScanner.discover(timeout=10.0)
        for device in devices:
            if device.name and DEVICE_NAME in device.name:
                logger.info(f"Found device: {device.name} ({device.address})")
                return device
                
        logger.error(f"Device {DEVICE_NAME} not found")
        return None
        
    async def connect_to_device(self, device):
        """Connect to SalesTag device"""
        try:
            self.client = BleakClient(device.address)
            await self.client.connect()
            logger.info(f"Connected to {device.name}")
            return True
        except Exception as e:
            logger.error(f"Failed to connect: {e}")
            return False
            
    def notification_handler(self, characteristic: BleakGATTCharacteristic, data: bytearray):
        """Handle incoming BLE notifications"""
        try:
            # Convert data to string for header parsing
            data_str = data.decode('utf-8', errors='ignore')
            
            # Check if this is a file header
            if data_str.startswith("FILE:"):
                self._handle_file_header(data_str)
            elif data_str == "END":
                self._handle_file_end()
            else:
                # This is file data
                self._handle_file_data(data)
                
        except Exception as e:
            logger.error(f"Error handling notification: {e}")
            
    def _handle_file_header(self, header_str):
        """Handle file transfer header"""
        try:
            # Parse header: "FILE:filename:filesize"
            parts = header_str.split(":")
            if len(parts) >= 3:
                filename = parts[1]
                file_size = int(parts[2])
                
                self.current_filename = os.path.join(DOWNLOAD_DIR, filename)
                self.file_size = file_size
                self.bytes_received = 0
                self.transfer_start_time = time.time()
                
                # Open file for writing
                self.current_file = open(self.current_filename, 'wb')
                logger.info(f"Starting file transfer: {filename} ({file_size} bytes)")
                
        except Exception as e:
            logger.error(f"Error handling file header: {e}")
            
    def _handle_file_data(self, data):
        """Handle file data chunk"""
        if self.current_file:
            try:
                self.current_file.write(data)
                self.bytes_received += len(data)
                
                # Log progress every 10%
                if self.file_size > 0:
                    progress = (self.bytes_received / self.file_size) * 100
                    if int(progress) % 10 == 0:
                        logger.info(f"Transfer progress: {progress:.1f}% ({self.bytes_received}/{self.file_size} bytes)")
                        
            except Exception as e:
                logger.error(f"Error writing file data: {e}")
                
    def _handle_file_end(self):
        """Handle end of file transfer"""
        if self.current_file:
            try:
                self.current_file.close()
                transfer_time = time.time() - self.transfer_start_time
                transfer_rate = self.bytes_received / transfer_time if transfer_time > 0 else 0
                
                logger.info(f"File transfer complete: {self.current_filename}")
                logger.info(f"Bytes received: {self.bytes_received}")
                logger.info(f"Transfer time: {transfer_time:.2f} seconds")
                logger.info(f"Transfer rate: {transfer_rate:.2f} bytes/second")
                
                # Reset state
                self.current_file = None
                self.current_filename = None
                self.file_size = 0
                self.bytes_received = 0
                self.transfer_start_time = None
                
            except Exception as e:
                logger.error(f"Error closing file: {e}")
                
    async def start_notifications(self):
        """Start receiving notifications"""
        try:
            await self.client.start_notify(CHARACTERISTIC_UUID, self.notification_handler)
            logger.info("Started receiving notifications")
            return True
        except Exception as e:
            logger.error(f"Failed to start notifications: {e}")
            return False
            
    async def run(self):
        """Main run loop"""
        try:
            # Scan for device
            device = await self.scan_for_device()
            if not device:
                return
                
            # Connect to device
            if not await self.connect_to_device(device):
                return
                
            # Start notifications
            if not await self.start_notifications():
                return
                
            logger.info("Connected and ready to receive audio files")
            logger.info("Press Ctrl+C to disconnect")
            
            # Keep connection alive
            while True:
                await asyncio.sleep(1)
                
        except KeyboardInterrupt:
            logger.info("Disconnecting...")
        except Exception as e:
            logger.error(f"Error in main loop: {e}")
        finally:
            if self.current_file:
                self.current_file.close()
            if self.client and self.client.is_connected:
                await self.client.disconnect()
                
    def upload_to_cloud(self, filepath):
        """Upload received file to cloud (placeholder)"""
        logger.info(f"Would upload {filepath} to cloud")
        # TODO: Implement actual cloud upload
        # This could be AWS S3, Google Cloud Storage, etc.
        pass

async def main():
    """Main entry point"""
    receiver = SalesTagAudioReceiver()
    await receiver.run()

if __name__ == "__main__":
    asyncio.run(main())
