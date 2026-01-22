/**
 * VARTA - Acoustic Drone Detector
 * Main Firmware
 * 
 * ESP32-S3 based acoustic detection system for fiber optic FPV drones.
 * Uses 4-microphone array with ML-based classification.
 * 
 * License: MIT
 */

#include <Arduino.h>
#include <driver/i2s.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <arduinoFFT.h>
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "config.h"
#include "model_data.h"
#include "audio_processor.h"
#include "direction_estimator.h"
#include "alert_manager.h"

// =============================================================================
// GLOBAL OBJECTS
// =============================================================================

// Display
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// LED Ring
Adafruit_NeoPixel ledRing(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Audio Processing
AudioProcessor audioProcessor;
DirectionEstimator directionEstimator;
AlertManager alertManager;

// TensorFlow Lite
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;
uint8_t tensorArena[MODEL_ARENA_SIZE];
tflite::AllOpsResolver resolver;

// State
enum SystemState {
    STATE_INIT,
    STATE_SCAN,
    STATE_ALERT,
    STATE_MONITOR,
    STATE_CALIBRATE,
    STATE_LOW_BATTERY,
    STATE_ERROR
};

SystemState currentState = STATE_INIT;
volatile bool newAudioReady = false;
float currentConfidence = 0.0f;
float currentDirection = 0.0f;
int detectionCount = 0;
unsigned long lastDetectionTime = 0;
unsigned long lastAlertTime = 0;
bool audioMuted = false;

// Audio buffers
float audioBuffer[4][FFT_SIZE];         // Per-microphone buffers
float melSpectrogram[MEL_BINS * SPEC_TIME_FRAMES];
int spectrogramIndex = 0;

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

void setupI2S();
void setupDisplay();
void setupLEDs();
void setupModel();
void readAudioSamples();
void processAudio();
float runInference();
void updateDisplay();
void updateLEDs(float direction, float confidence);
void handleButton();
float readBatteryVoltage();
void enterCalibrationMode();

// =============================================================================
// SETUP
// =============================================================================

void setup() {
    Serial.begin(SERIAL_BAUD);
    Serial.println("\n=== VARTA Acoustic Drone Detector ===");
    Serial.println("Initializing...");

    // Initialize GPIO
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(VIBRATION_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(BATTERY_ADC_PIN, INPUT);

    // Startup indication
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);

    // Initialize subsystems
    setupDisplay();
    setupLEDs();
    setupI2S();
    setupModel();

    // Initialize processors
    audioProcessor.begin(SAMPLE_RATE, FFT_SIZE, MEL_BINS);
    directionEstimator.begin(MIC_SPACING_MM, SPEED_OF_SOUND, SAMPLE_RATE);
    alertManager.begin(BUZZER_PIN, VIBRATION_PIN);

    // Self-test LED sequence
    for (int i = 0; i < LED_COUNT; i++) {
        ledRing.setPixelColor(i, ledRing.Color(0, 50, 0));
        ledRing.show();
        delay(100);
        ledRing.setPixelColor(i, 0);
    }
    ledRing.show();

    currentState = STATE_SCAN;
    Serial.println("Initialization complete. Entering SCAN mode.");

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("VARTA READY");
    display.println("Mode: SCAN");
    display.display();
}

// =============================================================================
// MAIN LOOP
// =============================================================================

void loop() {
    static unsigned long lastProcessTime = 0;
    unsigned long currentTime = millis();

    // Handle user input
    handleButton();

    // Check battery
    float batteryVoltage = readBatteryVoltage();
    if (batteryVoltage < BATTERY_CRITICAL_VOLTAGE) {
        currentState = STATE_LOW_BATTERY;
    }

    // State machine
    switch (currentState) {
        case STATE_SCAN:
        case STATE_ALERT:
            // Read and process audio at regular intervals
            if (currentTime - lastProcessTime >= (HOP_SIZE * 1000 / SAMPLE_RATE)) {
                lastProcessTime = currentTime;
                
                readAudioSamples();
                processAudio();
                
                // Run ML inference
                currentConfidence = runInference();
                
                // Estimate direction if detection
                if (currentConfidence >= CONFIDENCE_THRESHOLD) {
                    currentDirection = directionEstimator.estimateDirection(
                        audioBuffer[0], audioBuffer[1], 
                        audioBuffer[2], audioBuffer[3], 
                        FFT_SIZE
                    );
                    
                    detectionCount++;
                    lastDetectionTime = currentTime;
                    
                    #if DEBUG_PRINT_DETECTION
                    Serial.printf("DETECTION: conf=%.2f dir=%.1f° count=%d\n", 
                                  currentConfidence, currentDirection, detectionCount);
                    #endif
                }
                
                // Check if we should alert
                if (detectionCount >= MIN_DETECTIONS_FOR_ALERT && 
                    currentTime - lastAlertTime >= ALERT_HOLDOFF_MS) {
                    
                    currentState = STATE_ALERT;
                    lastAlertTime = currentTime;
                    
                    if (!audioMuted) {
                        alertManager.triggerAlert(ALERT_DURATION_MS);
                    } else {
                        alertManager.triggerHapticOnly(ALERT_DURATION_MS);
                    }
                    
                    Serial.println("*** ALERT: DRONE DETECTED ***");
                }
                
                // Decay detection count over time
                if (currentTime - lastDetectionTime > DETECTION_WINDOW_MS) {
                    detectionCount = 0;
                    if (currentState == STATE_ALERT) {
                        currentState = STATE_SCAN;
                    }
                }
            }
            
            updateDisplay();
            updateLEDs(currentDirection, currentConfidence);
            break;

        case STATE_MONITOR:
            // Real-time spectrogram display mode
            if (currentTime - lastProcessTime >= (HOP_SIZE * 1000 / SAMPLE_RATE)) {
                lastProcessTime = currentTime;
                readAudioSamples();
                processAudio();
                // Display spectrogram on TFT if available
            }
            updateDisplay();
            break;

        case STATE_CALIBRATE:
            enterCalibrationMode();
            currentState = STATE_SCAN;
            break;

        case STATE_LOW_BATTERY:
            display.clearDisplay();
            display.setCursor(0, 20);
            display.setTextSize(2);
            display.println("LOW BATT");
            display.setTextSize(1);
            display.printf("%.1fV", batteryVoltage);
            display.display();
            
            ledRing.fill(ledRing.Color(50, 0, 0));
            ledRing.show();
            
            delay(1000);
            break;

        case STATE_ERROR:
            display.clearDisplay();
            display.setCursor(0, 20);
            display.setTextSize(2);
            display.println("ERROR");
            display.display();
            delay(1000);
            break;

        default:
            break;
    }

    // Alert manager update
    alertManager.update();
}

// =============================================================================
// I2S AUDIO SETUP
// =============================================================================

void setupI2S() {
    Serial.println("Configuring I2S...");

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)SAMPLE_BITS,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = FFT_SIZE,
        .use_apll = true,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD_PIN_MIC1  // Primary mic for now
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("I2S driver install failed: %d\n", err);
        currentState = STATE_ERROR;
        return;
    }

    err = i2s_set_pin(I2S_NUM_0, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("I2S set pin failed: %d\n", err);
        currentState = STATE_ERROR;
        return;
    }

    Serial.println("I2S configured successfully");
}

