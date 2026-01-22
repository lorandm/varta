# VARTA Assembly Guide

Step-by-step instructions to build your acoustic drone detector.

## Overview

**Estimated time:** 2-4 hours  
**Difficulty:** Intermediate (soldering required)  
**Tools needed:**
- Soldering iron + solder
- Wire strippers
- Small screwdrivers (Phillips, flat)
- Multimeter
- Hot glue gun (optional)
- 3D printer access (or order printed parts)

## Bill of Materials

### Electronics

| Component | Qty | Notes | Approx. Cost |
|-----------|-----|-------|--------------|
| ESP32-S3-WROOM-1-N16R8 | 1 | DevKitC-1 recommended | $10 |
| INMP441 MEMS Microphone | 4 | I2S digital output | $12 |
| SSD1306 OLED Display | 1 | 0.96" 128x64 I2C | $4 |
| WS2812B LED Ring | 1 | 8 LED, ~32mm diameter | $3 |
| Active Piezo Buzzer | 1 | 3-5V | $1 |
| Coin Vibration Motor | 1 | 3V DC | $2 |
| 18650 Battery Holder | 1 | 2S with BMS | $3 |
| 18650 Cells | 2 | 2500mAh+ each | $8 |
| TP4056 USB-C Charger | 1 | With protection | $2 |
| MT3608 Boost Converter | 1 | Adjustable, set to 5V | $1 |
| Slide Switch | 1 | SPST | $0.50 |
| Tactile Button | 1 | 6mm | $0.25 |
| Resistors | 2 | 10K (voltage divider) | $0.10 |

**Subtotal electronics: ~$47**

### Hardware

| Item | Qty | Notes |
|------|-----|-------|
| M3 x 8mm screws | 8 | Stainless preferred |
| M3 x 6mm screws | 4 | PCB mounting |
| M2 x 4mm screws | 8 | Mic boards |
| M3 nuts | 4 | For clip |
| Silicone wire (22-26 AWG) | 1m | Flexible |
| Heat shrink tubing | Assorted | |
| Double-sided tape | Small roll | Foam type |

### Enclosure

| Part | Qty | Notes |
|------|-----|-------|
| Body (3D printed) | 1 | PETG recommended |
| Lid (3D printed) | 1 | PETG recommended |
| Clip (3D printed) | 1 | Optional |
| Acoustic vent membrane | 4 | 10mm diameter |
| Clear acrylic window | 1 | For display (optional) |

---

## Step 1: Prepare Components

### 1.1 Print Enclosure Parts

Print the following from `/enclosure/`:
- `varta_body.stl`
- `varta_lid.stl`
- `varta_clip.stl` (optional)

Settings: PETG, 0.2mm layers, 20% infill, supports on.

### 1.2 Test Electronics

Before soldering, verify each component works:

```
ESP32-S3: Connect USB, upload blink sketch
OLED: Connect I2C, run display test
Microphones: Connect one at a time, verify I2S signal
LED Ring: Connect data pin, run rainbow test
```

### 1.3 Prepare Wires

Cut and strip wires for:
- Power distribution (red/black, 22 AWG)
- I2S bus (4 colors, 26 AWG)
- I2C bus (2 colors, 26 AWG)
- Signal wires (various, 26 AWG)

---

## Step 2: Assemble Power System

### 2.1 Battery Holder

1. Solder wires to battery holder terminals
2. Connect BMS if not integrated
3. Verify polarity with multimeter

### 2.2 Charging Circuit

```
Battery+ ──→ TP4056 B+ ──→ (charging managed internally)
Battery- ──→ TP4056 B- 
            TP4056 OUT+ ──→ Power switch ──→ System+
            TP4056 OUT- ──→ System-
```

### 2.3 Voltage Regulation

```
System+ ──→ MT3608 IN+ ──→ MT3608 OUT+ ──→ 5V rail
System- ──→ MT3608 IN- ──→ MT3608 OUT- ──→ GND rail
```

