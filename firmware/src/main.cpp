/**
 * ESP32 Rover Firmware - Main Entry Point
 * 
 * Architectuur:
 * - Core 0: Real-time loop (1kHz) - Motor PWM, PID, Encoders, Sensoren, Safety
 * - Core 1: WiFi AP + WebSocket Server - JSON communicatie met Pi
 * 
 * Hardware: Zie pinout.h
 * 
 * Protocol (JSON over WebSocket):
 * Pi → ESP32 (10-50Hz): {"T":100,"L":180,"R":175,"M":1}
 * ESP32 → Pi (50-100Hz): {"t":12345,"dF":45,"dR":120,"bat":11.3,"imu":{"r":1,"p":-2},"enc":{"L":450,"R":448}}
 */

#include <Arduino.h>
#include "pinout.h"

// ==================== LIBRARIES ====================
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <MPU6050.h>
#include <PID_v1.h>
#include <esp_task_wdt.h>  // Task Watchdog Timer

// ==================== GLOBAL OBJECTS ====================
AsyncWebServer server(WS_PORT);
AsyncWebSocket ws("/ws");
MPU6050 mpu(&Wire1);  // Use Wire1 (GPIO 16/17) not default Wire (GPIO 21/22)

// ==================== MOTOR CONTROL ====================
struct MotorState {
  int pwm_L = 0;      // -1023..1023
  int pwm_R = 0;
  bool enabled = false;
  uint32_t last_cmd_ms = 0;
} motor;

// Encoder counts (volatile for ISR)
volatile long enc_count_L = 0;
volatile long enc_count_R = 0;

// Speed calculation (pulses per loop interval)
float speed_L = 0.0;  // mm/s
float speed_R = 0.0;

// PID for speed control
double pid_setpoint_L = 0, pid_input_L = 0, pid_output_L = 0;
double pid_setpoint_R = 0, pid_input_R = 0, pid_output_R = 0;
PID pid_L(&pid_input_L, &pid_output_L, &pid_setpoint_L, 1.0, 0.5, 0.1, DIRECT);
PID pid_R(&pid_input_R, &pid_output_R, &pid_setpoint_R, 1.0, 0.5, 0.1, DIRECT);

// ==================== SENSORS ====================
struct SensorData {
  // Ultrasonic
  int dist_front = -1;    // cm
  int dist_rear = -1;     // cm
  uint32_t us_last_ms = 0;
  
  // MPU6050
  float roll = 0, pitch = 0, yaw = 0;
  float accel_x = 0, accel_y = 0, accel_z = 0;
  float gyro_x = 0, gyro_y = 0, gyro_z = 0;
  uint32_t imu_last_ms = 0;
  
  // Line sensors (raw ADC 0-4095)
  int line_raw[4] = {0, 0, 0, 0};
  float line_pos = 0.0;   // -1.0 (links) .. +1.0 (rechts)
  uint32_t line_last_ms = 0;
  
  // Battery
  float bat_voltage = 0.0;
  uint32_t bat_last_ms = 0;
  
  // Encoders
  long enc_L = 0, enc_R = 0;
} sensors;

// ==================== COMMUNICATION ====================
struct Command {
  int throttle = 0;    // -100..100
  int steer = 0;       // -100..100
  int mode = 0;        // 0=manual, 1=auto
  uint32_t timestamp = 0;
} cmd;

bool ws_connected = false;
uint32_t last_telemetry_ms = 0;
uint32_t last_ws_heartbeat_ms = 0;

// ==================== SAFETY ====================
bool emergency_stop = false;
uint32_t last_cmd_ms = 0;
const uint32_t CMD_TIMEOUT_MS = 500;  // Noodstop na 500ms geen commando

// ==================== ENCODER ISRs ====================
// Alleen kanaal A gebruiken voor snelheid (geen double-count op A+B)
// Kanaal B kan voor richting worden gebruikt maar niet nodig voor snelheid
// millis() NIET in ISR gebruiken - alleen teller ophogen
void IRAM_ATTR enc_isr_LA() { enc_count_L++; }
void IRAM_ATTR enc_isr_LB() { /* Richting detectie - niet gebruikt voor snelheid */ }
void IRAM_ATTR enc_isr_RA() { enc_count_R++; }
void IRAM_ATTR enc_isr_RB() { /* Richting detectie - niet gebruikt voor snelheid */ }