// =============================================================================
// DISPLAY SETUP
// =============================================================================

void setupDisplay() {
    Serial.println("Initializing display...");

    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
        Serial.println("SSD1306 allocation failed");
        currentState = STATE_ERROR;
        return;
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("VARTA v1.0");
    display.println("Initializing...");
    display.display();

    Serial.println("Display initialized");
}

// =============================================================================
// LED SETUP
// =============================================================================

void setupLEDs() {
    Serial.println("Initializing LEDs...");
    ledRing.begin();
    ledRing.setBrightness(LED_BRIGHTNESS);
    ledRing.show();
    Serial.println("LEDs initialized");
}

// =============================================================================
// ML MODEL SETUP
// =============================================================================

void setupModel() {
    Serial.println("Loading ML model...");

    model = tflite::GetModel(drone_detector_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.printf("Model schema mismatch: %d vs %d\n", 
                      model->version(), TFLITE_SCHEMA_VERSION);
        currentState = STATE_ERROR;
        return;
    }

    interpreter = new tflite::MicroInterpreter(
        model, resolver, tensorArena, MODEL_ARENA_SIZE
    );

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        Serial.println("AllocateTensors() failed");
        currentState = STATE_ERROR;
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.printf("Model loaded. Input shape: [%d, %d, %d]\n",
                  input->dims->data[1], input->dims->data[2], input->dims->data[3]);
    Serial.printf("Arena used: %d bytes\n", interpreter->arena_used_bytes());
}

// =============================================================================
// AUDIO READING
// =============================================================================

