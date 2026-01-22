# VARTA ML Training Pipeline

This directory contains everything needed to train your own drone detection model.

## Requirements

```bash
pip install -r requirements.txt
```

## Quick Start

### 1. Collect Audio Samples

Record audio in environments where you'll deploy the detector:

```bash
# List available microphones
python collect_samples.py --list-devices

# Record for 5 minutes (press SPACE when you hear a drone)
python collect_samples.py --device 0 --duration 300 --output samples/
```

**Tips for good training data:**
- Record in actual deployment locations
- Include various distances (50m, 100m, 200m, 500m)
- Record different times of day (temperature affects sound propagation)
- Include various "negative" sounds: wind, vehicles, voices, wildlife
- Mark drone segments in real-time for accurate labels

### 2. Label Samples (if needed)

If you have unlabeled audio files, use the interactive labeler:

```bash
python labeler.py --input samples/

# Check labeling progress
python labeler.py --input samples/ --stats
```

### 3. Train Model

```bash
python train.py \
    --data samples/ \
    --labels samples/labels.csv \
    --epochs 100 \
    --output output/
```

Training will:
- Split data into train/val/test sets
- Train a lightweight CNN
- Save best model to `output/checkpoints/best.h5`
- Generate classification report

### 4. Convert to TFLite

```bash
python convert_tflite.py \
    --model output/checkpoints/best.h5 \
    --output ../models/ \
    --generate-header
```

This creates:
- `drone_detector_quant.tflite` - Quantized model for ESP32
- `model_data.h` - C header for firmware

### 5. Update Firmware

Copy the generated header to the firmware:

```bash
cp ../models/model_data.h ../firmware/src/model_data.h
```

Rebuild and flash:

```bash
cd ../firmware
pio run
pio run --target upload
```

## Training Data Guidelines

### Positive Samples (Drones)

Record various drone types at different distances and angles:

| Drone Type | Distance | Samples Needed |
|------------|----------|----------------|
| FPV racing | 50-200m  | 200+ segments  |
| FPV camera | 50-500m  | 200+ segments  |
| DJI-style  | 100-500m | 100+ segments  |

### Negative Samples (Not Drones)

Critical for reducing false positives:

| Sound Source | Samples Needed |
|--------------|----------------|
| Wind/foliage | 300+ segments  |
| Vehicles     | 200+ segments  |
| Aircraft     | 100+ segments  |
| Voices       | 100+ segments  |
| Wildlife     | 100+ segments  |
| Silence      | 100+ segments  |
| Combat sounds| 200+ segments  |

### Data Augmentation

The training script applies automatic augmentation:
- Time shifting
- Noise injection
- Pitch shifting
- Time stretching

This helps with generalization but doesn't replace diverse real-world data.

## Model Architecture

The default model is a lightweight CNN designed for ESP32-S3:

```
Input: (32, 128, 1) - 32 time frames × 128 mel bins

Conv2D(16) → BatchNorm → ReLU → MaxPool
Conv2D(32) → BatchNorm → ReLU → MaxPool
Conv2D(64) → BatchNorm → ReLU → GlobalAvgPool
Dense(32) → ReLU → Dropout(0.3)
Dense(2) → Softmax

Total parameters: ~45,000
Inference time: ~50ms on ESP32-S3
```

## Customization

### Adjusting Detection Sensitivity

In `train.py`, modify:

```python
SAMPLE_RATE = 44100    # Match your hardware
N_MELS = 128           # Frequency resolution
DURATION = 1.0         # Detection window (seconds)
```

### Different Model Architectures

For better accuracy (larger model):
- Add more conv layers
- Increase filter counts
- Use attention mechanisms

For faster inference (smaller model):
- Reduce filter counts
- Use depthwise separable convolutions
- Decrease input resolution

### Transfer Learning

Start from a pre-trained audio model:

```python
# In train.py
from tensorflow.keras.applications import MobileNetV2

base_model = MobileNetV2(
    input_shape=(128, 128, 1),
    include_top=False,
    weights=None  # or use imagenet and adapt
)
```

## Troubleshooting

### Low Accuracy

- More training data needed (especially negatives)
- Try longer training (more epochs)
- Check class balance in dataset
- Verify audio quality (no clipping, correct sample rate)

### High False Positive Rate

- Add more negative samples from deployment environment
- Increase detection threshold in firmware
- Use multiple consecutive detections before alerting

### Model Too Large

- Enable quantization (default)
- Reduce model complexity
- Decrease input resolution (fewer mel bins)

### Inference Too Slow

- Use int8 quantization
- Reduce model depth
- Optimize ESP32-S3 settings (CPU frequency, etc.)

## Files

| File | Description |
|------|-------------|
| `collect_samples.py` | Real-time audio recording with marking |
| `labeler.py` | Interactive sample labeling tool |
| `train.py` | Model training script |
| `convert_tflite.py` | TFLite conversion for ESP32 |
| `requirements.txt` | Python dependencies |

## License

MIT License - see main project LICENSE
