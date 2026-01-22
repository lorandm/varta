# Contributing to VARTA

Thank you for your interest in contributing to the VARTA acoustic drone detector project. This document outlines how you can help.

## Ways to Contribute

### 1. Training Data

The most valuable contribution is diverse training data:

**Drone audio recordings needed:**
- FPV racing drones at various distances (50m to 500m)
- Camera/cinematography drones
- Military-style FPV drones
- Different motor types and propeller configurations
- Various environmental conditions (wind, temperature)

**Negative samples needed:**
- Combat environments (gunfire, explosions, vehicles)
- Urban noise (traffic, construction, crowds)
- Rural noise (wildlife, agriculture, wind)
- Industrial noise (machinery, HVAC, generators)

**How to submit:**
1. Use `ml/training/collect_samples.py` to record
2. Label samples with `ml/training/labeler.py`
3. Create a pull request with samples in `ml/data/community/`
4. Include metadata: location type, conditions, equipment used

### 2. Code Contributions

Priority areas:

**Firmware:**
- Multi-microphone I2S support (TDM mode)
- Power optimization (deep sleep modes)
- OTA firmware updates
- Bluetooth configuration app integration

**ML:**
- Improved model architectures
- Transfer learning from pre-trained audio models
- Real-time spectrogram visualization
- Automated hyperparameter tuning

**Hardware:**
- Custom PCB design (vs. dev boards)
- Alternative microphone arrays (linear, larger aperture)
- Integration with external sensors (radar, thermal)

### 3. Documentation

- Translation to other languages
- Video tutorials
- Deployment guides for specific scenarios
- Troubleshooting guides

### 4. Testing and Feedback

- Field testing reports with detection statistics
- False positive analysis
- Environmental performance data
- Hardware durability reports

## Development Setup

### Firmware

```bash
# Install PlatformIO
pip install platformio

# Clone repository
git clone https://github.com/your-org/varta.git
cd varta/firmware

# Build
pio run

# Upload (connect ESP32-S3)
pio run --target upload

# Monitor serial output
pio device monitor
```

### ML Training

```bash
cd varta/ml/training

# Create virtual environment
python -m venv venv
source venv/bin/activate  # or venv\Scripts\activate on Windows

# Install dependencies
pip install -r requirements.txt

# Run training
python train.py --data ../data/samples --labels labels.csv --epochs 100
```

### Enclosure

Install OpenSCAD for enclosure modifications:
```bash
# Ubuntu
sudo apt install openscad

# macOS
brew install openscad

# Generate STL
openscad -o varta_body.stl -D 'part="body"' enclosure/enclosure.scad
```

## Code Style

### C++ (Firmware)

- Use clang-format with the provided `.clang-format`
- Meaningful variable and function names
- Comment complex algorithms
- Keep functions under 50 lines where possible

### Python (ML)

- Follow PEP 8
- Use type hints
- Document functions with docstrings
- Use black for formatting

## Pull Request Process

1. **Fork** the repository
2. **Create a branch** for your feature (`git checkout -b feature/amazing-feature`)
3. **Make your changes** with clear commit messages
4. **Test thoroughly** (include test results in PR)
5. **Update documentation** if needed
6. **Submit PR** with description of changes

### PR Requirements

- [ ] Code builds without errors
- [ ] Tests pass (if applicable)
- [ ] Documentation updated
- [ ] Commit messages are clear
- [ ] No unnecessary files included

## Issue Reporting

When reporting issues, include:

**For bugs:**
- Hardware configuration
- Firmware version
- Steps to reproduce
- Expected vs. actual behavior
- Serial output / error messages

**For feature requests:**
- Use case description
- Proposed implementation (if you have ideas)
- Impact on existing functionality

## Community Guidelines

- Be respectful and constructive
- Focus on the technical merits
- Help newcomers get started
- Share knowledge freely
- Credit others' contributions

## Security Considerations

This project is intended for defensive purposes. Please:

- Do not submit code that could be used to harm others
- Report security vulnerabilities privately
- Consider dual-use implications of features
- Follow responsible disclosure practices

## Recognition

Contributors will be acknowledged in:
- README.md contributors section
- Release notes for significant contributions
- Project documentation

## Questions?

- Open a GitHub Discussion for general questions
- Create an Issue for specific bugs or features
- Check existing issues before creating new ones

---

Thank you for helping make VARTA better!