void readAudioSamples() {
    size_t bytesRead = 0;
    int32_t rawSamples[FFT_SIZE];

    // Read from I2S
    esp_err_t result = i2s_read(I2S_NUM_0, rawSamples, sizeof(rawSamples), 
                                 &bytesRead, portMAX_DELAY);

    if (result == ESP_OK && bytesRead > 0) {
        int samplesRead = bytesRead / sizeof(int32_t);
        
        // Convert to float and normalize
        for (int i = 0; i < samplesRead && i < FFT_SIZE; i++) {
            // INMP441 is 24-bit in 32-bit frame, left-aligned
            float sample = (float)(rawSamples[i] >> 8) / 8388608.0f;  // Normalize to [-1, 1]
            audioBuffer[0][i] = sample;
        }
        
        // TODO: Read other 3 microphones via I2S multiplexing or additional I2S ports
        // For prototype, copy mic 1 to others (direction estimation won't work)
        memcpy(audioBuffer[1], audioBuffer[0], FFT_SIZE * sizeof(float));
        memcpy(audioBuffer[2], audioBuffer[0], FFT_SIZE * sizeof(float));
        memcpy(audioBuffer[3], audioBuffer[0], FFT_SIZE * sizeof(float));
    }
}

// =============================================================================
// AUDIO PROCESSING
// =============================================================================

void processAudio() {
    // Compute mel spectrogram from mic 1
    float melFrame[MEL_BINS];
    audioProcessor.computeMelSpectrogram(audioBuffer[0], FFT_SIZE, melFrame);

    // Add to rolling spectrogram buffer
    memcpy(&melSpectrogram[spectrogramIndex * MEL_BINS], melFrame, 
           MEL_BINS * sizeof(float));
    
    spectrogramIndex = (spectrogramIndex + 1) % SPEC_TIME_FRAMES;
}

// =============================================================================
// ML INFERENCE
// =============================================================================

float runInference() {
    if (interpreter == nullptr || input == nullptr || output == nullptr) {
        return 0.0f;
    }

    // Copy spectrogram to model input (normalize to 0-1 range)
    float* inputData = input->data.f;
    for (int t = 0; t < SPEC_TIME_FRAMES; t++) {
        int srcIndex = (spectrogramIndex + t) % SPEC_TIME_FRAMES;
        for (int f = 0; f < MEL_BINS; f++) {
            float val = melSpectrogram[srcIndex * MEL_BINS + f];
            // Normalize dB to 0-1 range (assuming -80 to 0 dB)
            val = (val + 80.0f) / 80.0f;
            val = constrain(val, 0.0f, 1.0f);
            inputData[t * MEL_BINS + f] = val;
        }
    }

    // Run inference
    if (interpreter->Invoke() != kTfLiteOk) {
        Serial.println("Inference failed");
        return 0.0f;
    }

    // Get drone class probability
    float* outputData = output->data.f;
    float droneConfidence = outputData[DRONE_CLASS_INDEX];

    return droneConfidence;
}

// =============================================================================
// DISPLAY UPDATE
// =============================================================================

void updateDisplay() {
    static unsigned long lastDisplayUpdate = 0;
    if (millis() - lastDisplayUpdate < 100) return;  // 10 Hz update
    lastDisplayUpdate = millis();

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);

    // Status line
    display.print("VARTA ");
    switch (currentState) {
        case STATE_SCAN:    display.println("SCAN"); break;
        case STATE_ALERT:   display.println("ALERT!"); break;
        case STATE_MONITOR: display.println("MONITOR"); break;
        default:            display.println("---"); break;
    }

    // Confidence bar
    display.setCursor(0, 12);
    display.print("Conf: ");
    int barWidth = (int)(currentConfidence * 60);
    display.drawRect(35, 12, 62, 8, SSD1306_WHITE);
    display.fillRect(36, 13, barWidth, 6, SSD1306_WHITE);

    // Direction
    display.setCursor(0, 24);
    display.printf("Dir: %.0f%c", currentDirection, 0xF8);  // Degree symbol

    // Detection count
    display.setCursor(0, 36);
    display.printf("Det: %d/%d", detectionCount, MIN_DETECTIONS_FOR_ALERT);

    // Battery
    float batt = readBatteryVoltage();
    display.setCursor(0, 48);
    display.printf("Batt: %.1fV", batt);
    if (audioMuted) {
        display.setCursor(80, 48);
        display.print("MUTE");
    }

    // Alert indicator
    if (currentState == STATE_ALERT) {
        display.fillRect(100, 0, 28, 10, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
        display.setCursor(102, 1);
        display.print("!!!");
        display.setTextColor(SSD1306_WHITE);
    }

    display.display();
}

// =============================================================================
// LED UPDATE
// =============================================================================

