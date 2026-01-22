/**
 * VARTA - Direction Estimator
 * Estimates sound source direction using TDOA (Time Difference of Arrival)
 * across a 4-microphone square array
 */

#ifndef DIRECTION_ESTIMATOR_H
#define DIRECTION_ESTIMATOR_H

#include <Arduino.h>

class DirectionEstimator {
public:
    DirectionEstimator();
    
    void begin(float micSpacingMm, float speedOfSound, int sampleRate);
    
    /**
     * Estimate direction from 4 microphone signals
     * Returns azimuth angle in degrees (0-360, 0 = forward)
     */
    float estimateDirection(float* mic1, float* mic2, float* mic3, float* mic4, int numSamples);
    
    /**
     * Get correlation confidence (0-1)
     */
    float getConfidence() { return _lastConfidence; }

private:
    float _micSpacingM;
    float _speedOfSound;
    int _sampleRate;
    float _maxDelaySamples;
    float _lastConfidence;
    float _smoothedDirection;
    
    /**
     * Cross-correlate two signals and find peak delay
     * Returns delay in samples (positive = sig2 leads sig1)
     */
    float crossCorrelate(float* sig1, float* sig2, int numSamples, float* confidence);
    
    /**
     * Convert TDOA measurements to azimuth angle
     */
    float tdoaToAzimuth(float tdoa12, float tdoa14, float tdoa32, float tdoa34);
};

// Implementation

DirectionEstimator::DirectionEstimator() :
    _micSpacingM(0.05f),
    _speedOfSound(343.0f),
    _sampleRate(44100),
    _maxDelaySamples(0),
    _lastConfidence(0),
    _smoothedDirection(0)
{
}

void DirectionEstimator::begin(float micSpacingMm, float speedOfSound, int sampleRate) {
    _micSpacingM = micSpacingMm / 1000.0f;
    _speedOfSound = speedOfSound;
    _sampleRate = sampleRate;
    
    // Maximum possible delay between any two mics
    // For diagonal: sqrt(2) * spacing
    float maxDistanceM = sqrt(2.0f) * _micSpacingM;
    float maxDelayS = maxDistanceM / _speedOfSound;
    _maxDelaySamples = maxDelayS * _sampleRate;
    
    Serial.printf("DirectionEstimator: spacing=%.1fmm maxDelay=%.1f samples\n",
                  micSpacingMm, _maxDelaySamples);
}

float DirectionEstimator::crossCorrelate(float* sig1, float* sig2, int numSamples, float* confidence) {
    int maxLag = (int)ceil(_maxDelaySamples) + 5;  // Some margin
    maxLag = min(maxLag, numSamples / 4);  // Don't search too far
    
    float maxCorr = -1e10f;
    int bestLag = 0;
    
    // Compute correlation at different lags
    for (int lag = -maxLag; lag <= maxLag; lag++) {
        float corr = 0.0f;
        float norm1 = 0.0f;
        float norm2 = 0.0f;
        int count = 0;
        
        for (int i = maxLag; i < numSamples - maxLag; i++) {
            int j = i + lag;
            if (j >= 0 && j < numSamples) {
                corr += sig1[i] * sig2[j];
                norm1 += sig1[i] * sig1[i];
                norm2 += sig2[j] * sig2[j];
                count++;
            }
        }
        
        // Normalized correlation
        float norm = sqrt(norm1 * norm2);
        if (norm > 1e-10f) {
            corr /= norm;
        } else {
            corr = 0.0f;
        }
        
        if (corr > maxCorr) {
            maxCorr = corr;
            bestLag = lag;
        }
    }
    
    // Subsample interpolation using parabolic fit
    float refinedLag = (float)bestLag;
    if (abs(bestLag) < maxLag - 1) {
        // Get correlation at bestLag-1, bestLag, bestLag+1
        // (Simplified - recompute or cache)
        // For now, just use integer lag
    }
    
    *confidence = maxCorr;
    return refinedLag;
}

