#!/usr/bin/env python3
"""
VARTA - Audio Sample Labeler
Interactive tool to label audio samples for training.

Usage:
    python labeler.py --input samples/
    
This will play each unlabeled audio file and prompt for classification.
"""

import argparse
import os
import csv
from pathlib import Path

import numpy as np
import soundfile as sf

# Try to import sounddevice for playback
try:
    import sounddevice as sd
    PLAYBACK_AVAILABLE = True
except ImportError:
    PLAYBACK_AVAILABLE = False
    print("Warning: sounddevice not available. Audio playback disabled.")


def play_audio(filepath):
    """Play audio file."""
    if not PLAYBACK_AVAILABLE:
        print(f"  (Playback not available - file: {filepath})")
        return
    
    try:
        data, sr = sf.read(filepath)
        sd.play(data, sr)
        sd.wait()
    except Exception as e:
        print(f"  Error playing audio: {e}")


def load_existing_labels(labels_path):
    """Load existing labels from CSV."""
    labels = {}
    if labels_path.exists():
        with open(labels_path, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                labels[row['filename']] = int(row['label'])
    return labels


def save_labels(labels, labels_path):
    """Save labels to CSV."""
    with open(labels_path, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['filename', 'label'])
        for filename, label in sorted(labels.items()):
            writer.writerow([filename, label])


def get_audio_files(input_dir):
    """Get list of audio files in directory."""
    extensions = ['.wav', '.mp3', '.flac', '.ogg']
    files = []
    
    for ext in extensions:
        files.extend(input_dir.glob(f'*{ext}'))
    
    return sorted(files)


def show_waveform(filepath):
    """Show simple ASCII waveform."""
    try:
        data, sr = sf.read(filepath)
        if len(data.shape) > 1:
            data = data[:, 0]  # Mono
        
        # Downsample for display
        window = len(data) // 60
        if window > 0:
            peaks = []
            for i in range(0, len(data), window):
                chunk = data[i:i+window]
                peaks.append(np.max(np.abs(chunk)))
            
            # Normalize and display
            max_peak = max(peaks) if max(peaks) > 0 else 1
            print("  Waveform: ", end='')
            for p in peaks:
                height = int((p / max_peak) * 8)
                chars = ' ▁▂▃▄▅▆▇█'
                print(chars[min(height, 8)], end='')
            print()
    except Exception as e:
        print(f"  (Could not display waveform: {e})")


def label_samples(input_dir, output_labels):
    """Interactive labeling session."""
    input_dir = Path(input_dir)
    labels_path = input_dir / output_labels
    
    # Load existing labels
    labels = load_existing_labels(labels_path)
    print(f"Loaded {len(labels)} existing labels")
    
    # Get audio files
    audio_files = get_audio_files(input_dir)
    print(f"Found {len(audio_files)} audio files")
    
    # Filter to unlabeled
    unlabeled = [f for f in audio_files if f.name not in labels]
    print(f"Unlabeled files: {len(unlabeled)}")
    print()
    
    if not unlabeled:
        print("All files are already labeled!")
        return
    
    print("=" * 60)
    print("VARTA Audio Labeler")
    print("=" * 60)
    print()
    print("Commands:")
    print("  1 or d  - Label as DRONE")
    print("  0 or n  - Label as NO DRONE")
    print("  r       - Replay audio")
    print("  s       - Skip (leave unlabeled)")
    print("  q       - Quit and save")
    print()
    
    labeled_count = 0
    
    for i, filepath in enumerate(unlabeled):
        print(f"\n[{i+1}/{len(unlabeled)}] {filepath.name}")
        show_waveform(filepath)
        
        # Play audio
        play_audio(filepath)
        
        while True:
            response = input("  Label (1=drone, 0=no_drone, r=replay, s=skip, q=quit): ").strip().lower()
            
            if response in ['1', 'd', 'drone']:
                labels[filepath.name] = 1
                labeled_count += 1
                print("  → Labeled as DRONE")
                break
            elif response in ['0', 'n', 'no', 'nodrone']:
                labels[filepath.name] = 0
                labeled_count += 1
                print("  → Labeled as NO DRONE")
                break
            elif response == 'r':
                play_audio(filepath)
            elif response == 's':
                print("  → Skipped")
                break
            elif response == 'q':
                print("\nSaving and quitting...")
                save_labels(labels, labels_path)
                print(f"Saved {len(labels)} labels to {labels_path}")
                print(f"Labeled {labeled_count} files this session")
                return
            else:
                print("  Invalid input. Use 1/d, 0/n, r, s, or q")
    
    # Save at end
    save_labels(labels, labels_path)
    print()
    print("=" * 60)
    print(f"Labeling complete!")
    print(f"Total labels: {len(labels)}")
    print(f"Labeled this session: {labeled_count}")
    print(f"Labels saved to: {labels_path}")
    print("=" * 60)


def show_stats(input_dir, labels_file):
    """Show labeling statistics."""
    input_dir = Path(input_dir)
    labels_path = input_dir / labels_file
    
    labels = load_existing_labels(labels_path)
    audio_files = get_audio_files(input_dir)
    
    drone_count = sum(1 for l in labels.values() if l == 1)
    nodrone_count = sum(1 for l in labels.values() if l == 0)
    unlabeled_count = len(audio_files) - len(labels)
    
    print("=" * 40)
    print("Dataset Statistics")
    print("=" * 40)
    print(f"Total audio files: {len(audio_files)}")
    print(f"Labeled:           {len(labels)}")
    print(f"  - Drone:         {drone_count}")
    print(f"  - No drone:      {nodrone_count}")
    print(f"Unlabeled:         {unlabeled_count}")
    print("=" * 40)


def main():
    parser = argparse.ArgumentParser(description='Label audio samples for VARTA')
    parser.add_argument('--input', type=str, required=True, help='Directory containing audio samples')
    parser.add_argument('--output', type=str, default='labels.csv', help='Output labels file')
    parser.add_argument('--stats', action='store_true', help='Show statistics and exit')
    
    args = parser.parse_args()
    
    if args.stats:
        show_stats(args.input, args.output)
    else:
        label_samples(args.input, args.output)


if __name__ == '__main__':
    main()
