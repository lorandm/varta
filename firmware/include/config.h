/**
 * VARTA - Acoustic Drone Detector
 * Configuration Header
 * 
 * Adjust these values based on your hardware and deployment environment.
 */

#ifndef CONFIG_H
#define CONFIG_H

// =============================================================================
// HARDWARE CONFIGURATION
// =============================================================================

// ESP32-S3 Pin Assignments
// Microphones (I2S) - INMP441
#define I2S_SCK_PIN         14      // Serial Clock (shared)
#define I2S_WS_PIN          15      // Word Select (shared)
#define I2S_SD_PIN_MIC1     4       // Serial Data - Mic 1 (Front Left)
#define I2S_SD_PIN_MIC2     5       // Serial Data - Mic 2 (Front Right)
#define I2S_SD_PIN_MIC3     6       // Serial Data - Mic 3 (Rear Right)
#define I2S_SD_PIN_MIC4     7       // Serial Data - Mic 4 (Rear Left)

// OLED Display (I2C) - SSD1306
#define OLED_SDA_PIN        21
#define OLED_SCL_PIN        22
#define OLED_WIDTH          128
#define OLED_HEIGHT         64
#define OLED_ADDRESS        0x3C

// TFT Display (SPI) - ILI9341 (optional)
#define TFT_ENABLED         false
#define TFT_MOSI_PIN        23
#define TFT_SCK_PIN         18
#define TFT_CS_PIN          5
#define TFT_DC_PIN          16
#define TFT_RST_PIN         17

// LED Ring - WS2812B
#define LED_PIN             48
#define LED_COUNT           8
#define LED_BRIGHTNESS      50      // 0-255

// Alert Outputs
#define BUZZER_PIN          38
#define VIBRATION_PIN       39

// User Input
#define BUTTON_PIN          0       // Boot button

// Battery Monitoring
#define BATTERY_ADC_PIN     1
#define BATTERY_DIVIDER     2.0f    // Voltage divider ratio

// =============================================================================
// AUDIO CONFIGURATION
// =============================================================================

#define SAMPLE_RATE         44100   // Hz
#define SAMPLE_BITS         32      // I2S bit depth
#define FFT_SIZE            2048    // Must be power of 2
#define HOP_SIZE            512     // FFT hop (overlap = FFT_SIZE - HOP_SIZE)
#define MEL_BINS            128     // Mel frequency bins
#define SPEC_TIME_FRAMES    32      // Time frames for ML input (1 second)

// Microphone array geometry
#define MIC_SPACING_MM      50.0f   // Distance between adjacent mics
#define SPEED_OF_SOUND      343.0f  // m/s at 20°C

// =============================================================================
// DETECTION CONFIGURATION
// =============================================================================

// Detection thresholds
#define DETECTION_THRESHOLD_DB      65.0f   // Minimum signal level (dB SPL)
#define CONFIDENCE_THRESHOLD        0.75f   // ML model confidence (0-1)
#define DRONE_CLASS_INDEX           1       // Index of "drone" class in model output

// Drone acoustic signature ranges (Hz)
#define DRONE_FREQ_MIN              100     // Minimum frequency of interest
#define DRONE_FREQ_MAX              1000    // Maximum frequency of interest
#define MOTOR_FUNDAMENTAL_MIN       150     // Typical motor fundamental min
#define MOTOR_FUNDAMENTAL_MAX       400     // Typical motor fundamental max

// Alert behavior
#define ALERT_HOLDOFF_MS            2000    // Minimum time between alerts
#define ALERT_DURATION_MS           500     // Buzzer/vibration pulse duration
#define DETECTION_WINDOW_MS         4000    // Rolling window for detection
#define MIN_DETECTIONS_FOR_ALERT    3       // Detections needed in window

// Direction estimation
#define DIRECTION_SMOOTHING         0.3f    // EMA alpha for direction (0-1)
#define MIN_CORRELATION             0.5f    // Minimum cross-correlation for valid TDOA

// =============================================================================
// POWER MANAGEMENT
// =============================================================================

#define BATTERY_LOW_VOLTAGE         6.8f    // 2S low voltage warning (V)
#define BATTERY_CRITICAL_VOLTAGE    6.4f    // 2S critical - shutdown (V)
#define BATTERY_FULL_VOLTAGE        8.4f    // 2S full charge (V)

#define SLEEP_TIMEOUT_MS            0       // 0 = no auto-sleep
#define CPU_FREQ_MHZ                240     // ESP32-S3 frequency

// =============================================================================
// DEBUG CONFIGURATION
// =============================================================================

#define SERIAL_BAUD                 115200
#define DEBUG_ENABLED               true
#define DEBUG_PRINT_FFT             false
#define DEBUG_PRINT_DETECTION       true
#define DEBUG_PRINT_DIRECTION       true

// =============================================================================
// MODEL CONFIGURATION
// =============================================================================

#define MODEL_INPUT_WIDTH           MEL_BINS
#define MODEL_INPUT_HEIGHT          SPEC_TIME_FRAMES
#define MODEL_INPUT_CHANNELS        1
#define MODEL_ARENA_SIZE            (100 * 1024)    // TFLite arena size (bytes)

#endif // CONFIG_H