float DirectionEstimator::tdoaToAzimuth(float tdoa12, float tdoa14, float tdoa32, float tdoa34) {
    /*
     * Microphone arrangement (looking from above):
     * 
     *        Front (0°)
     *           ↑
     *     M1 -------- M2
     *      |          |
     *      |    ●     |      Y axis
     *      |          |        ↑
     *     M4 -------- M3       |
     *                          +--→ X axis
     * 
     * TDOA12: positive = sound from right (M2 side)
     * TDOA14: positive = sound from rear (M4 side)
     * 
     * We use the horizontal (M1-M2, M4-M3) and vertical (M1-M4, M2-M3) pairs
     * to estimate X and Y components of arrival direction.
     */
    
    // Convert TDOA (in samples) to time difference (in seconds)
    float dt12 = tdoa12 / _sampleRate;
    float dt14 = tdoa14 / _sampleRate;
    float dt32 = tdoa32 / _sampleRate;
    float dt34 = tdoa34 / _sampleRate;
    
    // Average the parallel pairs for robustness
    float dtX = (dt12 + dt34) / 2.0f;  // Left-right
    float dtY = (dt14 + dt32) / 2.0f;  // Front-back (note: M3-M2 = -(M2-M3))
    
    // Convert to sine of arrival angle
    // sin(θ) = (c * Δt) / d
    float sinX = (_speedOfSound * dtX) / _micSpacingM;
    float sinY = (_speedOfSound * dtY) / _micSpacingM;
    
    // Clamp to valid range
    sinX = constrain(sinX, -1.0f, 1.0f);
    sinY = constrain(sinY, -1.0f, 1.0f);
    
    // Convert to azimuth angle
    // atan2 gives angle from +X axis, we want angle from +Y axis (front)
    float azimuth = atan2(sinX, -sinY) * 180.0f / PI;
    
    // Normalize to 0-360
    if (azimuth < 0) azimuth += 360.0f;
    
    return azimuth;
}

float DirectionEstimator::estimateDirection(float* mic1, float* mic2, float* mic3, float* mic4, int numSamples) {
    float conf12, conf14, conf32, conf34;
    
    // Compute TDOA for each mic pair
    float tdoa12 = crossCorrelate(mic1, mic2, numSamples, &conf12);
    float tdoa14 = crossCorrelate(mic1, mic4, numSamples, &conf14);
    float tdoa32 = crossCorrelate(mic3, mic2, numSamples, &conf32);
    float tdoa34 = crossCorrelate(mic3, mic4, numSamples, &conf34);
    
    // Average confidence
    _lastConfidence = (conf12 + conf14 + conf32 + conf34) / 4.0f;
    
    // Check if correlation is strong enough
    if (_lastConfidence < 0.5f) {
        // Low confidence - return smoothed previous estimate
        return _smoothedDirection;
    }
    
    // Convert TDOA to azimuth
    float azimuth = tdoaToAzimuth(tdoa12, tdoa14, tdoa32, tdoa34);
    
    // Smooth the output (exponential moving average)
    // Handle wraparound at 0/360
    float diff = azimuth - _smoothedDirection;
    if (diff > 180.0f) diff -= 360.0f;
    if (diff < -180.0f) diff += 360.0f;
    
    _smoothedDirection += 0.3f * diff;  // Smoothing factor
    
    // Normalize
    if (_smoothedDirection < 0) _smoothedDirection += 360.0f;
    if (_smoothedDirection >= 360.0f) _smoothedDirection -= 360.0f;
    
    #if DEBUG_PRINT_DIRECTION
    Serial.printf("TDOA: [%.1f, %.1f, %.1f, %.1f] conf=%.2f -> %.1f°\n",
                  tdoa12, tdoa14, tdoa32, tdoa34, _lastConfidence, _smoothedDirection);
    #endif
    
    return _smoothedDirection;
}

#endif // DIRECTION_ESTIMATOR_H