**Important:** Adjust MT3608 output to exactly 5.0V before connecting ESP32!

### 2.4 Battery Monitoring

Create voltage divider for ADC:
```
Battery+ ──┬── 10K resistor ──┬── 10K resistor ──→ GND
           │                  │
           │                  └── To ESP32 GPIO1
           │
           └── (Also to power switch)
```

This gives 0-4.2V input → 0-2.1V to ADC (safe for 3.3V input).

---

## Step 3: Microphone Array

### 3.1 INMP441 Pinout

```
VDD ─── 3.3V
GND ─── GND
SD  ─── Data out (to ESP32)
WS  ─── Word Select (shared)
SCK ─── Serial Clock (shared)
L/R ─── Left/Right select
```

### 3.2 Wiring Diagram

```
              ┌─────────────────────────────────────────────┐
              │                 ESP32-S3                    │
              │                                             │
Mic1 ─SD──────┼──GPIO4                                      │
Mic2 ─SD──────┼──GPIO5                                      │
Mic3 ─SD──────┼──GPIO6                                      │
Mic4 ─SD──────┼──GPIO7                                      │
              │                                             │
All Mics ─WS──┼──GPIO15 (shared Word Select)               │
All Mics ─SCK─┼──GPIO14 (shared Serial Clock)              │
              │                                             │
All Mics ─VDD─┼──3.3V                                      │
All Mics ─GND─┼──GND                                       │
              └─────────────────────────────────────────────┘
```

### 3.3 L/R Channel Selection

For 4 independent channels, set L/R pins:
- Mic1: L/R → GND (Left channel)
- Mic2: L/R → VDD (Right channel)
- Mic3: L/R → GND
- Mic4: L/R → VDD

**Note:** Current firmware reads Mic1 only due to I2S limitations. Full 4-mic support requires I2S TDM mode or multiple I2S peripherals. See firmware TODO.

### 3.4 Mount Microphones in Lid

1. Position mic boards over the 4 ports
2. Ensure mic hole aligns with acoustic port
3. Secure with M2 screws
4. Apply acoustic vent membrane over each port (weatherproofing)

---

## Step 4: Main Board Assembly

### 4.1 ESP32-S3 Connections

| ESP32 Pin | Connection | Notes |
|-----------|------------|-------|
| GPIO4 | Mic1 SD | I2S Data |
| GPIO5 | Mic2 SD | I2S Data |
| GPIO6 | Mic3 SD | I2S Data |
| GPIO7 | Mic4 SD | I2S Data |
| GPIO14 | All Mics SCK | I2S Clock |
| GPIO15 | All Mics WS | I2S Word Select |
| GPIO21 | OLED SDA | I2C Data |
| GPIO22 | OLED SCL | I2C Clock |
| GPIO48 | LED Ring Data | WS2812B |
| GPIO38 | Buzzer | Active high |
| GPIO39 | Vibration Motor | Via transistor |
| GPIO0 | Button | Boot button, active low |
| GPIO1 | Battery ADC | Voltage divider |
| 5V | Power input | From MT3608 |
| GND | Ground | Common ground |

### 4.2 Display Wiring

```
OLED VCC ─── 3.3V
OLED GND ─── GND
OLED SDA ─── GPIO21
OLED SCL ─── GPIO22
```

### 4.3 LED Ring Wiring

```
LED VCC ─── 5V (needs full 5V for bright LEDs)
LED GND ─── GND
LED DIN ─── GPIO48
```

### 4.4 Alert Outputs

**Buzzer (direct drive OK for small buzzers):**
```
Buzzer+ ─── GPIO38
Buzzer- ─── GND
```

**Vibration Motor (use transistor for higher current):**
```
                    ┌─── Motor+ ─── 5V
2N2222 or           │
similar    Base ────┼─── 1K resistor ─── GPIO39
           Emitter ─┴─── GND
           Collector ─── Motor-
```

---

