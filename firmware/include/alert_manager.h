/**
 * VARTA - Alert Manager
 * Handles buzzer, vibration motor, and alert patterns
 */

#ifndef ALERT_MANAGER_H
#define ALERT_MANAGER_H

#include <Arduino.h>

class AlertManager {
public:
    AlertManager();
    
    void begin(int buzzerPin, int vibrationPin);
    void update();  // Call in main loop
    
    void triggerAlert(int durationMs);
    void triggerHapticOnly(int durationMs);
    void stopAlert();
    
    void playTone(int frequency, int durationMs);
    void playPattern(const int* pattern, int length);
    
    bool isAlerting() { return _alertActive; }

private:
    int _buzzerPin;
    int _vibrationPin;
    
    bool _alertActive;
    bool _hapticOnly;
    unsigned long _alertStartTime;
    int _alertDuration;
    
    // Pattern playback
    const int* _pattern;
    int _patternLength;
    int _patternIndex;
    unsigned long _patternStepTime;
    
    // Pulse state
    bool _pulseState;
    unsigned long _lastPulseTime;
    int _pulseOnTime;
    int _pulseOffTime;
};

// Implementation

AlertManager::AlertManager() :
    _buzzerPin(-1),
    _vibrationPin(-1),
    _alertActive(false),
    _hapticOnly(false),
    _alertStartTime(0),
    _alertDuration(0),
    _pattern(nullptr),
    _patternLength(0),
    _patternIndex(0),
    _patternStepTime(0),
    _pulseState(false),
    _lastPulseTime(0),
    _pulseOnTime(100),
    _pulseOffTime(100)
{
}

void AlertManager::begin(int buzzerPin, int vibrationPin) {
    _buzzerPin = buzzerPin;
    _vibrationPin = vibrationPin;
    
    pinMode(_buzzerPin, OUTPUT);
    pinMode(_vibrationPin, OUTPUT);
    
    digitalWrite(_buzzerPin, LOW);
    digitalWrite(_vibrationPin, LOW);
    
    Serial.println("AlertManager initialized");
}

void AlertManager::update() {
    unsigned long now = millis();
    
    if (!_alertActive) {
        return;
    }
    
    // Check if alert duration has elapsed
    if (_alertDuration > 0 && (now - _alertStartTime) >= (unsigned long)_alertDuration) {
        stopAlert();
        return;
    }
    
    // Pulsing pattern
    if (now - _lastPulseTime >= (unsigned long)(_pulseState ? _pulseOnTime : _pulseOffTime)) {
        _pulseState = !_pulseState;
        _lastPulseTime = now;
        
        if (_pulseState) {
            // ON
            if (!_hapticOnly) {
                digitalWrite(_buzzerPin, HIGH);
            }
            digitalWrite(_vibrationPin, HIGH);
        } else {
            // OFF
            digitalWrite(_buzzerPin, LOW);
            digitalWrite(_vibrationPin, LOW);
        }
    }
}

void AlertManager::triggerAlert(int durationMs) {
    _alertActive = true;
    _hapticOnly = false;
    _alertStartTime = millis();
    _alertDuration = durationMs;
    _pulseState = true;
    _lastPulseTime = millis();
    
    // Start with both on
    digitalWrite(_buzzerPin, HIGH);
    digitalWrite(_vibrationPin, HIGH);
    
    // Fast pulse pattern for urgency
    _pulseOnTime = 100;
    _pulseOffTime = 50;
    
    Serial.println("ALERT triggered");
}

void AlertManager::triggerHapticOnly(int durationMs) {
    _alertActive = true;
    _hapticOnly = true;
    _alertStartTime = millis();
    _alertDuration = durationMs;
    _pulseState = true;
    _lastPulseTime = millis();
    
    // Vibration only
    digitalWrite(_vibrationPin, HIGH);
    
    _pulseOnTime = 150;
    _pulseOffTime = 100;
    
    Serial.println("HAPTIC alert triggered");
}

void AlertManager::stopAlert() {
    _alertActive = false;
    digitalWrite(_buzzerPin, LOW);
    digitalWrite(_vibrationPin, LOW);
}

void AlertManager::playTone(int frequency, int durationMs) {
    // Use ESP32 LEDC for tone generation
    ledcSetup(0, frequency, 8);  // Channel 0, 8-bit resolution
    ledcAttachPin(_buzzerPin, 0);
    ledcWrite(0, 128);  // 50% duty cycle
    
    delay(durationMs);
    
    ledcWrite(0, 0);
    ledcDetachPin(_buzzerPin);
    pinMode(_buzzerPin, OUTPUT);
    digitalWrite(_buzzerPin, LOW);
}

void AlertManager::playPattern(const int* pattern, int length) {
    // Pattern format: [freq1, dur1, freq2, dur2, ...]
    // freq=0 means pause
    for (int i = 0; i < length; i += 2) {
        int freq = pattern[i];
        int dur = pattern[i + 1];
        
        if (freq > 0) {
            playTone(freq, dur);
        } else {
            delay(dur);
        }
    }
}

// Predefined alert patterns
namespace AlertPatterns {
    // Urgent detection alert
    const int DETECTION[] = {
        2000, 100, 0, 50, 2000, 100, 0, 50, 2000, 100
    };
    const int DETECTION_LEN = 10;
    
    // Startup sound
    const int STARTUP[] = {
        800, 100, 1000, 100, 1200, 100, 1600, 200
    };
    const int STARTUP_LEN = 8;
    
    // Low battery warning
    const int LOW_BATTERY[] = {
        500, 500, 0, 500, 500, 500
    };
    const int LOW_BATTERY_LEN = 6;
    
    // Calibration complete
    const int CALIBRATION_DONE[] = {
        1000, 200, 1500, 200, 2000, 300
    };
    const int CALIBRATION_DONE_LEN = 6;
}

#endif // ALERT_MANAGER_H
