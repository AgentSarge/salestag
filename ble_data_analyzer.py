#!/usr/bin/env python3
"""
BLE Data Corruption Analyzer
Analyzes audio data received from ESP32 to identify corruption patterns
"""

import struct
import sys
from typing import List, Dict, Tuple
from dataclasses import dataclass
from collections import Counter

@dataclass
class RawAudioHeader:
    magic_number: int
    version: int
    sample_rate: int
    total_samples: int
    start_timestamp: int
    end_timestamp: int

    @classmethod
    def from_bytes(cls, data: bytes) -> 'RawAudioHeader':
        if len(data) < 24:  # Header is 24 bytes
            raise ValueError("Header too short")

        values = struct.unpack('<IIIIIII', data[:24])
        return cls(
            magic_number=values[0],
            version=values[1],
            sample_rate=values[2],
            total_samples=values[3],
            start_timestamp=values[4],
            end_timestamp=values[5]
        )

class BLEDataAnalyzer:
    def __init__(self):
        self.corruption_stats = Counter()
        self.sample_analysis = []
        self.header_analysis = []

    def analyze_header(self, header_data: bytes) -> Dict:
        """Analyze header for corruption"""
        try:
            header = RawAudioHeader.from_bytes(header_data)
            analysis = {
                'valid': True,
                'magic_valid': header.magic_number == 0x52415741,  # "RAWA"
                'magic_bytes': header_data[:4].hex(),
                'magic_string': ''.join(chr(b) if 32 <= b <= 126 else '?' for b in header_data[:4]),
                'version': header.version,
                'sample_rate': header.sample_rate,
                'total_samples': header.total_samples,
                'start_timestamp': header.start_timestamp,
                'end_timestamp': header.end_timestamp,
                'duration_ms': header.end_timestamp - header.start_timestamp if header.end_timestamp > header.start_timestamp else 0,
                'issues': []
            }

            # Check for corruption indicators
            if not analysis['magic_valid']:
                analysis['issues'].append(f"Invalid magic number: {header.magic_number:08X} (expected: 52415741)")

            if header.version != 1:
                analysis['issues'].append(f"Unexpected version: {header.version} (expected: 1)")

            if header.sample_rate != 16000:
                analysis['issues'].append(f"Unexpected sample rate: {header.sample_rate} (expected: 16000)")

            if header.start_timestamp > 1000000000000:  # > 1000 seconds in ms
                analysis['issues'].append(f"Suspicious start timestamp: {header.start_timestamp}")

            if header.total_samples > 1000000:  # > 1M samples (unrealistic for short recordings)
                analysis['issues'].append(f"Suspicious total samples: {header.total_samples}")

            if analysis['issues']:
                analysis['valid'] = False

            return analysis

        except Exception as e:
            return {
                'valid': False,
                'error': str(e),
                'raw_data': header_data.hex(),
                'issues': [f"Parse error: {str(e)}"]
            }

    def analyze_samples(self, sample_data: bytes, expected_count: int = None) -> Dict:
        """Analyze audio samples for corruption"""
        if len(sample_data) % 12 != 0:  # Each sample is 12 bytes
            return {
                'valid': False,
                'error': f"Invalid sample data length: {len(sample_data)} (not divisible by 12)",
                'issues': ["Data length not divisible by sample size (12 bytes)"]
            }

        sample_count = len(sample_data) // 12
        samples = []

        # Parse all samples
        for i in range(sample_count):
            offset = i * 12
            sample_bytes = sample_data[offset:offset+12]

            try:
                mic_sample, timestamp, sample_count_val = struct.unpack('<III', sample_bytes)
                samples.append({
                    'index': i,
                    'mic_sample': mic_sample,
                    'timestamp': timestamp,
                    'sample_count': sample_count_val
                })
            except struct.error as e:
                return {
                    'valid': False,
                    'error': f"Failed to parse sample {i}: {str(e)}",
                    'issues': [f"Sample parsing error at index {i}"]
                }

        # Analyze samples
        analysis = {
            'sample_count': sample_count,
            'valid': True,
            'issues': []
        }

        # Check for extreme values
        adc_values = [s['mic_sample'] for s in samples]
        extreme_values = [v for v in adc_values if v > 4095]  # 12-bit ADC max

        if extreme_values:
            analysis['issues'].append(f"Found {len(extreme_values)} extreme ADC values > 4095")
            analysis['extreme_values'] = extreme_values[:10]  # Show first 10

        # Check for 0xFFFF values (common corruption pattern)
        ffff_values = [i for i, v in enumerate(adc_values) if v == 0xFFFF]
        if ffff_values:
            analysis['issues'].append(f"Found {len(ffff_values)} samples with 0xFFFF value")
            analysis['ffff_positions'] = ffff_values[:10]

        # Check timestamp sequence
        timestamps = [s['timestamp'] for s in samples]
        timestamp_diffs = [t2 - t1 for t1, t2 in zip(timestamps[:-1], timestamps[1:])]
        invalid_diffs = [d for d in timestamp_diffs if d < 0 or d > 1000]  # More than 1 second gap

        if invalid_diffs:
            analysis['issues'].append(f"Found {len(invalid_diffs)} invalid timestamp differences")
            analysis['timestamp_issues'] = invalid_diffs[:5]

        # Check sample count sequence
        sample_counts = [s['sample_count'] for s in samples]
        expected_sequence = list(range(sample_counts[0], sample_counts[0] + len(sample_counts)))
        sequence_errors = [i for i, (actual, expected) in enumerate(zip(sample_counts, expected_sequence)) if actual != expected]

        if sequence_errors:
            analysis['issues'].append(f"Found {len(sequence_errors)} sample count sequence errors")
            analysis['sequence_errors'] = sequence_errors[:10]

        # Statistics
        analysis['adc_stats'] = {
            'min': min(adc_values),
            'max': max(adc_values),
            'mean': sum(adc_values) / len(adc_values),
            'unique_values': len(set(adc_values))
        }

        if analysis['issues']:
            analysis['valid'] = False

        return analysis

    def analyze_file(self, filepath: str) -> Dict:
        """Analyze complete audio file for corruption"""
        try:
            with open(filepath, 'rb') as f:
                data = f.read()

            if len(data) < 24:
                return {
                    'valid': False,
                    'error': "File too short for header",
                    'file_size': len(data)
                }

            # Analyze header
            header_data = data[:24]
            header_analysis = self.analyze_header(header_data)

            # Analyze samples if header is valid
            sample_analysis = {}
            if header_analysis['valid']:
                sample_data = data[24:]
                sample_analysis = self.analyze_samples(sample_data, header_analysis.get('total_samples'))

            return {
                'file_size': len(data),
                'header_analysis': header_analysis,
                'sample_analysis': sample_analysis,
                'overall_valid': header_analysis['valid'] and sample_analysis.get('valid', False),
                'summary': {
                    'issues_found': len(header_analysis.get('issues', [])) + len(sample_analysis.get('issues', [])),
                    'data_integrity': "GOOD" if (header_analysis['valid'] and sample_analysis.get('valid', False)) else "CORRUPTED"
                }
            }

        except Exception as e:
            return {
                'valid': False,
                'error': f"Failed to analyze file: {str(e)}"
            }

