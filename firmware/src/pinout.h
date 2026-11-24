/**
 * Pinout definities voor ESP32 Rover
 * Hardware: ESP32 DevKitC V1 (USB-C, WROOM-32)
 * 
 * Let op: Alle pinnen zijn GPIO nummers (niet fysieke pin nummers!)
 * Check datasheet DevKitC V1 voor mapping.
 */

#ifndef PINOUT_H
#define PINOUT_H

// ==================== MOTOR DRIVER (TB6612FNG) ====================
// Kanaal A = Linker motor, Kanaal B = Rechter motor
// TB6612FNG pinout: PWMA, AIN1, AIN2, STBY, BIN1, BIN2, PWMB
// Geen conflicten, alle PWM capable:

#define PIN_MOTOR_STBY      4    // GPIO4  - STBY
#define PIN_MOTOR_AIN1      5    // GPIO5  - AIN1
#define PIN_MOTOR_AIN2      18   // GPIO18 - AIN2
#define PIN_MOTOR_PWMA      19   // GPIO19 - PWMA (LEDc Ch 0)
#define PIN_MOTOR_BIN1      21   // GPIO21 - BIN1
#define PIN_MOTOR_BIN2      22   // GPIO22 - BIN2
#define PIN_MOTOR_PWMB      23   // GPIO23 - PWMB (LEDc Ch 1)

// PWM instellingen
#define MOTOR_PWM_FREQ      20000   // 20 kHz (inaudible, efficiënt voor TB6612FNG)
#define MOTOR_PWM_RES       10      // 10-bit resolutie (0-1023)
#define MOTOR_PWM_MAX       1023    // Max duty cycle

// ==================== ENCODERS (TT Motor met encoder disc) ====================
// Encoder A en B per motor (quadrature)
// Gebruik GPIO met interrupt capability
// VERPLAATST naar vrije pinnen om ADC1 vrij te maken voor line sensors + battery
#define PIN_ENC_LA          14   // Encoder Linker A (GPIO14 - free, interrupt OK)
#define PIN_ENC_LB          25   // Encoder Linker B (GPIO25 - free, interrupt OK, ADC2 but digital)
#define PIN_ENC_RA          26   // Encoder Rechter A (GPIO26 - free, interrupt OK, ADC2 but digital)
#define PIN_ENC_RB          27   // Encoder Rechter B (GPIO27 - free, interrupt OK, ADC2 but digital)

// Encoder specificaties (TT motor 1:48 + encoder disc)
// Pulses per revolution (PPR) van encoder disc: typisch 10-20
// Gear ratio: 1:48
// Wheel diameter: 66mm
#define ENCODER_PPR         20      // Pulses per motor revolution (encoder disc)
#define GEAR_RATIO          48.0    // Gearbox ratio
#define WHEEL_DIAMETER_MM   66.0    // Wiel diameter in mm
#define WHEEL_CIRCUMFERENCE_MM (WHEEL_DIAMETER_MM * 3.14159265359)

// Alleen kanaal A gebruiken voor snelheid (geen double-count op A+B)
// Richting kan via kanaal B worden bepaald indien nodig

// ==================== HC-SR04 ULTRASONIC SENSORS ====================
// Voor + Achter
// Trig = output, Echo = input (5V tolerant met spanningsdeler!)

#define PIN_US_FRONT_TRIG   13   // Voor - Trig
#define PIN_US_FRONT_ECHO   12   // Voor - Echo (via 1k/2k spanningsdeler naar 3.3V!)
#define PIN_US_REAR_TRIG    2    // Achter - Trig
#define PIN_US_REAR_ECHO    15   // Achter - Echo (via 1k/2k spanningsdeler!)

#define US_TIMEOUT_US       30000  // 30ms timeout (~5m max)
#define US_MAX_DIST_CM      400    // Max afstand in cm

