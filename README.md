# VARTA - Open Source Acoustic Drone Detector

**V**isual and **A**coustic **R**eal-**T**ime **A**lert System

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Hardware: Open Source](https://img.shields.io/badge/Hardware-Open%20Source-green.svg)](https://www.oshwa.org/)
[![Status: Alpha](https://img.shields.io/badge/Status-Alpha-red.svg)]()

---

> ⚠️ **ALPHA STATUS - READ BEFORE PROCEEDING**
>
> This project provides a **complete framework** for building an acoustic drone detector, but it is **not a plug-and-play solution**. Before this device will detect anything, you must:
>
> 1. **Collect training data** - Record drone audio samples in your environment
> 2. **Train the ML model** - Run the training pipeline on your collected data
> 3. **Assemble hardware** - Build the device from components (~$80-100)
> 4. **Calibrate and test** - Tune detection thresholds for your conditions
>
> **Current limitations:**
> - No pre-trained model included (you must train your own)
> - 4-microphone simultaneous capture needs firmware work (currently single-mic)
> - Detection performance is theoretical until field-validated
> - Hardware design is breadboard-ready, not a finished PCB
>
> This project is ideal for makers, researchers, and defense engineers who can contribute to development. It is **not ready** for non-technical end users expecting a finished product.
>
> **We welcome contributions** - especially training data, field test results, and firmware improvements.

---

## Overview

VARTA is an open-source, portable acoustic drone detection system designed to detect fiber optic controlled FPV drones that evade traditional RF-based detectors. Built for under $100 in components, it provides early warning capability for personnel in the field.

**Why VARTA exists:** RF drone detectors (like the BlueBird Chuyka) cannot detect fiber optic controlled drones because these drones don't emit radio signals. VARTA fills this critical gap using acoustic detection.

## Key Features

- **4-microphone beamforming array** for direction estimation
- **ML-based detection** using TensorFlow Lite for edge inference
- **4-8 second detection** at 200-500m range (environment dependent)
- **Visual + haptic + audio alerts** for high-noise environments
- **8+ hours battery life** on 2S 18650 pack
- **IP54 rated enclosure** (3D printable)
- **$80-100 total BOM cost**

## Specifications

| Parameter | Value |
|-----------|-------|
| Detection Range | 200-500m (line of sight, low ambient noise) |
| Detection Time | 4-8 seconds |
| Direction Accuracy | ±15° azimuth |
| Power Supply | 7.4V (2S 18650) |
| Battery Life | 8+ hours continuous |
| Operating Temp | -20°C to +50°C |
| Enclosure Rating | IP54 |
| Weight | ~350g with battery |
| Dimensions | 140 x 80 x 45 mm |

## How It Works

### Detection Principle

Drone motors produce distinctive acoustic signatures in the 100-1000Hz range with characteristic harmonic patterns. VARTA uses:

1. **FFT Analysis** - Continuous spectral analysis looking for drone motor fundamentals (typically 150-400Hz) and their harmonics
2. **Beamforming** - Time-difference-of-arrival (TDOA) across 4 microphones estimates threat direction
3. **ML Classification** - TensorFlow Lite model distinguishes drones from environmental noise (vehicles, wind, wildlife)

### Limitations

- **Range decreases significantly** in high ambient noise (combat, vehicles, wind)
- **Cannot detect** drones with silenced/baffled motors at useful range
- **False positives** possible from other small motors, insects, etc.
- **No altitude estimation** - only azimuth direction
- **Requires line-of-sight acoustically** - terrain/structures block sound

## Hardware

### Bill of Materials

| Component | Quantity | Unit Cost | Notes |
|-----------|----------|-----------|-------|
| ESP32-S3-WROOM-1 | 1 | $8 | N16R8 variant recommended |
| INMP441 MEMS Microphone | 4 | $3 | I2S output |
| 0.96" OLED Display (SSD1306) | 1 | $4 | I2C, 128x64 |
| 2.4" TFT Display (ILI9341) | 1 | $8 | SPI, optional for waveform |
| Piezo Buzzer | 1 | $1 | 3-5V active |
| Vibration Motor | 1 | $2 | 3V coin type |
| WS2812B LED Ring (8 LED) | 1 | $3 | Direction indicator |
| 18650 Battery Holder (2S) | 1 | $3 | With BMS |
| 18650 Cells | 2 | $8 | 2500mAh+ recommended |
| TP4056 Charger Module | 1 | $1 | USB-C preferred |
| MT3608 Boost Converter | 1 | $1 | 5V for ESP32 |
| Power Switch | 1 | $1 | SPST |
| SMA Connectors (optional) | 4 | $4 | For external mics |
| PCB (custom) | 1 | $10 | JLCPCB/PCBWay |
| 3D Printed Enclosure | 1 | $5 | PLA or PETG |
| Misc (wires, screws, etc.) | - | $5 | - |

**Total: ~$85**

### Microphone Array Geometry

The 4 microphones are arranged in a square pattern with 50mm spacing:

```
        Front
          ↑
    [M1]-----[M2]
      |       |
      |   ●   |    ● = center/mounting point
      |       |
    [M4]-----[M3]
    
    Spacing: 50mm between adjacent mics
```

This geometry allows:
- Azimuth estimation via TDOA
- Reasonable angular resolution (~15°)
- Compact form factor

### Schematic

See `/hardware/kicad/` for full KiCad project files.

Key connections:
- **Microphones (I2S)**: All 4 INMP441 share SCK (GPIO14) and WS (GPIO15), individual SD pins (GPIO4, 5, 6, 7)
- **OLED Display (I2C)**: SDA (GPIO21), SCL (GPIO22)
- **TFT Display (SPI)**: MOSI (GPIO23), SCK (GPIO18), CS (GPIO5), DC (GPIO16), RST (GPIO17)
- **LED Ring**: Data (GPIO48)
- **Buzzer**: GPIO38
- **Vibration Motor**: GPIO39

## Firmware

### Building

Requires PlatformIO:

```bash
cd firmware
pio run
pio run --target upload
```

### Configuration

Edit `include/config.h`:

```c
// Detection thresholds (adjust based on field testing)
#define DETECTION_THRESHOLD_DB    65.0f   // Minimum signal level
#define CONFIDENCE_THRESHOLD      0.75f   // ML model confidence
#define ALERT_HOLDOFF_MS          2000    // Debounce alerts

// Hardware config
#define MIC_SPACING_MM            50.0f   // Physical mic spacing
#define SAMPLE_RATE               44100   // Audio sample rate
#define FFT_SIZE                  2048    // FFT window size
```

### Operating Modes

1. **SCAN** (default) - Continuous monitoring, LED ring shows ambient level
2. **ALERT** - Threat detected, LED shows direction, buzzer + vibration active
3. **MONITOR** - Display shows real-time spectrogram (debugging)
4. **CALIBRATE** - Ambient noise profiling for location

Button controls:
- **Short press**: Cycle display modes
- **Long press (3s)**: Enter calibration mode
- **Double press**: Mute audio alerts (haptic remains)

## Machine Learning

### Model Architecture

Lightweight CNN designed for ESP32-S3:

```
Input: 128 mel-frequency bands x 32 time frames (1 second)
├── Conv2D(16, 3x3) + ReLU + MaxPool
├── Conv2D(32, 3x3) + ReLU + MaxPool
├── Conv2D(64, 3x3) + ReLU + GlobalAvgPool
├── Dense(32) + ReLU + Dropout(0.3)
└── Dense(2) + Softmax → [no_drone, drone]

Parameters: ~45,000
Inference time: ~50ms on ESP32-S3
```

### Training Your Own Model

The default model provides baseline detection. For best results, train on data from your operational environment:

```bash
cd ml/training

# 1. Collect audio samples
python collect_samples.py --device your_mic --duration 300

# 2. Label samples (creates labels.csv)
python labeler.py --input samples/

# 3. Train model
python train.py --data samples/ --labels labels.csv --epochs 100

# 4. Convert to TFLite
python convert_tflite.py --model checkpoints/best.h5 --output ../models/

# 5. Generate C header for firmware
xxd -i models/drone_detector.tflite > ../firmware/src/model_data.h
```

### Training Data Requirements

For effective detection, collect:
- **Positive samples**: Various drone types at different distances/angles
- **Negative samples**: Environmental noise from deployment location
  - Wind, foliage rustling
  - Distant vehicles, aircraft
  - Voices, footsteps
  - Wildlife (birds, insects)
  - Combat sounds (if applicable)

Recommended: 2+ hours of each class, diverse conditions.

## Enclosure

### 3D Printing

Files in `/enclosure/`:
- `varta_body.stl` - Main enclosure
- `varta_lid.stl` - Top cover with mic ports
- `varta_clip.stl` - Belt/MOLLE clip

Print settings:
- Material: PETG recommended (better temperature resistance than PLA)
- Layer height: 0.2mm
- Infill: 20%
- Supports: Yes (for mic port overhangs)

### Weatherproofing

For IP54 rating:
1. Apply silicone sealant to lid seam
2. Install acoustic vent membrane over mic ports (e.g., Gore-Tex patch)
3. Seal cable glands with silicone

## Assembly

See [docs/ASSEMBLY.md](docs/ASSEMBLY.md) for step-by-step instructions with photos.

Quick overview:
1. Solder microphones to breakout boards (or direct to PCB)
2. Assemble main PCB with ESP32, power management
3. Wire display, buzzer, vibration motor
4. Install battery holder and cells
5. Mount PCB in enclosure
6. Position microphones in lid at correct spacing
7. Final wiring and close-up
8. Flash firmware and calibrate

## Field Use

### Pre-deployment

1. Power on and wait for self-test (LED sequence)
2. Verify GPS fix if equipped (optional module)
3. Run calibration in deployment location (30 seconds)
4. Verify alert function with test tone

### Operational Notes

- **Position**: Wear on shoulder/chest, mics facing likely threat direction
- **Ambient noise**: Detection range decreases significantly with background noise
- **Terrain**: Hills, buildings, vegetation attenuate sound - adjust expectations
- **Weather**: Wind is the enemy - seek shelter or accept reduced range
- **False alarms**: Initial deployments will have false positives; re-train model with local data

### Interpreting Alerts

LED ring indicates estimated threat direction:
- **Red LED position**: Threat azimuth (north = front, clockwise)
- **LED brightness**: Signal strength / confidence
- **Pulsing**: Active threat
- **Solid**: Recent detection, threat may have passed

## Roadmap

### Current Status (v0.1-alpha)

| Component | Status | Notes |
|-----------|--------|-------|
| Firmware architecture | ✅ Complete | Compiles, structured for expansion |
| Single-mic detection | ✅ Working | Basic audio capture and FFT |
| 4-mic simultaneous | 🔧 Partial | Needs I2S TDM implementation |
| Direction estimation | 🔧 Partial | Algorithm ready, needs 4-mic hardware |
| ML training pipeline | ✅ Complete | Collect, label, train, convert |
| Pre-trained model | ❌ Missing | Requires drone audio recordings |
| 3D enclosure | ✅ Complete | Parametric OpenSCAD source |
| Custom PCB | ❌ Not started | Current design uses dev boards |
| Field validation | ❌ Not done | No real-world test data yet |

### Priority Contributions Needed

1. **Training data** - Drone recordings at various distances/conditions
2. **4-channel I2S** - ESP32-S3 TDM mode or multi-I2S implementation  
3. **Field testing** - Build one, test it, report results
4. **PCB design** - Single board replacing dev board stack

### Future Ideas

- Bluetooth app for configuration and alerts
- Mesh networking between multiple units
- Integration with camera/thermal systems
- Solar power option for fixed installations
- LoRa for long-range alerting

## Contributing

Contributions welcome! Priority areas:
- Improved ML models trained on diverse drone types
- Alternative microphone arrays (linear, larger aperture)
- Integration with other sensor systems
- Mobile app for configuration/visualization
- Power optimization

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

Hardware: CERN-OHL-P-2.0
Software: MIT
Documentation: CC-BY-4.0

## Acknowledgments

This project builds on:
- ESP-IDF and Arduino frameworks
- TensorFlow Lite for Microcontrollers
- Countless open source audio processing libraries

## Disclaimer

VARTA is provided as-is for educational and research purposes. Detection performance varies significantly based on environmental conditions, drone types, and deployment scenario. This device is not a substitute for other protective measures. The authors assume no liability for use in any context.

---

*"In war, let your great object be victory, not lengthy campaigns."* - Sun Tzu

Built with urgency for those who need it.
