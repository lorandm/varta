/**
 * VARTA - ML Model Data
 * 
 * This is a PLACEHOLDER model. You must train your own model
 * and replace this file using:
 * 
 *   xxd -i models/drone_detector.tflite > firmware/src/model_data.h
 * 
 * See ml/training/README.md for training instructions.
 */

#ifndef MODEL_DATA_H
#define MODEL_DATA_H

// Placeholder - minimal valid TFLite model structure
// This will NOT provide useful detection - train your own!
alignas(8) const unsigned char drone_detector_tflite[] = {
    // TFLite flatbuffer header
    0x1c, 0x00, 0x00, 0x00, 0x54, 0x46, 0x4c, 0x33,  // "TFL3"
    // ... minimal model data
    // This placeholder will fail to load - intentionally
    // You MUST train and export your own model
    0x00
};
const unsigned int drone_detector_tflite_len = 9;

/*
 * INSTRUCTIONS:
 * 
 * 1. Collect training data:
 *    cd ml/training
 *    python collect_samples.py --device your_mic --duration 300
 * 
 * 2. Label your samples:
 *    python labeler.py --input samples/
 * 
 * 3. Train the model:
 *    python train.py --data samples/ --labels labels.csv --epochs 100
 * 
 * 4. Convert to TFLite:
 *    python convert_tflite.py --model checkpoints/best.h5 --output ../models/
 * 
 * 5. Generate C header:
 *    xxd -i ../models/drone_detector.tflite > ../firmware/src/model_data.h
 * 
 * 6. Rebuild firmware:
 *    cd ../firmware
 *    pio run
 */

#endif // MODEL_DATA_H