def main():
    if len(sys.argv) != 2:
        print("Usage: python ble_data_analyzer.py <audio_file>")
        sys.exit(1)

    analyzer = BLEDataAnalyzer()
    result = analyzer.analyze_file(sys.argv[1])

    print("🔍 BLE Data Corruption Analysis Report")
    print("=" * 50)
    print(f"File: {sys.argv[1]}")
    print(f"Size: {result['file_size']} bytes")
    print(f"Overall Status: {'✅ VALID' if result['overall_valid'] else '❌ CORRUPTED'}")
    print()

    # Header analysis
    header = result['header_analysis']
    print("📋 HEADER ANALYSIS:")
    print(f"  Magic: {header.get('magic_string', 'N/A')} ({header.get('magic_bytes', 'N/A')})")
    print(f"  Valid: {'✅' if header.get('valid') else '❌'}")
    if header.get('issues'):
        for issue in header['issues']:
            print(f"  ⚠️  {issue}")
    print()

    # Sample analysis
    if 'sample_analysis' in result and result['sample_analysis']:
        samples = result['sample_analysis']
        print("🎵 SAMPLE ANALYSIS:")
        print(f"  Sample Count: {samples.get('sample_count', 0)}")
        print(f"  Valid: {'✅' if samples.get('valid') else '❌'}")

        if samples.get('adc_stats'):
            stats = samples['adc_stats']
            print(f"  ADC Range: {stats['min']} - {stats['max']} (expected: 0-4095)")
            print(f"  ADC Mean: {stats['mean']:.1f}")
            print(f"  Unique Values: {stats['unique_values']}")

        if samples.get('issues'):
            for issue in samples['issues']:
                print(f"  ⚠️  {issue}")
    print()

    # Summary
    summary = result['summary']
    print("📊 SUMMARY:")
    print(f"  Issues Found: {summary['issues_found']}")
    print(f"  Data Integrity: {summary['data_integrity']}")
    print()
    print("💡 RECOMMENDATIONS:")
    if not result['overall_valid']:
        print("  - Check BLE connection stability")
        print("  - Verify firmware data validation")
        print("  - Consider adding CRC checks")
        print("  - Test with smaller BLE packets")
    else:
        print("  - Data appears to be intact")
        print("  - Consider monitoring for intermittent corruption")

if __name__ == "__main__":
    main()