// ==================== MOTOR HELPERS ====================
void motor_set_pwm(int pwm_L, int pwm_R) {
  pwm_L = constrain(pwm_L, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
  pwm_R = constrain(pwm_R, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
  
  // Linker motor
  if (pwm_L >= 0) {
    digitalWrite(PIN_MOTOR_AIN1, HIGH);
    digitalWrite(PIN_MOTOR_AIN2, LOW);
  } else {
    digitalWrite(PIN_MOTOR_AIN1, LOW);
    digitalWrite(PIN_MOTOR_AIN2, HIGH);
  }
  ledcWrite(0, abs(pwm_L));
  
  // Rechter motor
  if (pwm_R >= 0) {
    digitalWrite(PIN_MOTOR_BIN1, HIGH);
    digitalWrite(PIN_MOTOR_BIN2, LOW);
  } else {
    digitalWrite(PIN_MOTOR_BIN1, LOW);
    digitalWrite(PIN_MOTOR_BIN2, HIGH);
  }
  ledcWrite(1, abs(pwm_R));
}

void motor_enable(bool en) {
  motor.enabled = en;
  digitalWrite(PIN_MOTOR_STBY, en ? HIGH : LOW);
  if (!en) motor_set_pwm(0, 0);
}

// Differential drive: throttle + steer (open-loop, for manual mode)
void motor_drive(int throttle, int steer) {
  // throttle: -100..100, steer: -100..100
  int left = throttle + steer;
  int right = throttle - steer;
  
  // Schaal naar PWM range
  left = map(constrain(left, -200, 200), -200, 200, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
  right = map(constrain(right, -200, 200), -200, 200, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
  
  motor_set_pwm(left, right);
}

// PID speed control: converteer throttle naar snelheid setpoints
void motor_drive_pid(int throttle, int steer) {
  // Converteer throttle (-100..100) naar snelheid setpoint (mm/s)
  // Max snelheid schatting: wiel omtrek * max RPM
  // 66mm * PI * (max motor RPM / 48) - schatting ~500mm/s max
  const float MAX_SPEED_MM_S = 500.0;
  
  float speed_setpoint = (throttle / 100.0) * MAX_SPEED_MM_S;
  float steer_factor = steer / 100.0;  // -1.0 .. 1.0
  
  pid_setpoint_L = speed_setpoint * (1.0 + steer_factor);
  pid_setpoint_R = speed_setpoint * (1.0 - steer_factor);
  
  // PID output wordt in update_pid_speed() berekend en direct toegepast
  // via motor_set_pwm() met PID correctie
  apply_pid_output();
}

// Pas PID output toe op motoren
void apply_pid_output() {
  // pid_output is PWM correctie (-1023..1023)
  // We voegen dit toe aan een basis PWM of gebruiken het direct
  // Voor nu: PID output IS de motor PWM (snelheidsregeling)
  int pwm_L = constrain((int)pid_output_L, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
  int pwm_R = constrain((int)pid_output_R, -MOTOR_PWM_MAX, MOTOR_PWM_MAX);
  
  motor_set_pwm(pwm_L, pwm_R);
}

// ==================== ULTRASONIC ====================
int read_ultrasonic(int trig_pin, int echo_pin) {
  digitalWrite(trig_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(trig_pin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig_pin, LOW);
  
  unsigned long duration = pulseIn(echo_pin, HIGH, US_TIMEOUT_US);
  if (duration == 0) return -1;
  
  int dist_cm = duration * 0.0343 / 2;  // cm
  return constrain(dist_cm, 2, US_MAX_DIST_CM);
}

// ==================== LINE SENSORS ====================
// ALLE 4 OP ADC1 PINNEN (GPIO 36, 39, 34, 35) - werken normaal met WiFi
// Layout: [L2, L1, R1, R2] = indexes [0, 1, 2, 3]
void read_line_sensors() {
  // Alle 4 direct lezen (ADC1, geen WiFi conflict)
  sensors.line_raw[0] = analogRead(PIN_LINE_L2);  // GPIO36 - Linksbuiten
  sensors.line_raw[1] = analogRead(PIN_LINE_L1);  // GPIO39 - Linksbinnen
  sensors.line_raw[2] = analogRead(PIN_LINE_R1);  // GPIO34 - Rechtsbinnen
  sensors.line_raw[3] = analogRead(PIN_LINE_R2);  // GPIO35 - Rechtsbuiten
  
  // Gewogen gemiddelde voor lijnpositie (-1.0 .. +1.0)
  // Gewichten: L2=-1.5, L1=-0.5, R1=+0.5, R2=+1.5
  long weighted_sum = 0;
  long total = 0;
  int weights[4] = {-150, -50, 50, 150};  // x100 voor integer math
  
  for (int i = 0; i < 4; i++) {
    int val = 4095 - sensors.line_raw[i];  // Inverteren: hoog = lijn
    if (val > LINE_THRESHOLD) {
      weighted_sum += val * weights[i];
      total += val;
    }
  }
  
  if (total > 0) {
    sensors.line_pos = (float)weighted_sum / (float)total / 100.0;
  } else {
    sensors.line_pos = 0.0;
  }
  sensors.line_last_ms = millis();
}

// ==================== BATTERY ====================
// OP ADC1 (GPIO 33) - werkt normaal met WiFi, geen workaround nodig
float read_battery_voltage() {
  int adc = analogRead(PIN_BAT_VOLTAGE);
  float v_adc = adc * 3.3 / 4095.0;
  return v_adc * BAT_DIVIDER_RATIO;
}

// ==================== MPU6050 ====================
void read_mpu6050() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
  
  // Converteer naar fysieke eenheden
  sensors.accel_x = ax / 16384.0;  // ±2g range
  sensors.accel_y = ay / 16384.0;
  sensors.accel_z = az / 16384.0;
  sensors.gyro_x = gx / 131.0;     // ±250°/s range
  sensors.gyro_y = gy / 131.0;
  sensors.gyro_z = gz / 131.0;
  
  // Eenvoudige complementaire filter voor roll/pitch
  // (voor yaw heb je magnetometer nodig)
  float dt = 0.01;  // 100Hz loop
  float acc_roll = atan2(sensors.accel_y, sensors.accel_z) * 180.0 / PI;
  float acc_pitch = atan2(-sensors.accel_x, sqrt(sensors.accel_y*sensors.accel_y + sensors.accel_z*sensors.accel_z)) * 180.0 / PI;
  
  const float alpha = 0.98;
  sensors.roll = alpha * (sensors.roll + sensors.gyro_x * dt) + (1 - alpha) * acc_roll;
  sensors.pitch = alpha * (sensors.pitch + sensors.gyro_y * dt) + (1 - alpha) * acc_pitch;
  sensors.yaw += sensors.gyro_z * dt;  // Drift!
  
  sensors.imu_last_ms = millis();
}

// ==================== PID SPEED CONTROL ====================
void update_pid_speed() {
  // Bereken snelheid uit encoder pulsen per loop interval
  // Pulses per mm = (ENCODER_PPR * GEAR_RATIO) / WHEEL_CIRCUMFERENCE_MM
  const float pulses_per_mm = (ENCODER_PPR * GEAR_RATIO) / WHEEL_CIRCUMFERENCE_MM;
  const float dt = 0.001;  // 1kHz loop = 1ms
  
  static long last_enc_L = 0, last_enc_R = 0;
  long delta_L = enc_count_L - last_enc_L;
  long delta_R = enc_count_R - last_enc_R;
  last_enc_L = enc_count_L;
  last_enc_R = enc_count_R;
  
  speed_L = (delta_L / pulses_per_mm) / dt;  // mm/s
  speed_R = (delta_R / pulses_per_mm) / dt;
  
  // PID input = gemeten snelheid, setpoint = gewenste snelheid
  pid_input_L = speed_L;
  pid_input_R = speed_R;
  
  pid_L.Compute();
  pid_R.Compute();
  
  // PID output direct toepassen op motoren (alleen in auto mode)
  if (cmd.mode == 1) {
    apply_pid_output();
  }
}

// ==================== SAFETY CHECKS ====================
void safety_check() {
  // 1. Commando timeout (alleen in auto mode of als motors enabled)
  if (motor.enabled && (millis() - last_cmd_ms > CMD_TIMEOUT_MS)) {
    Serial.println("SAFETY: Command timeout - EMERGENCY STOP");
    motor_enable(false);
    emergency_stop = true;
  }
  
  // 2. Batterij laag
  if (sensors.bat_voltage < BAT_CRITICAL) {
    Serial.printf("SAFETY: Battery critical %.2fV - EMERGENCY STOP\n", sensors.bat_voltage);
    motor_enable(false);
    emergency_stop = true;
  } else if (sensors.bat_voltage < BAT_LOW_WARNING) {
    // Alleen waarschuwing via telemetrie
  }
  
  // 3. Ultrasonic noodstop (te dichtbij)
  if (sensors.dist_front > 0 && sensors.dist_front < 10) {
    Serial.println("SAFETY: Front obstacle < 10cm");
    // Optioneel: motor_enable(false);
  }
  
  // 4. Emergency stop pin wordt in core0_task gecheckt (1kHz)
}

// ==================== WEBSOCKET HANDLERS ====================
void ws_on_event(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                 AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      ws_connected = true;
      Serial.printf("WS Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      ws_connected = false;
      Serial.printf("WS Client #%u disconnected\n", client->id());
      // Safety: emergency stop on disconnect
      motor_enable(false);
      emergency_stop = true;
      break;
    case WS_EVT_DATA:
      if (len > 0) {
        data[len] = 0;  // Null-terminate
        handle_ws_command((char*)data);
      }
      break;
    default:
      break;
  }
}

void handle_ws_command(const char* json_str) {
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, json_str);
  if (err) {
    Serial.printf("WS JSON parse error: %s\n", err.c_str());
    return;
  }
  
  // Protocol: {"T":throttle,"S":steer,"M":mode,"E":estop_clear}
  // M: 0=manual, 1=auto (PID), 2=clear emergency stop
  cmd.throttle = doc["T"] | 0;
  cmd.steer = doc["S"] | 0;
  cmd.mode = doc["M"] | 0;
  bool estop_clear = doc["E"] | false;
  cmd.timestamp = doc["t"] | millis();
  
  last_cmd_ms = millis();
  
  if (cmd.mode == 2 || estop_clear) {
    // Clear emergency stop
    emergency_stop = false;
    Serial.println("Emergency stop CLEARED via command");
    // Don't start motors automatically - wait for next drive command
  } else if (cmd.mode == 0) {
    // Manual mode: direct motor control (open-loop)
    motor_drive(cmd.throttle, cmd.steer);
  } else if (cmd.mode == 1) {
    // Auto mode: PID speed control
    motor_drive_pid(cmd.throttle, cmd.steer);
  }
  
  // Debug echo
  Serial.printf("CMD: T=%d S=%d M=%d E=%d\n", cmd.throttle, cmd.steer, cmd.mode, estop_clear);
}

// ==================== TELEMETRY ====================
void send_telemetry() {
  if (!ws_connected) return;
  
  StaticJsonDocument<512> doc;
  doc["t"] = millis();
  doc["dF"] = sensors.dist_front;
  doc["dR"] = sensors.dist_rear;
  doc["bat"] = round(sensors.bat_voltage * 10) / 10.0;
  doc["imu"]["r"] = round(sensors.roll * 10) / 10.0;
  doc["imu"]["p"] = round(sensors.pitch * 10) / 10.0;
  doc["imu"]["y"] = round(sensors.yaw * 10) / 10.0;
  doc["enc"]["L"] = enc_count_L;
  doc["enc"]["R"] = enc_count_R;
  doc["spd"]["L"] = round(speed_L);
  doc["spd"]["R"] = round(speed_R);
  doc["line"] = round(sensors.line_pos * 100) / 100.0;
  doc["mode"] = cmd.mode;
  doc["estop"] = emergency_stop;
  
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}

// ==================== CORE 0 TASK: REAL-TIME LOOP ====================
void core0_task(void *pvParameters) {
  Serial.println("Core 0: Real-time loop started (1kHz)");
  
  // Enable Task Watchdog Timer for this task
  esp_task_wdt_init(3, true);  // 3 second timeout, panic on timeout
  esp_task_wdt_add(NULL);      // Add current task to watchdog
  
  const TickType_t interval = pdMS_TO_TICKS(1);  // 1ms = 1kHz
  TickType_t last_wake = xTaskGetTickCount();
  
  for (;;) {
    // Feed watchdog every loop
    esp_task_wdt_reset();
    
    // 1. Sensor polling (100Hz)
    static uint32_t sensor_counter = 0;
    if (++sensor_counter >= 10) {  // elke 10ms = 100Hz
      sensor_counter = 0;
      sensors.dist_front = read_ultrasonic(PIN_US_FRONT_TRIG, PIN_US_FRONT_ECHO);
      sensors.dist_rear = read_ultrasonic(PIN_US_REAR_TRIG, PIN_US_REAR_ECHO);
      read_line_sensors();
      read_mpu6050();
      
      // Battery at 100Hz (ADC1, no workaround needed)
      sensors.bat_voltage = read_battery_voltage();
    }
    
    // 2. PID speed control (1kHz)
    update_pid_speed();
    
    // 3. Safety check (1kHz)
    safety_check();
    
    // 4. Emergency stop pin check (1kHz)
    if (digitalRead(PIN_EMERGENCY_STOP) == LOW) {  // Active LOW with pull-up
      if (!emergency_stop) {
        Serial.println("SAFETY: Emergency stop button pressed!");
        motor_enable(false);
        emergency_stop = true;
      }
    }
    
    // 5. Encoder snapshot voor telemetrie
    sensors.enc_L = enc_count_L;
    sensors.enc_R = enc_count_R;
    
    // Precise 1kHz timing
    vTaskDelayUntil(&last_wake, interval);
  }
}

// ==================== CORE 1: MAIN LOOP (WIFI/WS) ====================
void setup() {
  // Serial voor debug
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== ESP32 ROVER STARTING ===");
  
  // ==================== PIN INIT ====================
  // Motor driver
  pinMode(PIN_MOTOR_STBY, OUTPUT);
  pinMode(PIN_MOTOR_AIN1, OUTPUT);
  pinMode(PIN_MOTOR_AIN2, OUTPUT);
  pinMode(PIN_MOTOR_BIN1, OUTPUT);
  pinMode(PIN_MOTOR_BIN2, OUTPUT);
  
  // PWM setup (LEDc)
  ledcSetup(0, MOTOR_PWM_FREQ, MOTOR_PWM_RES);  // Channel 0 -> PWMA
  ledcSetup(1, MOTOR_PWM_FREQ, MOTOR_PWM_RES);  // Channel 1 -> PWMB
  ledcAttachPin(PIN_MOTOR_PWMA, 0);
  ledcAttachPin(PIN_MOTOR_PWMB, 1);
  
  motor_enable(false);
  
  // Encoders (interrupts - alleen kanaal A voor snelheid)
  pinMode(PIN_ENC_LA, INPUT_PULLUP);
  pinMode(PIN_ENC_LB, INPUT_PULLUP);  // Voor richting (niet gebruikt voor snelheid)
  pinMode(PIN_ENC_RA, INPUT_PULLUP);
  pinMode(PIN_ENC_RB, INPUT_PULLUP);  // Voor richting (niet gebruikt voor snelheid)
  attachInterrupt(PIN_ENC_LA, enc_isr_LA, RISING);
  attachInterrupt(PIN_ENC_RA, enc_isr_RA, RISING);
  // Kanaal B interrupts niet aangesloten voor snelheid (voorkomt double-count)
  
  // Ultrasonic
  pinMode(PIN_US_FRONT_TRIG, OUTPUT);
  pinMode(PIN_US_FRONT_ECHO, INPUT);
  pinMode(PIN_US_REAR_TRIG, OUTPUT);
  pinMode(PIN_US_REAR_ECHO, INPUT);
  digitalWrite(PIN_US_FRONT_TRIG, LOW);
  digitalWrite(PIN_US_REAR_TRIG, LOW);
  
  // Line sensors (ADC)
  analogReadResolution(12);  // 0-4095
  analogSetAttenuation(LINE_ADC_ATTEN);
  pinMode(PIN_LINE_L2, INPUT);
  pinMode(PIN_LINE_L1, INPUT);
  pinMode(PIN_LINE_R1, INPUT);
  pinMode(PIN_LINE_R2, INPUT);
  
  // Battery ADC (ADC1 - GPIO33)
  pinMode(PIN_BAT_VOLTAGE, INPUT);
  analogSetAttenuation(ADC_11db);
  
  // Buzzer
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
  
  // Emergency stop (active LOW with pull-up)
  pinMode(PIN_EMERGENCY_STOP, INPUT_PULLUP);
  
  // I2C voor MPU6050 (op alternatieve pinnen!)
  Wire1.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
  
  // Status LED
  pinMode(PIN_STATUS_LED, OUTPUT);
  digitalWrite(PIN_STATUS_LED, LOW);
  
  // ==================== MPU6050 INIT ====================
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection FAILED!");
  } else {
    Serial.println("MPU6050 connected OK");
    mpu.setFullScaleAccelRange(MPU6050_ACCEL_FS_2);
    mpu.setFullScaleGyroRange(MPU6050_GYRO_FS_250);
    mpu.setDLPFMode(MPU6050_DLPF_BW_20);  // 20Hz low-pass
  }
  
  // PID setup
  pid_L.SetMode(AUTOMATIC);
  pid_L.SetOutputLimits(-MOTOR_PWM_MAX, MOTOR_PWM_MAX);
  pid_L.SetSampleTime(1);  // 1ms
  
  pid_R.SetMode(AUTOMATIC);
  pid_R.SetOutputLimits(-MOTOR_PWM_MAX, MOTOR_PWM_MAX);
  pid_R.SetSampleTime(1);
  
  // ==================== WIFI AP ====================
  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL);
  Serial.printf("AP Started: %s (IP: %s)\n", WIFI_AP_SSID, WiFi.softAPIP().toString().c_str());
  
  // ==================== WEBSOCKET SERVER ====================
  ws.onEvent(ws_on_event);
  server.addHandler(&ws);
  
  // Health check endpoint
  server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "OK");
  });
  
  server.begin();
  Serial.println("HTTP + WS Server started");
  
  // ==================== START CORE 0 TASK ====================
  xTaskCreatePinnedToCore(
    core0_task,      // Task functie
    "Core0_RT",      // Naam
    8192,            // Stack size (bytes)
    NULL,            // Parameters
    24,              // Priority (hoge prioriteit voor real-time)
    NULL,            // Task handle
    TASK_CORE_0      // Core 0
  );
  
  Serial.println("Setup complete. Entering main loop (Core 1).");
}

void loop() {
  // Core 1: WiFi, WebSocket, Telemetrie, OTA
  
  // Telemetrie sturen (50Hz = elke 20ms)
  if (millis() - last_telemetry_ms >= 20) {
    last_telemetry_ms = millis();
    send_telemetry();
  }
  
  // WebSocket cleanup (periodiek)
  static uint32_t ws_cleanup_ms = 0;
  if (millis() - ws_cleanup_ms > 30000) {
    ws_cleanup_ms = millis();
    ws.cleanupClients();
  }
  
  // Heartbeat LED
  static uint32_t led_ms = 0;
  if (millis() - led_ms > 500) {
    led_ms = millis();
    digitalWrite(PIN_STATUS_LED, ws_connected ? HIGH : (millis() / 500) % 2);
  }
  
  // Yield voor WiFi stack
  vTaskDelay(pdMS_TO_TICKS(1));
}