# VARTA Quick Start Guide

Get your acoustic drone detector running in under 2 hours.

## Prerequisites

**Tools needed:**
- Soldering iron
- Wire strippers
- Small screwdrivers
- 3D printer (or order prints)

**Software:**
- PlatformIO (VS Code extension or CLI)
- Python 3.8+ (for ML training)

## Step 1: Order Parts (5 minutes)

Order from the BOM in `/hardware/bom/BOM.csv`. Key items:
- ESP32-S3-DevKitC-1 (~$10)
- 4x INMP441 microphones (~$12)
- SSD1306 OLED display (~$4)
- WS2812B 8-LED ring (~$3)
- 2S 18650 battery pack (~$12)
- Power management boards (~$4)

**Total: ~$85**

## Step 2: Print Enclosure (2-4 hours)

Print from `/enclosure/` with PETG:
```bash
# Generate STLs if needed
openscad -o varta_body.stl -D 'part="body"' enclosure.scad
openscad -o varta_lid.stl -D 'part="lid"' enclosure.scad
```

Settings: 0.2mm layers, 20% infill, supports on.

## Step 3: Assemble (1-2 hours)

See `/docs/ASSEMBLY.md` for detailed instructions. Quick version:

1. **Power system:**
   - Battery holder → TP4056 charger → power switch → MT3608 boost (set to 5V) → ESP32

2. **Microphones:**
   - Wire all 4 INMP441 boards to shared I2S bus (SCK=GPIO14, WS=GPIO15)
   - Individual data lines to GPIO4, 5, 6, 7
   - Mount in lid with 50mm spacing

3. **Peripherals:**
   - OLED: SDA=GPIO21, SCL=GPIO22
   - LED ring: Data=GPIO48
   - Buzzer: GPIO38
   - Button: GPIO0

4. **Final assembly:**
   - Mount PCB in body
   - Connect all wiring
   - Install lid

## Step 4: Flash Firmware (5 minutes)

```bash
cd firmware
pip install platformio
pio run --target upload
pio device monitor  # Verify startup
```

You should see:
```
=== VARTA Acoustic Drone Detector ===
Initializing...
Initialization complete. Entering SCAN mode.
```

## Step 5: Calibrate (1 minute)

1. Power on device
2. Take to deployment location
3. Long-press button (3 seconds)
4. Wait 30 seconds for calibration
5. Done!

## Step 6: Train Custom Model (Optional, 1+ hours)

For best results, train on local audio:

```bash
cd ml/training
pip install -r requirements.txt

# Collect samples (press SPACE when drone present)
python collect_samples.py --device 0 --duration 300

# Train model
python train.py --data samples/ --labels samples/labels.csv --epochs 100

# Convert for ESP32
python convert_tflite.py --model output/checkpoints/best.h5 --output ../models/ --generate-header

# Update firmware
cp ../models/model_data.h ../firmware/src/model_data.h
cd ../firmware && pio run --target upload
```

## Usage

**Display modes:**
- SCAN: Normal monitoring (blue LED breathing)
- ALERT: Threat detected (red LED shows direction)
- MONITOR: Debug spectrogram view

**Controls:**
- Short press: Cycle display modes
- Double press: Mute audio (haptic only)
- Long press (3s): Calibration mode

**LED ring:**
- Blue breathing: Scanning
- Red direction: Threat detected (north = front)

## Troubleshooting

**No power:** Check battery, switch, boost converter output (should be 5V)

**Display blank:** Verify I2C address (0x3C or 0x3D), check wiring

**No detection:** Recalibrate in deployment location, check mic connections

**Too many false alarms:** Train custom model with local noise samples

## Need Help?

- Full docs: `/docs/ASSEMBLY.md`
- ML training: `/ml/training/README.md`
- Issues: GitHub Issues

---

*Built with urgency for those who need it.*
