/**
 * VARTA - Audio Processor
 * Handles FFT, mel spectrogram computation, and audio feature extraction
 */

#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <Arduino.h>
#include <arduinoFFT.h>

class AudioProcessor {
public:
    AudioProcessor();
    ~AudioProcessor();

    void begin(int sampleRate, int fftSize, int melBins);
    void computeMelSpectrogram(float* audioSamples, int numSamples, float* melOutput);
    void setNoiseFloor(float* noiseFloor);
    float computeRMS(float* samples, int numSamples);
    float computePeakFrequency(float* samples, int numSamples);

private:
    int _sampleRate;
    int _fftSize;
    int _melBins;

    double* _vReal;
    double* _vImag;
    float* _melFilterbank;
    float* _noiseFloor;
    float* _window;

    ArduinoFFT<double>* _fft;

    void createMelFilterbank();
    void createHannWindow();
    float hzToMel(float hz);
    float melToHz(float mel);
};

// Implementation

AudioProcessor::AudioProcessor() :
    _sampleRate(44100),
    _fftSize(2048),
    _melBins(128),
    _vReal(nullptr),
    _vImag(nullptr),
    _melFilterbank(nullptr),
    _noiseFloor(nullptr),
    _window(nullptr),
    _fft(nullptr)
{
}

AudioProcessor::~AudioProcessor() {
    if (_vReal) delete[] _vReal;
    if (_vImag) delete[] _vImag;
    if (_melFilterbank) delete[] _melFilterbank;
    if (_noiseFloor) delete[] _noiseFloor;
    if (_window) delete[] _window;
    if (_fft) delete _fft;
}

void AudioProcessor::begin(int sampleRate, int fftSize, int melBins) {
    _sampleRate = sampleRate;
    _fftSize = fftSize;
    _melBins = melBins;

    // Allocate buffers
    _vReal = new double[_fftSize];
    _vImag = new double[_fftSize];
    _melFilterbank = new float[_melBins * (_fftSize / 2 + 1)];
    _noiseFloor = new float[_melBins];
    _window = new float[_fftSize];

    // Initialize
    memset(_noiseFloor, 0, _melBins * sizeof(float));

    _fft = new ArduinoFFT<double>(_vReal, _vImag, _fftSize, _sampleRate);

    createMelFilterbank();
    createHannWindow();

    Serial.printf("AudioProcessor initialized: SR=%d FFT=%d MEL=%d\n", 
                  _sampleRate, _fftSize, _melBins);
}

void AudioProcessor::createHannWindow() {
    for (int i = 0; i < _fftSize; i++) {
        _window[i] = 0.5f * (1.0f - cos(2.0f * PI * i / (_fftSize - 1)));
    }
}

float AudioProcessor::hzToMel(float hz) {
    return 2595.0f * log10(1.0f + hz / 700.0f);
}

float AudioProcessor::melToHz(float mel) {
    return 700.0f * (pow(10.0f, mel / 2595.0f) - 1.0f);
}

void AudioProcessor::createMelFilterbank() {
    // Create triangular mel filterbank
    float fMin = 0.0f;
    float fMax = _sampleRate / 2.0f;
    
    float melMin = hzToMel(fMin);
    float melMax = hzToMel(fMax);
    
    int numFftBins = _fftSize / 2 + 1;
    float* melPoints = new float[_melBins + 2];
    int* fftBinPoints = new int[_melBins + 2];
    
    // Equally spaced mel points
    for (int i = 0; i < _melBins + 2; i++) {
        float mel = melMin + (melMax - melMin) * i / (_melBins + 1);
        melPoints[i] = melToHz(mel);
        fftBinPoints[i] = (int)((melPoints[i] / (_sampleRate / 2.0f)) * numFftBins);
        fftBinPoints[i] = constrain(fftBinPoints[i], 0, numFftBins - 1);
    }
    
    // Initialize filterbank to zero
    memset(_melFilterbank, 0, _melBins * numFftBins * sizeof(float));
    
    // Create triangular filters
    for (int m = 0; m < _melBins; m++) {
        int fStart = fftBinPoints[m];
        int fCenter = fftBinPoints[m + 1];
        int fEnd = fftBinPoints[m + 2];
        
        // Rising slope
        for (int k = fStart; k < fCenter; k++) {
            if (fCenter != fStart) {
                _melFilterbank[m * numFftBins + k] = 
                    (float)(k - fStart) / (fCenter - fStart);
            }
        }
        
        // Falling slope
        for (int k = fCenter; k <= fEnd; k++) {
            if (fEnd != fCenter) {
                _melFilterbank[m * numFftBins + k] = 
                    (float)(fEnd - k) / (fEnd - fCenter);
            }
        }
    }
    
    delete[] melPoints;
    delete[] fftBinPoints;
}

void AudioProcessor::computeMelSpectrogram(float* audioSamples, int numSamples, float* melOutput) {
    int numFftBins = _fftSize / 2 + 1;
    
    // Apply window and copy to FFT buffer
    for (int i = 0; i < _fftSize; i++) {
        if (i < numSamples) {
            _vReal[i] = audioSamples[i] * _window[i];
        } else {
            _vReal[i] = 0.0;
        }
        _vImag[i] = 0.0;
    }
    
    // Compute FFT
    _fft->windowing(FFTWindow::Rectangle, FFTDirection::Forward);  // Window already applied
    _fft->compute(FFTDirection::Forward);
    _fft->complexToMagnitude();
    
    // Apply mel filterbank
    for (int m = 0; m < _melBins; m++) {
        float sum = 0.0f;
        for (int k = 0; k < numFftBins; k++) {
            sum += _melFilterbank[m * numFftBins + k] * (float)_vReal[k];
        }
        
        // Convert to dB
        sum = max(sum, 1e-10f);  // Avoid log(0)
        melOutput[m] = 20.0f * log10(sum);
        
        // Subtract noise floor if calibrated
        if (_noiseFloor[m] != 0.0f) {
            melOutput[m] -= _noiseFloor[m];
            melOutput[m] = max(melOutput[m], 0.0f);
        }
    }
}

void AudioProcessor::setNoiseFloor(float* noiseFloor) {
    memcpy(_noiseFloor, noiseFloor, _melBins * sizeof(float));
    Serial.println("Noise floor updated");
}

float AudioProcessor::computeRMS(float* samples, int numSamples) {
    float sum = 0.0f;
    for (int i = 0; i < numSamples; i++) {
        sum += samples[i] * samples[i];
    }
    return sqrt(sum / numSamples);
}

float AudioProcessor::computePeakFrequency(float* samples, int numSamples) {
    // Copy to FFT buffer
    for (int i = 0; i < _fftSize; i++) {
        if (i < numSamples) {
            _vReal[i] = samples[i] * _window[i];
        } else {
            _vReal[i] = 0.0;
        }
        _vImag[i] = 0.0;
    }
    
    _fft->compute(FFTDirection::Forward);
    _fft->complexToMagnitude();
    
    // Find peak
    double maxMag = 0.0;
    int maxIndex = 0;
    for (int i = 1; i < _fftSize / 2; i++) {
        if (_vReal[i] > maxMag) {
            maxMag = _vReal[i];
            maxIndex = i;
        }
    }
    
    return (float)maxIndex * _sampleRate / _fftSize;
}

#endif // AUDIO_PROCESSOR_H
