# VARTA Enclosure

3D printable enclosure for the acoustic drone detector.

## Files

| File | Description |
|------|-------------|
| `enclosure.scad` | Parametric OpenSCAD source |
| `varta_body.stl` | Main body (pre-generated) |
| `varta_lid.stl` | Top cover with mic ports |
| `varta_clip.stl` | Belt/MOLLE attachment clip |

## Generating STL Files

If you need to modify the design, edit `enclosure.scad` and regenerate:

```bash
# Install OpenSCAD first
# Ubuntu: sudo apt install openscad
# Mac: brew install openscad

# Generate all parts
openscad -o varta_body.stl -D 'part="body"' enclosure.scad
openscad -o varta_lid.stl -D 'part="lid"' enclosure.scad  
openscad -o varta_clip.stl -D 'part="clip"' enclosure.scad
```

## Print Settings

### Recommended Settings

| Parameter | Value |
|-----------|-------|
| Material | PETG (preferred) or PLA |
| Layer Height | 0.2mm |
| Infill | 20% |
| Perimeters | 3 |
| Top/Bottom Layers | 4 |
| Supports | Yes (for mic port overhangs) |

### Material Notes

**PETG** (Recommended)
- Better temperature resistance (-20°C to +70°C)
- More impact resistant
- Slight flexibility reduces cracking
- Harder to print (stringing)

**PLA**
- Easier to print
- More rigid
- Lower temperature tolerance (<50°C)
- Can warp in hot environments

**ABS**
- Good temperature resistance
- Requires enclosed printer
- More post-processing (fumes, warping)

## Assembly

### Hardware Needed

| Item | Quantity | Notes |
|------|----------|-------|
| M3 x 8mm screws | 4 | Lid attachment |
| M3 x 6mm screws | 4 | PCB mounting |
| M2 x 4mm screws | 4 | Mic board mounting |
| M3 nuts | 4 | For clip attachment |
| Silicone gasket (optional) | 1 | For IP54 sealing |

### Assembly Steps

1. **Prepare body**
   - Clean up any support material
   - Check screw boss threads (may need to tap)

2. **Install PCB standoffs**
   - Insert M3 screws from inside
   - Secure PCB with additional nuts or use self-tapping screws

3. **Mount components in lid**
   - Position microphone boards over ports
   - Secure with M2 screws
   - Install display bezel (if using)
   - Install LED ring

4. **Wire everything**
   - Connect microphones to main PCB
   - Connect display, LEDs, buzzer, vibration motor
   - Install battery holder

5. **Final assembly**
   - Apply silicone gasket to lid edge (optional)
   - Close lid and secure with M3 screws
   - Attach belt clip if needed

## Weatherproofing (IP54)

For outdoor use, add weatherproofing:

1. **Lid seal**
   - Apply 2mm silicone gasket to lid edge
   - Or use RTV silicone sealant

2. **Microphone ports**
   - Install acoustic vent membranes (e.g., GORE-TEX patches)
   - These allow sound through while blocking water
   - Available from speaker repair suppliers

3. **Display window**
   - Install clear acrylic or polycarbonate window
   - Seal edges with silicone

4. **USB port**
   - Install rubber plug when not charging
   - Or use waterproof USB connector

## Customization

### Key Parameters in OpenSCAD

```openscad
// Adjust these to fit your components:

mic_spacing = 50;       // Distance between mics
mic_hole_dia = 8;       // INMP441 breakout size
oled_width = 28;        // Display cutout
led_ring_dia = 32;      // LED ring size

// Overall size:
body_length = 140;
body_width = 80;
body_height = 35;
```

### Adding Features

To add new cutouts or mounting points:

1. Open `enclosure.scad` in OpenSCAD
2. Find the `body()` or `lid()` module
3. Add `translate()` and primitive shapes
4. Use `difference()` for cutouts
5. Preview and export STL

### Alternative Mounting Options

**Helmet mount**
- Design a curved plate to match helmet profile
- Use Velcro or screw attachment

**Vehicle mount**
- Add suction cup or magnetic base
- Consider vibration damping

**Pole mount**
- Design clamp attachment
- Good for perimeter monitoring

## Troubleshooting

### Parts Don't Fit

- Check `fit_tolerance` parameter (default 0.3mm)
- Your printer may need different value
- Print a test piece first

### Mic Ports Wrong Size

- Measure your actual breakout boards
- Adjust `mic_hole_dia` and `mic_mount_dia`

### Screws Don't Thread

- May need to tap holes with M3 tap
- Or use self-tapping screws
- Or heat-set inserts (recommended)

### Water Ingress

- Check gasket seating
- Verify acoustic membranes are installed
- Apply additional sealant to seams

## License

CERN-OHL-P-2.0 (Permissive Open Hardware License)