void updateLEDs(float direction, float confidence) {
    ledRing.clear();

    if (currentState == STATE_ALERT) {
        // Map direction (0-360) to LED index (0-7)
        // Assuming LED 0 is "forward"
        int ledIndex = ((int)((direction + 22.5f) / 45.0f)) % LED_COUNT;
        
        // Intensity based on confidence
        int intensity = (int)(confidence * 255);
        
        // Primary LED (threat direction)
        ledRing.setPixelColor(ledIndex, ledRing.Color(intensity, 0, 0));
        
        // Adjacent LEDs at lower intensity
        int prev = (ledIndex - 1 + LED_COUNT) % LED_COUNT;
        int next = (ledIndex + 1) % LED_COUNT;
        ledRing.setPixelColor(prev, ledRing.Color(intensity / 3, 0, 0));
        ledRing.setPixelColor(next, ledRing.Color(intensity / 3, 0, 0));
        
    } else if (currentState == STATE_SCAN) {
        // Ambient level indicator - subtle blue pulse
        static uint8_t breathe = 0;
        static int8_t breatheDir = 1;
        breathe += breatheDir * 2;
        if (breathe >= 30 || breathe <= 0) breatheDir = -breatheDir;
        
        for (int i = 0; i < LED_COUNT; i++) {
            ledRing.setPixelColor(i, ledRing.Color(0, 0, breathe));
        }
    }

    ledRing.show();
}

// =============================================================================
// BUTTON HANDLING
// =============================================================================

void handleButton() {
    static unsigned long buttonPressTime = 0;
    static bool buttonWasPressed = false;
    static int quickPressCount = 0;
    static unsigned long lastQuickPress = 0;

    bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);

    if (buttonPressed && !buttonWasPressed) {
        // Button just pressed
        buttonPressTime = millis();
        buttonWasPressed = true;
    }
    else if (!buttonPressed && buttonWasPressed) {
        // Button just released
        unsigned long pressDuration = millis() - buttonPressTime;
        buttonWasPressed = false;

        if (pressDuration >= 3000) {
            // Long press - calibration mode
            Serial.println("Long press - entering calibration");
            currentState = STATE_CALIBRATE;
        }
        else if (pressDuration >= 50) {
            // Short press
            if (millis() - lastQuickPress < 500) {
                quickPressCount++;
            } else {
                quickPressCount = 1;
            }
            lastQuickPress = millis();

            if (quickPressCount >= 2) {
                // Double press - toggle mute
                audioMuted = !audioMuted;
                Serial.printf("Audio mute: %s\n", audioMuted ? "ON" : "OFF");
                quickPressCount = 0;
            }
        }
    }

    // Single press action (after timeout)
    if (quickPressCount == 1 && millis() - lastQuickPress > 500) {
        // Cycle display mode
        if (currentState == STATE_SCAN) {
            currentState = STATE_MONITOR;
        } else if (currentState == STATE_MONITOR) {
            currentState = STATE_SCAN;
        }
        Serial.printf("Mode changed to: %d\n", currentState);
        quickPressCount = 0;
    }
}

// =============================================================================
// BATTERY MONITORING
// =============================================================================

float readBatteryVoltage() {
    int adcValue = analogRead(BATTERY_ADC_PIN);
    float voltage = (adcValue / 4095.0f) * 3.3f * BATTERY_DIVIDER;
    return voltage;
}

// =============================================================================
// CALIBRATION
// =============================================================================

void enterCalibrationMode() {
    Serial.println("=== CALIBRATION MODE ===");
    
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("CALIBRATING...");
    display.println("Keep quiet for");
    display.println("30 seconds");
    display.display();

    // Collect ambient noise profile
    float noiseFloor[MEL_BINS] = {0};
    int sampleCount = 0;
    unsigned long startTime = millis();

    while (millis() - startTime < 30000) {
        readAudioSamples();
        
        float melFrame[MEL_BINS];
        audioProcessor.computeMelSpectrogram(audioBuffer[0], FFT_SIZE, melFrame);
        
        // Running average
        for (int i = 0; i < MEL_BINS; i++) {
            noiseFloor[i] = (noiseFloor[i] * sampleCount + melFrame[i]) / (sampleCount + 1);
        }
        sampleCount++;

        // Progress indicator
        int progress = (millis() - startTime) / 300;  // 0-100
        display.fillRect(0, 50, progress * 1.28, 10, SSD1306_WHITE);
        display.display();

        delay(10);
    }

    // Store noise profile
    audioProcessor.setNoiseFloor(noiseFloor);

    display.clearDisplay();
    display.setCursor(0, 20);
    display.println("CALIBRATION");
    display.println("COMPLETE");
    display.display();
    delay(2000);

    Serial.println("Calibration complete");
}
