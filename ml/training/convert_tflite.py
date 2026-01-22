#!/usr/bin/env python3
"""
Convert trained Keras model to TensorFlow Lite for ESP32-S3.

Usage:
    python convert_tflite.py --model checkpoints/best.h5 --output ../models/
"""

import argparse
import os
import numpy as np
from pathlib import Path

import tensorflow as tf


def convert_to_tflite(model_path, output_dir, quantize=True):
    """Convert Keras model to TFLite format."""
    print(f"Loading model from {model_path}")
    model = tf.keras.models.load_model(model_path)
    
    # Create converter
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    
    if quantize:
        print("Applying int8 quantization for ESP32...")
        
        # Full integer quantization for ESP32
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        converter.target_spec.supported_types = [tf.float32]
        
        # For full int8, we need representative dataset
        # Using random data as placeholder - replace with real samples
        def representative_dataset():
            for _ in range(100):
                data = np.random.rand(1, 32, 128, 1).astype(np.float32)
                yield [data]
        
        converter.representative_dataset = representative_dataset
        
        # Optional: force int8 for all ops (may reduce accuracy)
        # converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
        # converter.inference_input_type = tf.int8
        # converter.inference_output_type = tf.int8
    
    # Convert
    print("Converting to TFLite...")
    tflite_model = converter.convert()
    
    # Save
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    
    suffix = '_quant' if quantize else ''
    output_path = output_dir / f'drone_detector{suffix}.tflite'
    
    with open(output_path, 'wb') as f:
        f.write(tflite_model)
    
    print(f"Saved TFLite model to {output_path}")
    print(f"Model size: {len(tflite_model) / 1024:.1f} KB")
    
    # Verify model
    verify_tflite_model(output_path)
    
    return output_path


def verify_tflite_model(model_path):
    """Load and verify TFLite model."""
    print("\nVerifying TFLite model...")
    
    interpreter = tf.lite.Interpreter(model_path=str(model_path))
    interpreter.allocate_tensors()
    
    # Get input/output details
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()
    
    print(f"Input shape: {input_details[0]['shape']}")
    print(f"Input dtype: {input_details[0]['dtype']}")
    print(f"Output shape: {output_details[0]['shape']}")
    print(f"Output dtype: {output_details[0]['dtype']}")
    
    # Test inference
    input_shape = input_details[0]['shape']
    test_input = np.random.rand(*input_shape).astype(input_details[0]['dtype'])
    
    interpreter.set_tensor(input_details[0]['index'], test_input)
    interpreter.invoke()
    
    output = interpreter.get_tensor(output_details[0]['index'])
    print(f"Test output: {output}")
    print("Model verification passed!")


def generate_c_header(tflite_path, output_path=None):
    """Generate C header file from TFLite model."""
    if output_path is None:
        output_path = tflite_path.with_suffix('.h')
    
    with open(tflite_path, 'rb') as f:
        model_bytes = f.read()
    
    # Generate header
    header = '''/**
 * VARTA - ML Model Data
 * Auto-generated from {filename}
 * 
 * Model size: {size} bytes ({size_kb:.1f} KB)
 */

#ifndef MODEL_DATA_H
#define MODEL_DATA_H

alignas(8) const unsigned char drone_detector_tflite[] = {{
{data}
}};

const unsigned int drone_detector_tflite_len = {size};

#endif // MODEL_DATA_H
'''
    
    # Format bytes as hex
    hex_lines = []
    for i in range(0, len(model_bytes), 12):
        chunk = model_bytes[i:i+12]
        hex_str = ', '.join(f'0x{b:02x}' for b in chunk)
        hex_lines.append('    ' + hex_str)
    
    data_str = ',\n'.join(hex_lines)
    
    header = header.format(
        filename=tflite_path.name,
        size=len(model_bytes),
        size_kb=len(model_bytes) / 1024,
        data=data_str
    )
    
    with open(output_path, 'w') as f:
        f.write(header)
    
    print(f"Generated C header: {output_path}")


def main():
    parser = argparse.ArgumentParser(description='Convert model to TFLite')
    parser.add_argument('--model', type=str, required=True, help='Path to Keras model (.h5)')
    parser.add_argument('--output', type=str, default='../models/', help='Output directory')
    parser.add_argument('--no-quantize', action='store_true', help='Disable quantization')
    parser.add_argument('--generate-header', action='store_true', help='Generate C header file')
    
    args = parser.parse_args()
    
    # Convert model
    tflite_path = convert_to_tflite(
        args.model,
        args.output,
        quantize=not args.no_quantize
    )
    
    # Generate C header if requested
    if args.generate_header:
        header_path = Path(args.output) / 'model_data.h'
        generate_c_header(tflite_path, header_path)


if __name__ == '__main__':
    main()
