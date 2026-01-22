#!/usr/bin/env python3
"""
VARTA ML Training Pipeline
Train a drone detection model from audio samples.

Usage:
    python train.py --data samples/ --labels labels.csv --epochs 100
"""

import os
import argparse
import numpy as np
import pandas as pd
from pathlib import Path
from datetime import datetime

import tensorflow as tf
from tensorflow import keras
from tensorflow.keras import layers, callbacks
from sklearn.model_selection import train_test_split
from sklearn.metrics import classification_report, confusion_matrix

# Audio processing
import librosa
import soundfile as sf

# Configuration
SAMPLE_RATE = 44100
N_FFT = 2048
HOP_LENGTH = 512
N_MELS = 128
DURATION = 1.0  # seconds per sample
N_TIME_FRAMES = 32

# Model input shape
INPUT_SHAPE = (N_TIME_FRAMES, N_MELS, 1)


def load_audio(filepath, sr=SAMPLE_RATE, duration=DURATION):
    """Load audio file and convert to correct format."""
    try:
        audio, file_sr = librosa.load(filepath, sr=sr, duration=duration, mono=True)
        
        # Pad if shorter than expected
        expected_samples = int(sr * duration)
        if len(audio) < expected_samples:
            audio = np.pad(audio, (0, expected_samples - len(audio)))
        
        return audio[:expected_samples]
    except Exception as e:
        print(f"Error loading {filepath}: {e}")
        return None


def extract_mel_spectrogram(audio, sr=SAMPLE_RATE):
    """Extract mel spectrogram from audio."""
    mel_spec = librosa.feature.melspectrogram(
        y=audio,
        sr=sr,
        n_fft=N_FFT,
        hop_length=HOP_LENGTH,
        n_mels=N_MELS,
        fmin=50,
        fmax=8000
    )
    
    # Convert to dB
    mel_spec_db = librosa.power_to_db(mel_spec, ref=np.max)
    
    # Normalize to 0-1
    mel_spec_norm = (mel_spec_db - mel_spec_db.min()) / (mel_spec_db.max() - mel_spec_db.min() + 1e-8)
    
    # Ensure correct shape (pad/truncate time dimension)
    if mel_spec_norm.shape[1] < N_TIME_FRAMES:
        pad_width = N_TIME_FRAMES - mel_spec_norm.shape[1]
        mel_spec_norm = np.pad(mel_spec_norm, ((0, 0), (0, pad_width)))
    elif mel_spec_norm.shape[1] > N_TIME_FRAMES:
        mel_spec_norm = mel_spec_norm[:, :N_TIME_FRAMES]
    
    # Transpose to (time, mel) and add channel dimension
    mel_spec_norm = mel_spec_norm.T  # (time, mel)
    mel_spec_norm = mel_spec_norm[..., np.newaxis]  # (time, mel, 1)
    
    return mel_spec_norm


def build_model(input_shape=INPUT_SHAPE, num_classes=2):
    """Build lightweight CNN for ESP32-S3."""
    model = keras.Sequential([
        # Input
        layers.Input(shape=input_shape),
        
        # Conv block 1
        layers.Conv2D(16, (3, 3), padding='same'),
        layers.BatchNormalization(),
        layers.ReLU(),
        layers.MaxPooling2D((2, 2)),
        
        # Conv block 2
        layers.Conv2D(32, (3, 3), padding='same'),
        layers.BatchNormalization(),
        layers.ReLU(),
        layers.MaxPooling2D((2, 2)),
        
        # Conv block 3
        layers.Conv2D(64, (3, 3), padding='same'),
        layers.BatchNormalization(),
        layers.ReLU(),
        layers.GlobalAveragePooling2D(),
        
        # Dense layers
        layers.Dense(32),
        layers.ReLU(),
        layers.Dropout(0.3),
        
        # Output
        layers.Dense(num_classes, activation='softmax')
    ])
    
    return model


def prepare_dataset(data_dir, labels_csv):
    """Load and prepare dataset from labeled audio files."""
    print(f"Loading dataset from {data_dir}")
    
    # Read labels
    labels_df = pd.read_csv(labels_csv)
    print(f"Found {len(labels_df)} labeled samples")
    
    X = []
    y = []
    
    for idx, row in labels_df.iterrows():
        filepath = os.path.join(data_dir, row['filename'])
        label = row['label']  # 0 = no_drone, 1 = drone
        
        if not os.path.exists(filepath):
            print(f"Warning: File not found: {filepath}")
            continue
        
        audio = load_audio(filepath)
        if audio is None:
            continue
        
        mel_spec = extract_mel_spectrogram(audio)
        X.append(mel_spec)
        y.append(label)
        
        if (idx + 1) % 100 == 0:
            print(f"Processed {idx + 1}/{len(labels_df)} files")
    
    X = np.array(X, dtype=np.float32)
    y = np.array(y, dtype=np.int32)
    
    print(f"Dataset shape: X={X.shape}, y={y.shape}")
    print(f"Class distribution: {np.bincount(y)}")
    
    return X, y


