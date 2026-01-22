#!/usr/bin/env python3
"""
VARTA - Audio Sample Collection Tool
Record audio samples for training the drone detection model.

Usage:
    python collect_samples.py --device 0 --duration 300 --output samples/
    
Controls during recording:
    SPACE - Mark current segment as 'drone' (default is 'no_drone')
    Q     - Stop recording
"""

import argparse
import os
import time
import threading
from datetime import datetime
from pathlib import Path

import numpy as np
import sounddevice as sd
import soundfile as sf

# Try to import keyboard for hotkey support
try:
    import keyboard
    KEYBOARD_AVAILABLE = True
except ImportError:
    KEYBOARD_AVAILABLE = False
    print("Note: 'keyboard' module not found. Install with: pip install keyboard")
    print("Hotkey marking will be disabled.")


# Configuration
SAMPLE_RATE = 44100
CHANNELS = 1
SEGMENT_DURATION = 1.0  # seconds per sample
BLOCKSIZE = int(SAMPLE_RATE * SEGMENT_DURATION)


class AudioCollector:
    def __init__(self, device, output_dir, duration):
        self.device = device
        self.output_dir = Path(output_dir)
        self.duration = duration
        
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        self.is_recording = False
        self.is_drone_marked = False
        self.segments = []
        self.labels = []
        self.current_segment = []
        self.segment_count = 0
        self.start_time = None
        
    def audio_callback(self, indata, frames, time_info, status):
        """Called for each audio block."""
        if status:
            print(f"Audio status: {status}")
        
        # Append to current segment
        self.current_segment.extend(indata[:, 0])
        
        # Check if segment is complete
        if len(self.current_segment) >= BLOCKSIZE:
            segment = np.array(self.current_segment[:BLOCKSIZE])
            self.current_segment = self.current_segment[BLOCKSIZE:]
            
            # Determine label
            label = 1 if self.is_drone_marked else 0
            
            # Save segment
            self.save_segment(segment, label)
            
            # Reset mark for next segment
            self.is_drone_marked = False
    
    def save_segment(self, segment, label):
        """Save audio segment to file."""
        timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
        label_str = 'drone' if label == 1 else 'nodrone'
        filename = f'{timestamp}_{self.segment_count:04d}_{label_str}.wav'
        filepath = self.output_dir / filename
        
        sf.write(str(filepath), segment, SAMPLE_RATE)
        
        self.segments.append(filename)
        self.labels.append(label)
        self.segment_count += 1
        
        # Progress indicator
        elapsed = time.time() - self.start_time
        print(f"\r[{elapsed:.0f}s] Segments: {self.segment_count} | "
              f"Drones: {sum(self.labels)} | "
              f"{'[DRONE]' if label == 1 else '[----]'}    ", end='')
    
    def mark_drone(self):
        """Mark current segment as containing drone audio."""
        self.is_drone_marked = True
        print("\n>>> DRONE MARKED <<<")
    
    def stop(self):
        """Stop recording."""
        self.is_recording = False
    
    def run(self):
        """Main recording loop."""
        print("=" * 60)
        print("VARTA Audio Sample Collector")
        print("=" * 60)
        print(f"Device: {self.device}")
        print(f"Output: {self.output_dir}")
        print(f"Duration: {self.duration}s")
        print(f"Segment length: {SEGMENT_DURATION}s")
        print()
        
        if KEYBOARD_AVAILABLE:
            print("Controls:")
            print("  SPACE - Mark current segment as 'drone'")
            print("  Q     - Stop recording")
            print()
            
            # Set up hotkeys
            keyboard.on_press_key('space', lambda _: self.mark_drone())
            keyboard.on_press_key('q', lambda _: self.stop())
        else:
            print("Recording without hotkey support.")
            print("Press Ctrl+C to stop.")
            print()
        
        print("Starting recording in 3 seconds...")
        time.sleep(3)
        print("Recording started!")
        print()
        
        self.is_recording = True
        self.start_time = time.time()
        
        # Start audio stream
        try:
            with sd.InputStream(
                device=self.device,
                samplerate=SAMPLE_RATE,
                channels=CHANNELS,
                callback=self.audio_callback,
                blocksize=1024
            ):
                while self.is_recording:
                    elapsed = time.time() - self.start_time
                    if elapsed >= self.duration:
                        print(f"\n\nDuration reached ({self.duration}s)")
                        break
                    time.sleep(0.1)
                    
        except KeyboardInterrupt:
            print("\n\nRecording stopped by user.")
        
        finally:
            self.is_recording = False
            if KEYBOARD_AVAILABLE:
                keyboard.unhook_all()
        
        # Save labels CSV
        self.save_labels()
        
        print()
        print("=" * 60)
        print("Recording complete!")
        print(f"Total segments: {self.segment_count}")
        print(f"Drone segments: {sum(self.labels)}")
        print(f"No-drone segments: {len(self.labels) - sum(self.labels)}")
        print(f"Labels saved to: {self.output_dir / 'labels.csv'}")
        print("=" * 60)
    
    def save_labels(self):
        """Save labels to CSV file."""
        csv_path = self.output_dir / 'labels.csv'
        
        with open(csv_path, 'w') as f:
            f.write('filename,label\n')
            for filename, label in zip(self.segments, self.labels):
                f.write(f'{filename},{label}\n')


def list_devices():
    """List available audio devices."""
    print("Available audio devices:")
    print("-" * 40)
    devices = sd.query_devices()
    for i, device in enumerate(devices):
        if device['max_input_channels'] > 0:
            print(f"  [{i}] {device['name']}")
            print(f"      Channels: {device['max_input_channels']}, "
                  f"Sample rate: {device['default_samplerate']}")
    print()


def main():
    parser = argparse.ArgumentParser(description='Collect audio samples for VARTA')
    parser.add_argument('--device', type=int, default=None, help='Audio device index')
    parser.add_argument('--duration', type=int, default=300, help='Recording duration (seconds)')
    parser.add_argument('--output', type=str, default='samples/', help='Output directory')
    parser.add_argument('--list-devices', action='store_true', help='List audio devices and exit')
    
    args = parser.parse_args()
    
    if args.list_devices:
        list_devices()
        return
    
    collector = AudioCollector(
        device=args.device,
        output_dir=args.output,
        duration=args.duration
    )
    
    collector.run()


if __name__ == '__main__':
    main()
