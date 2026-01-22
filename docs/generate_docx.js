const {
    Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
    Header, Footer, AlignmentType, LevelFormat, HeadingLevel,
    BorderStyle, WidthType, ShadingType, PageNumber, PageBreak,
    TableOfContents
} = require('docx');
const fs = require('fs');

// Helper for creating table cells
const border = { style: BorderStyle.SINGLE, size: 1, color: "CCCCCC" };
const borders = { top: border, bottom: border, left: border, right: border };

function createCell(text, width, isHeader = false, colSpan = 1) {
    return new TableCell({
        borders,
        width: { size: width, type: WidthType.DXA },
        shading: isHeader ? { fill: "2B5797", type: ShadingType.CLEAR } : undefined,
        margins: { top: 80, bottom: 80, left: 120, right: 120 },
        columnSpan: colSpan,
        children: [new Paragraph({
            children: [new TextRun({
                text: text,
                bold: isHeader,
                color: isHeader ? "FFFFFF" : "000000",
                size: 22
            })]
        })]
    });
}

function createTable(headers, rows, colWidths) {
    const tableRows = [
        new TableRow({
            children: headers.map((h, i) => createCell(h, colWidths[i], true))
        }),
        ...rows.map(row => new TableRow({
            children: row.map((cell, i) => createCell(cell, colWidths[i]))
        }))
    ];
    
    return new Table({
        width: { size: 100, type: WidthType.PERCENTAGE },
        columnWidths: colWidths,
        rows: tableRows
    });
}