def augment_audio(audio, sr=SAMPLE_RATE):
    """Data augmentation for audio."""
    augmented = []
    
    # Original
    augmented.append(audio)
    
    # Time shift
    shift = np.random.randint(-sr // 10, sr // 10)
    shifted = np.roll(audio, shift)
    augmented.append(shifted)
    
    # Add noise
    noise = np.random.normal(0, 0.005, len(audio))
    noisy = audio + noise
    augmented.append(noisy)
    
    # Pitch shift (small)
    pitched = librosa.effects.pitch_shift(audio, sr=sr, n_steps=np.random.uniform(-2, 2))
    augmented.append(pitched)
    
    # Time stretch (small)
    rate = np.random.uniform(0.9, 1.1)
    stretched = librosa.effects.time_stretch(audio, rate=rate)
    if len(stretched) > len(audio):
        stretched = stretched[:len(audio)]
    else:
        stretched = np.pad(stretched, (0, len(audio) - len(stretched)))
    augmented.append(stretched)
    
    return augmented


def train(args):
    """Main training function."""
    print("=" * 60)
    print("VARTA Drone Detection Model Training")
    print("=" * 60)
    
    # Load dataset
    X, y = prepare_dataset(args.data, args.labels)
    
    if len(X) == 0:
        print("Error: No valid samples found!")
        return
    
    # Split dataset
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y
    )
    
    X_train, X_val, y_train, y_val = train_test_split(
        X_train, y_train, test_size=0.2, random_state=42, stratify=y_train
    )
    
    print(f"Train: {len(X_train)}, Val: {len(X_val)}, Test: {len(X_test)}")
    
    # Build model
    model = build_model()
    model.summary()
    
    # Count parameters
    total_params = model.count_params()
    print(f"\nTotal parameters: {total_params:,}")
    
    # Compile
    model.compile(
        optimizer=keras.optimizers.Adam(learning_rate=args.lr),
        loss='sparse_categorical_crossentropy',
        metrics=['accuracy']
    )
    
    # Callbacks
    checkpoint_dir = Path(args.output) / 'checkpoints'
    checkpoint_dir.mkdir(parents=True, exist_ok=True)
    
    cb_list = [
        callbacks.ModelCheckpoint(
            str(checkpoint_dir / 'best.h5'),
            monitor='val_accuracy',
            save_best_only=True,
            mode='max',
            verbose=1
        ),
        callbacks.EarlyStopping(
            monitor='val_loss',
            patience=15,
            restore_best_weights=True,
            verbose=1
        ),
        callbacks.ReduceLROnPlateau(
            monitor='val_loss',
            factor=0.5,
            patience=5,
            min_lr=1e-6,
            verbose=1
        ),
        callbacks.TensorBoard(
            log_dir=str(Path(args.output) / 'logs' / datetime.now().strftime('%Y%m%d-%H%M%S'))
        )
    ]
    
    # Train
    print("\nStarting training...")
    history = model.fit(
        X_train, y_train,
        validation_data=(X_val, y_val),
        epochs=args.epochs,
        batch_size=args.batch_size,
        callbacks=cb_list,
        verbose=1
    )
    
    # Evaluate on test set
    print("\nEvaluating on test set...")
    test_loss, test_acc = model.evaluate(X_test, y_test, verbose=0)
    print(f"Test accuracy: {test_acc:.4f}")
    print(f"Test loss: {test_loss:.4f}")
    
    # Detailed classification report
    y_pred = model.predict(X_test)
    y_pred_classes = np.argmax(y_pred, axis=1)
    
    print("\nClassification Report:")
    print(classification_report(y_test, y_pred_classes, 
                              target_names=['no_drone', 'drone']))
    
    print("\nConfusion Matrix:")
    print(confusion_matrix(y_test, y_pred_classes))
    
    # Save final model
    final_path = Path(args.output) / 'final_model.h5'
    model.save(str(final_path))
    print(f"\nModel saved to {final_path}")
    
    return model, history


def main():
    parser = argparse.ArgumentParser(description='Train VARTA drone detection model')
    parser.add_argument('--data', type=str, required=True, help='Directory containing audio samples')
    parser.add_argument('--labels', type=str, required=True, help='CSV file with labels')
    parser.add_argument('--output', type=str, default='./output', help='Output directory')
    parser.add_argument('--epochs', type=int, default=100, help='Number of epochs')
    parser.add_argument('--batch-size', type=int, default=32, help='Batch size')
    parser.add_argument('--lr', type=float, default=0.001, help='Learning rate')
    
    args = parser.parse_args()
    
    # Create output directory
    Path(args.output).mkdir(parents=True, exist_ok=True)
    
    train(args)


if __name__ == '__main__':
    main()