// ==================== MPU6050 (I2C) ====================
// Wire1 op GPIO 16/17 (niet Wire0 want conflict met motor driver BIN1/BIN2 op 21/22)
#define PIN_I2C_SDA         16   // GPIO16 - I2C SDA (voor MPU6050)
#define PIN_I2C_SCL         17   // GPIO17 - I2C SCL (voor MPU6050)

// MPU6050 I2C adres
#define MPU6050_ADDR        0x68

// ==================== TCRT5000 LINE SENSORS (4-kanaals) ====================
// 4 analoge ingangen voor lijnvolgen
// Layout: [Linksbuiten | Linksbinnen | Rechtsbinnen | Rechtsbuiten]
// Output: 0-3.3V (pull-up op module, laag = zwart lijn)
//
// ALLE OP ADC1 PINNEN (werken met WiFi):
// GPIO 34, 35, 36, 39 zijn ADC1 en nu vrij (encoders verplaatst)

#define PIN_LINE_L2         36   // Linksbuiten - ADC1_CH0 (GPIO36)
#define PIN_LINE_L1         39   // Linksbinnen - ADC1_CH3 (GPIO39)
#define PIN_LINE_R1         34   // Rechtsbinnen - ADC1_CH6 (GPIO34)
#define PIN_LINE_R2         35   // Rechtsbuiten - ADC1_CH7 (GPIO35)

// ADC instellingen
#define LINE_ADC_ATTEN      ADC_11db  // 0-3.3V range
#define LINE_THRESHOLD      2000      // Kalibratie waarde (0-4095)

// ==================== BATTERIJ MONITORING ====================
// Spanningsdeler: 2x 100k + 1x 10k = 210k totaal
// Meet 2S (max 8.4V) -> ESP32 ADC (max 3.3V)
// Vout = Vin * (10k / 210k) = Vin / 21
// Max Vin = 8.4V -> Vout = 0.4V (veilig)
//
// OP ADC1 (GPIO 33) - werkt normaal met WiFi, geen workaround nodig

#define PIN_BAT_VOLTAGE     33   // GPIO33 (ADC1_CH5) - input only, werkt met WiFi

// Berekening
#define BAT_DIVIDER_RATIO   21.0   // (100k+100k+10k) / 10k = 21
#define BAT_MIN_VOLTAGE     6.6    // 2S min (3.3V per cel)
#define BAT_MAX_VOLTAGE     8.4    // 2S max (4.2V per cel)
#define BAT_LOW_WARNING     7.0    // Waarschuwing niveau
#define BAT_CRITICAL        6.6    // Noodstop niveau

// ==================== STATUS LED & BUZZER ====================
#define PIN_STATUS_LED      2    // Onboard LED (GPIO2) - active HIGH
#define PIN_BUZZER          32   // GPIO32 (free, PWM capable, geen boot conflict)

// ==================== NOODSTOP ====================
#define PIN_EMERGENCY_STOP  3    // GPIO3 (RX0, free met USB_CDC_ON_BOOT, input pull-up)

// ==================== WIFI / WEBSOCKET ====================
#define WIFI_AP_SSID        "Rover_AP"
#define WIFI_AP_PASSWORD    "rover123"
#define WIFI_AP_CHANNEL     1
#define WS_PORT             80
#define WS_PATH             "/ws"

// ==================== CORE TOEWIJZING ====================
// Core 0: Real-time motor control, sensor polling, PID (1kHz loop)
// Core 1: WiFi, WebSocket, JSON, OTA, logging

#define TASK_CORE_0         0
#define TASK_CORE_1         1

// Loop frequenties
#define LOOP_RATE_HZ        1000   // Core 0: 1kHz real-time loop
#define SENSOR_RATE_HZ      100    // Sensor polling 100Hz
#define TELEMETRY_RATE_HZ   50     // Telemetrie naar Pi 50Hz
#define WS_HEARTBEAT_MS     1000   // WebSocket heartbeat

#endif // PINOUT_H