const doc = new Document({
    styles: {
        default: {
            document: {
                run: { font: "Arial", size: 24 }
            }
        },
        paragraphStyles: [
            {
                id: "Heading1",
                name: "Heading 1",
                basedOn: "Normal",
                next: "Normal",
                quickFormat: true,
                run: { size: 36, bold: true, font: "Arial", color: "2B5797" },
                paragraph: { spacing: { before: 400, after: 200 }, outlineLevel: 0 }
            },
            {
                id: "Heading2",
                name: "Heading 2",
                basedOn: "Normal",
                next: "Normal",
                quickFormat: true,
                run: { size: 28, bold: true, font: "Arial", color: "2B5797" },
                paragraph: { spacing: { before: 300, after: 150 }, outlineLevel: 1 }
            },
            {
                id: "Heading3",
                name: "Heading 3",
                basedOn: "Normal",
                next: "Normal",
                quickFormat: true,
                run: { size: 24, bold: true, font: "Arial" },
                paragraph: { spacing: { before: 200, after: 100 }, outlineLevel: 2 }
            }
        ]
    },
    numbering: {
        config: [
            {
                reference: "bullets",
                levels: [{
                    level: 0,
                    format: LevelFormat.BULLET,
                    text: "•",
                    alignment: AlignmentType.LEFT,
                    style: { paragraph: { indent: { left: 720, hanging: 360 } } }
                }]
            },
            {
                reference: "numbers",
                levels: [{
                    level: 0,
                    format: LevelFormat.DECIMAL,
                    text: "%1.",
                    alignment: AlignmentType.LEFT,
                    style: { paragraph: { indent: { left: 720, hanging: 360 } } }
                }]
            }
        ]
    },
    sections: [{
        properties: {
            page: {
                size: { width: 12240, height: 15840 },
                margin: { top: 1440, right: 1440, bottom: 1440, left: 1440 }
            }
        },
        headers: {
            default: new Header({
                children: [new Paragraph({
                    alignment: AlignmentType.RIGHT,
                    children: [new TextRun({ text: "VARTA Open Source Project", italics: true, size: 20, color: "666666" })]
                })]
            })
        },
        footers: {
            default: new Footer({
                children: [new Paragraph({
                    alignment: AlignmentType.CENTER,
                    children: [
                        new TextRun({ text: "Page ", size: 20 }),
                        new TextRun({ children: [PageNumber.CURRENT], size: 20 }),
                        new TextRun({ text: " of ", size: 20 }),
                        new TextRun({ children: [PageNumber.TOTAL_PAGES], size: 20 })
                    ]
                })]
            })
        },
        children: [
            // TITLE PAGE
            new Paragraph({ spacing: { after: 1200 } }),
            new Paragraph({
                alignment: AlignmentType.CENTER,
                spacing: { after: 400 },
                children: [new TextRun({ text: "VARTA", size: 72, bold: true, color: "2B5797" })]
            }),
            new Paragraph({
                alignment: AlignmentType.CENTER,
                spacing: { after: 200 },
                children: [new TextRun({ text: "Visual and Acoustic Real-Time Alert System", size: 32, italics: true })]
            }),
            new Paragraph({
                alignment: AlignmentType.CENTER,
                spacing: { after: 800 },
                children: [new TextRun({ text: "Open Source Portable Acoustic Drone Detector", size: 28 })]
            }),
            new Paragraph({
                alignment: AlignmentType.CENTER,
                spacing: { after: 200 },
                children: [new TextRun({ text: "Designed to detect fiber optic controlled FPV drones", size: 24, color: "666666" })]
            }),
            new Paragraph({
                alignment: AlignmentType.CENTER,
                spacing: { after: 200 },
                children: [new TextRun({ text: "that evade traditional RF-based detection systems", size: 24, color: "666666" })]
            }),
            new Paragraph({ spacing: { after: 1600 } }),
            new Paragraph({
                alignment: AlignmentType.CENTER,
                children: [new TextRun({ text: "Technical Documentation v1.0", size: 24 })]
            }),
            new Paragraph({
                alignment: AlignmentType.CENTER,
                spacing: { after: 200 },
                children: [new TextRun({ text: "January 2025", size: 24 })]
            }),
            new Paragraph({ spacing: { after: 800 } }),
            new Paragraph({
                alignment: AlignmentType.CENTER,
                children: [new TextRun({ text: "Hardware: CERN-OHL-P-2.0  |  Software: MIT  |  Docs: CC-BY-4.0", size: 20, color: "666666" })]
            }),

            // PAGE BREAK
            new Paragraph({ children: [new PageBreak()] }),

            // TABLE OF CONTENTS
            new Paragraph({
                heading: HeadingLevel.HEADING_1,
                children: [new TextRun("Table of Contents")]
            }),
            new TableOfContents("Table of Contents", { hyperlink: true, headingStyleRange: "1-3" }),
            
            new Paragraph({ children: [new PageBreak()] }),

            // EXECUTIVE SUMMARY
            new Paragraph({
                heading: HeadingLevel.HEADING_1,
                children: [new TextRun("Executive Summary")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("VARTA is an open-source, portable acoustic drone detection system designed to address a critical gap in current drone defense: the inability of RF-based detectors to identify fiber optic controlled drones.")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("Traditional drone detection systems like the BlueBird Chuyka rely on detecting radio frequency emissions from drone control links. However, fiber optic controlled FPV drones transmit no RF signals, making them invisible to these systems. VARTA fills this gap using acoustic detection and machine learning classification.")]
            }),
            
            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Key Capabilities")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("4-microphone beamforming array for direction estimation")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Machine learning classification using TensorFlow Lite on edge")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("200-500m detection range (environment dependent)")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Visual, audio, and haptic alerts for high-noise environments")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("$80-100 total bill of materials cost")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Fully open source hardware and software")]
            }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Target Users")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("Infantry units, security personnel, and defensive operators who need portable, low-cost early warning against fiber optic FPV drones where RF detection is ineffective.")]
            }),

            new Paragraph({ children: [new PageBreak()] }),

            // PROBLEM STATEMENT
            new Paragraph({
                heading: HeadingLevel.HEADING_1,
                children: [new TextRun("Problem Statement")]
            }),
            
            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("The Fiber Optic Drone Threat")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("Fiber optic controlled FPV drones represent a significant evolution in drone warfare. By transmitting video and control signals through a thin fiber optic cable rather than radio waves, these systems achieve:")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Complete immunity to RF jamming and detection")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Zero electromagnetic signature during flight")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("High-bandwidth, low-latency video transmission")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Resistance to electronic warfare countermeasures")]
            }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Current Detection Gap")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("RF-based drone detectors like the BlueBird Chuyka 2.0 ($460, 900-1800 MHz and 4860-6060 MHz scanning) are completely ineffective against fiber optic drones because there are no radio signals to detect.")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("Military-grade solutions exist (radar, sensor fusion) but are expensive ($50,000+), vehicle-mounted, and unavailable to frontline infantry units who face the most immediate threat.")]
            }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Market Opportunity")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("There is no commercially available portable detector in the $1,000-3,000 price range that can detect fiber optic drones. VARTA fills this gap using acoustic detection, the only viable passive detection method for infantry-portable systems.")]
            }),

            new Paragraph({ children: [new PageBreak()] }),

            // TECHNICAL SPECIFICATIONS
            new Paragraph({
                heading: HeadingLevel.HEADING_1,
                children: [new TextRun("Technical Specifications")]
            }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("System Overview")]
            }),

            createTable(
                ["Parameter", "Specification"],
                [
                    ["Detection Range", "200-500m (line of sight, low ambient noise)"],
                    ["Detection Time", "4-8 seconds"],
                    ["Direction Accuracy", "±15° azimuth"],
                    ["Power Supply", "7.4V (2S 18650 Li-Ion)"],
                    ["Battery Life", "8+ hours continuous"],
                    ["Operating Temperature", "-20°C to +50°C"],
                    ["Enclosure Rating", "IP54 (dust and splash protected)"],
                    ["Weight", "~350g with battery"],
                    ["Dimensions", "140 x 80 x 45 mm"]
                ],
                [4680, 4680]
            ),

            new Paragraph({ spacing: { after: 300 } }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Detection Methodology")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("VARTA uses three complementary detection techniques:")]
            }),
            new Paragraph({
                spacing: { after: 100 },
                children: [
                    new TextRun({ text: "1. FFT Spectral Analysis: ", bold: true }),
                    new TextRun("Continuous fast Fourier transform analysis identifies drone motor fundamentals (typically 150-400 Hz) and their characteristic harmonic patterns.")
                ]
            }),
            new Paragraph({
                spacing: { after: 100 },
                children: [
                    new TextRun({ text: "2. Beamforming: ", bold: true }),
                    new TextRun("Time-difference-of-arrival (TDOA) across the 4-microphone square array estimates threat direction with ±15° accuracy.")
                ]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [
                    new TextRun({ text: "3. ML Classification: ", bold: true }),
                    new TextRun("A lightweight convolutional neural network running on the ESP32-S3 distinguishes drone signatures from environmental noise (vehicles, wind, wildlife).")
                ]
            }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Hardware Architecture")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("The system is built around the ESP32-S3-WROOM-1 microcontroller with integrated Wi-Fi and Bluetooth, 16MB flash, and 8MB PSRAM for ML inference.")]
            }),

            createTable(
                ["Component", "Specification", "Purpose"],
                [
                    ["ESP32-S3-WROOM-1-N16R8", "240 MHz dual-core, 16MB flash", "Main processor, ML inference"],
                    ["INMP441 MEMS Microphone", "I2S digital output, 4 units", "Audio capture"],
                    ["SSD1306 OLED", "128x64 pixels, I2C", "Status display"],
                    ["WS2812B LED Ring", "8 LEDs, 32mm diameter", "Direction indicator"],
                    ["Piezo Buzzer", "3-5V active", "Audio alert"],
                    ["Coin Vibration Motor", "3V DC", "Haptic alert"],
                    ["18650 2S Pack", "7.4V, 5000mAh", "Power source"]
                ],
                [3120, 3120, 3120]
            ),

            new Paragraph({ children: [new PageBreak()] }),

            // ML MODEL
            new Paragraph({
                heading: HeadingLevel.HEADING_1,
                children: [new TextRun("Machine Learning Model")]
            }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Architecture")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("The detection model is a lightweight convolutional neural network optimized for real-time inference on the ESP32-S3:")]
            }),
            new Paragraph({
                spacing: { after: 100 },
                children: [
                    new TextRun({ text: "Input: ", bold: true }),
                    new TextRun("128 mel-frequency bands × 32 time frames (1 second of audio)")
                ]
            }),
            new Paragraph({
                spacing: { after: 100 },
                children: [
                    new TextRun({ text: "Layer 1: ", bold: true }),
                    new TextRun("Conv2D (16 filters, 3×3) + BatchNorm + ReLU + MaxPool")
                ]
            }),
            new Paragraph({
                spacing: { after: 100 },
                children: [
                    new TextRun({ text: "Layer 2: ", bold: true }),
                    new TextRun("Conv2D (32 filters, 3×3) + BatchNorm + ReLU + MaxPool")
                ]
            }),
            new Paragraph({
                spacing: { after: 100 },
                children: [
                    new TextRun({ text: "Layer 3: ", bold: true }),
                    new TextRun("Conv2D (64 filters, 3×3) + BatchNorm + ReLU + GlobalAvgPool")
                ]
            }),
            new Paragraph({
                spacing: { after: 100 },
                children: [
                    new TextRun({ text: "Dense: ", bold: true }),
                    new TextRun("32 units + ReLU + Dropout (0.3)")
                ]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [
                    new TextRun({ text: "Output: ", bold: true }),
                    new TextRun("2 classes (no_drone, drone) with softmax")
                ]
            }),

            createTable(
                ["Metric", "Value"],
                [
                    ["Total Parameters", "~45,000"],
                    ["Model Size (quantized)", "~50 KB"],
                    ["Inference Time", "~50 ms on ESP32-S3"],
                    ["Memory Usage", "~100 KB arena"]
                ],
                [4680, 4680]
            ),

            new Paragraph({ spacing: { after: 300 } }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Training Pipeline")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("The project includes a complete training pipeline with Python scripts for data collection, labeling, training, and TensorFlow Lite conversion. Users train custom models on audio from their deployment environment for optimal performance.")]
            }),

            new Paragraph({ children: [new PageBreak()] }),

            // BILL OF MATERIALS
            new Paragraph({
                heading: HeadingLevel.HEADING_1,
                children: [new TextRun("Bill of Materials")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("Total estimated cost: $83.92 USD")]
            }),

            createTable(
                ["Component", "Qty", "Unit Cost", "Total"],
                [
                    ["ESP32-S3-DevKitC-1", "1", "$9.99", "$9.99"],
                    ["INMP441 MEMS Microphone", "4", "$2.99", "$11.96"],
                    ["SSD1306 0.96\" OLED", "1", "$3.99", "$3.99"],
                    ["WS2812B 8-LED Ring", "1", "$2.99", "$2.99"],
                    ["Piezo Buzzer", "1", "$0.99", "$0.99"],
                    ["Vibration Motor", "1", "$1.49", "$1.49"],
                    ["18650 2S Holder + BMS", "1", "$3.49", "$3.49"],
                    ["18650 Cells (2500mAh)", "2", "$4.99", "$9.98"],
                    ["TP4056 USB-C Charger", "1", "$1.99", "$1.99"],
                    ["MT3608 Boost Converter", "1", "$0.99", "$0.99"],
                    ["Switches, Buttons", "2", "$0.40", "$0.80"],
                    ["Resistors, Transistor", "4", "$0.10", "$0.40"],
                    ["Connectors, Headers", "-", "-", "$2.00"],
                    ["Wire (22AWG + 26AWG)", "-", "-", "$10.98"],
                    ["3D Printed Enclosure", "3 parts", "-", "$6.00"],
                    ["Hardware (screws, nuts)", "-", "-", "$2.04"],
                    ["Miscellaneous", "-", "-", "$13.98"],
                    ["", "", "TOTAL:", "$83.92"]
                ],
                [4000, 1000, 2180, 2180]
            ),

            new Paragraph({ children: [new PageBreak()] }),

            // LIMITATIONS
            new Paragraph({
                heading: HeadingLevel.HEADING_1,
                children: [new TextRun("Limitations and Considerations")]
            }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Environmental Factors")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Detection range decreases significantly in high ambient noise (combat, vehicles, wind)")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Cannot detect drones with silenced or baffled motors at useful range")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("False positives possible from other small motors, insects, distant vehicles")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Requires acoustic line-of-sight; terrain and structures attenuate sound")]
            }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Technical Limitations")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Azimuth direction only; no altitude estimation")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Model performance depends heavily on training data quality")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Initial deployment will have false positives until calibrated")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Not a substitute for other protective measures")]
            }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Recommended Mitigations")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("Deploy as part of layered defense with RF detection. Train custom ML models with audio from actual deployment environments. Use multiple units for overlapping coverage. Recalibrate when changing locations.")]
            }),

            new Paragraph({ children: [new PageBreak()] }),

            // PROJECT STRUCTURE
            new Paragraph({
                heading: HeadingLevel.HEADING_1,
                children: [new TextRun("Project Structure")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun({ text: "Repository: ", bold: true }), new TextRun("github.com/[your-org]/varta")]
            }),

            new Paragraph({
                spacing: { after: 100 },
                children: [new TextRun({ text: "/firmware/", bold: true }), new TextRun(" - ESP32-S3 firmware (PlatformIO)")]
            }),
            new Paragraph({
                spacing: { after: 100 },
                children: [new TextRun({ text: "/ml/training/", bold: true }), new TextRun(" - Python ML training pipeline")]
            }),
            new Paragraph({
                spacing: { after: 100 },
                children: [new TextRun({ text: "/hardware/kicad/", bold: true }), new TextRun(" - KiCad schematic files")]
            }),
            new Paragraph({
                spacing: { after: 100 },
                children: [new TextRun({ text: "/hardware/bom/", bold: true }), new TextRun(" - Bill of materials")]
            }),
            new Paragraph({
                spacing: { after: 100 },
                children: [new TextRun({ text: "/enclosure/", bold: true }), new TextRun(" - OpenSCAD 3D printable enclosure")]
            }),
            new Paragraph({
                spacing: { after: 100 },
                children: [new TextRun({ text: "/docs/", bold: true }), new TextRun(" - Assembly instructions and documentation")]
            }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Getting Started")]
            }),
            new Paragraph({
                numbering: { reference: "numbers", level: 0 },
                children: [new TextRun("Clone repository and review documentation")]
            }),
            new Paragraph({
                numbering: { reference: "numbers", level: 0 },
                children: [new TextRun("Order components from BOM")]
            }),
            new Paragraph({
                numbering: { reference: "numbers", level: 0 },
                children: [new TextRun("3D print enclosure (PETG recommended)")]
            }),
            new Paragraph({
                numbering: { reference: "numbers", level: 0 },
                children: [new TextRun("Assemble hardware per assembly guide")]
            }),
            new Paragraph({
                numbering: { reference: "numbers", level: 0 },
                children: [new TextRun("Flash firmware using PlatformIO")]
            }),
            new Paragraph({
                numbering: { reference: "numbers", level: 0 },
                children: [new TextRun("Collect training data in deployment environment")]
            }),
            new Paragraph({
                numbering: { reference: "numbers", level: 0 },
                children: [new TextRun("Train custom ML model")]
            }),
            new Paragraph({
                numbering: { reference: "numbers", level: 0 },
                children: [new TextRun("Deploy and calibrate")]
            }),

            new Paragraph({ children: [new PageBreak()] }),

            // CONTRIBUTING
            new Paragraph({
                heading: HeadingLevel.HEADING_1,
                children: [new TextRun("Contributing and Roadmap")]
            }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Priority Contributions Needed")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Diverse drone audio training data from various types and distances")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Environmental noise samples from deployment scenarios")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Field testing reports with detection statistics")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Custom PCB design (currently uses dev boards)")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Mobile companion app for configuration")]
            }),

            new Paragraph({
                heading: HeadingLevel.HEADING_2,
                children: [new TextRun("Future Roadmap")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Multi-unit mesh networking for overlapping coverage")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Integration with external sensors (radar, thermal)")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("OTA firmware updates")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Pre-trained models for common drone types")]
            }),
            new Paragraph({
                numbering: { reference: "bullets", level: 0 },
                children: [new TextRun("Alternative form factors (helmet mount, vehicle mount)")]
            }),

            new Paragraph({ children: [new PageBreak()] }),

            // CONCLUSION
            new Paragraph({
                heading: HeadingLevel.HEADING_1,
                children: [new TextRun("Conclusion")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("VARTA addresses a critical capability gap in drone defense. As fiber optic controlled drones become more prevalent, the inability of RF-based systems to detect them creates a significant vulnerability.")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("By leveraging acoustic detection, machine learning, and open-source collaboration, VARTA provides an affordable, deployable solution that can be built, modified, and improved by the community.")]
            }),
            new Paragraph({
                spacing: { after: 200 },
                children: [new TextRun("While acoustic detection has inherent limitations, it remains the only viable passive detection method for fiber optic drones at an infantry-portable price point. VARTA is designed to be one layer in a comprehensive defense strategy, not a complete solution.")]
            }),
            new Paragraph({
                spacing: { after: 400 },
                children: [new TextRun({ text: "The project is fully open source and welcomes contributions from the global defense and maker communities.", italics: true })]
            }),

            // Licenses footer
            new Paragraph({
                spacing: { before: 800 },
                alignment: AlignmentType.CENTER,
                children: [new TextRun({ text: "Licenses", bold: true, size: 22 })]
            }),
            new Paragraph({
                alignment: AlignmentType.CENTER,
                children: [new TextRun({ text: "Hardware: CERN-OHL-P-2.0 | Software: MIT | Documentation: CC-BY-4.0", size: 20, color: "666666" })]
            }),
        ]
    }]
});

// Generate document
Packer.toBuffer(doc).then(buffer => {
    fs.writeFileSync('/home/claude/fiber-drone-detector/docs/VARTA_Technical_Documentation.docx', buffer);
    console.log('Document created successfully!');
});