## Step 5: Final Assembly

### 5.1 Install Components in Body

1. **Battery holder:** Slide between guide rails, secure with hot glue or double-sided tape

2. **Charging board:** Mount near USB port cutout

3. **Power switch:** Mount in side wall (drill hole if needed)

4. **Main PCB/ESP32:** Mount on standoffs with M3 screws

5. **Boost converter:** Mount with double-sided tape, away from mics

### 5.2 Connect All Wiring

1. Connect power system (battery → charger → switch → boost → ESP32)
2. Connect I2S bus from lid mics to ESP32
3. Connect I2C bus (OLED)
4. Connect LED ring data
5. Connect buzzer and vibration motor
6. Connect button
7. Connect battery monitoring

### 5.3 Route and Secure Wires

- Use zip ties or hot glue to secure wires
- Keep power and signal wires separated
- Leave slack for lid opening/closing
- Ensure no wires block battery removal

### 5.4 Install Lid Components

Components mounted in lid:
- 4x Microphones (already installed)
- OLED display (position in window)
- LED ring (position in cutout)
- Button (if lid-mounted)

### 5.5 Close and Secure

1. Carefully close lid, guiding wires
2. Insert M3 screws in corners
3. Tighten evenly (don't over-tighten plastic)
4. Apply silicone gasket if weatherproofing

---

## Step 6: Flash Firmware

### 6.1 Install PlatformIO

```bash
pip install platformio
```

### 6.2 Build and Upload

```bash
cd firmware
pio run
pio run --target upload
```

### 6.3 Monitor Serial Output

```bash
pio device monitor
```

You should see:
```
=== VARTA Acoustic Drone Detector ===
Initializing...
I2S configured successfully
Display initialized
LEDs initialized
Loading ML model...
Model loaded. Input shape: [32, 128, 1]
Initialization complete. Entering SCAN mode.
```

---

## Step 7: Calibration

### 7.1 Initial Power-On

1. Insert charged batteries
2. Turn on power switch
3. Wait for startup sequence (LED sweep)
4. Display shows "VARTA READY"

### 7.2 Ambient Noise Calibration

1. Take device to deployment location
2. Ensure no drones present
3. Long-press button (3 seconds)
4. Wait 30 seconds while device records ambient noise
5. "CALIBRATION COMPLETE" displayed

### 7.3 Verify Operation

1. Device should show "SCAN" mode
2. LED ring shows subtle blue breathing
3. Display shows confidence near 0%
4. Test with drone recording (play on speaker) to verify detection

---

## Troubleshooting

### No Power

- Check battery charge (multimeter on battery terminals)
- Check power switch
- Verify MT3608 output is 5V
- Check for shorts

### Display Not Working

- Verify I2C address (0x3C default, some are 0x3D)
- Check SDA/SCL connections
- Try I2C scanner sketch

### No Audio Detection

- Check I2S wiring
- Verify mic boards have power (3.3V)
- Check L/R pin settings
- Try one mic at a time

### False Positives

- Re-run calibration in deployment environment
- Adjust DETECTION_THRESHOLD_DB in config.h
- Train custom ML model with local noise

### Short Battery Life

- Check for short circuits (feel for hot components)
- Reduce LED brightness
- Increase sleep intervals
- Use higher capacity cells

---

## Maintenance

### Regular Checks

- Clean mic ports (blow out dust)
- Check battery connections
- Verify gaskets if weatherproofed
- Re-calibrate if moving to new location

### Battery Care

- Charge before extended storage
- Don't discharge below 6.0V
- Replace cells every 2-3 years
- Store at ~50% charge if not using

---

## Safety Notes

- Li-ion batteries can be dangerous if damaged or short-circuited
- Use protected cells with built-in BMS
- Don't charge unattended
- Don't expose to extreme heat
- Dispose of batteries properly

---

**Congratulations!** Your VARTA drone detector is ready for field testing.

For training a custom ML model, see `ml/training/README.md`.
