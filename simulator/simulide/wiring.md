# SimulIDE Wiring Guide - Rover Project

## 🎯 Quick Start Wiring

### Step 1: Place Components
Drag these onto the canvas (approximate positions):
```
[ESP32]          [TB6612FNG]    [Motor L] [Motor R]   [Enc L] [Enc R]
  (100,100)        (300,100)      (500,50)  (500,200)   (650,50) (650,200)

[HC-SR04 F]   [HC-SR04 R]   [MPU6050]   [Line x4]    [Battery] [Button] [Buzzer]
  (300,300)     (300,400)     (100,300)   (500,300)    (100,450) (300,450) (500,450)
```

### Step 2: Wire Power (Do First!)
```
# Main Power Rails
ESP32:5V      → TB6612FNG:VCC
ESP32:3V3     → MPU6050:VCC, HC-SR04:VCC (x2), Line Sensors:VCC (x4), Encoders:VCC (x2)
ESP32:GND     → All GNDs (star ground)
Battery:VCC   → TB6612FNG:VM, Buck Converter:IN
Battery:GND   → All GNDs
Buck:OUT (5V) → ESP32:5V, TB6612FNG:VCC (logic), HC-SR04:VCC
```

### Step 3: Motor Driver Signals
| ESP32 GPIO | TB6612FNG Pin | Wire Color |
|------------|---------------|------------|
| D4         | STBY          | Green      |
| D5         | AIN1          | Blue       |
| D18        | AIN2          | Blue       |
| D19        | PWMA          | Yellow (PWM) |
| D21        | BIN1          | Blue       |
| D22        | BIN2          | Blue       |
| D23        | PWMB          | Yellow (PWM) |

### Step 4: Motors & Encoders
```
TB6612FNG:AO1 → Motor L:+
TB6612FNG:AO2 → Motor L:-
TB6612FNG:BO1 → Motor R:+
TB6612FNG:BO2 → Motor R:-

Encoder L:A → ESP32:D14
Encoder L:B → ESP32:D25
Encoder R:A → ESP32:D26
Encoder R:B → ESP32:D27

Encoder L:VCC → ESP32:3V3, GND → GND
Encoder R:VCC → ESP32:3V3, GND → GND

# Connect encoder to motor shaft (SimulIDE: drag encoder onto motor)
```

### Step 5: Ultrasonic Sensors
| Sensor | ESP32 GPIO | Pins |
|--------|------------|------|
| Front Trig | D13 | HC-SR04 Front:TRIG |
| Front Echo | D12 | HC-SR04 Front:ECHO |
| Rear Trig  | D2  | HC-SR04 Rear:TRIG |
| Rear Echo  | D15 | HC-SR04 Rear:ECHO |

Both HC-SR04: VCC→3V3, GND→GND

### Step 6: MPU6050 (I2C)
| ESP32 GPIO | MPU6050 Pin |
|------------|-------------|
| D16        | SDA         |
| D17        | SCL         |
| 3V3        | VCC         |
| GND        | GND         |

**Add 4.7kΩ pull-up resistors** on SDA/SCL to 3V3

### Step 7: Line Sensors (4x IR Reflectance)
| Sensor | ESP32 GPIO | ADC Channel |
|--------|------------|-------------|
| L2 (outer left) | D36 | ADC1_CH0 |
| L1 (inner left) | D39 (VN) | ADC1_CH3 |
| R1 (inner right) | D34 | ADC1_CH6 |
| R2 (outer right) | D35 | ADC1_CH7 |

All: VCC→3V3, GND→GND, OUT→respective GPIO

### Step 8: Battery Monitor
```
Battery + → [210kΩ] → [10kΩ] → GND
                    │
                    └──→ ESP32:D33 (ADC1_CH5)

Ratio: 21:1, Max 8.4V → 0.4V (safe for ESP32 ADC)
```

### Step 9: Peripherals
| Component | ESP32 GPIO | Notes |
|-----------|------------|-------|
| Buzzer | D32 | PWM capable |
| Emergency Stop | D3 | Pushbutton, pull-up, active LOW |

### Step 10: Verify Connections
- [ ] All GNDs connected (star ground)
- [ ] No floating pins
- [ ] Motor driver STBY wired to D4
- [ ] Encoder A pins on D14, D26 (speed counting)
- [ ] Line sensors on ADC1 pins (36, 39, 34, 35)
- [ ] Battery ADC on D33 (ADC1)
- [ ] I2C pull-ups on D16/D17
- [ ] Emergency stop on D3 with pull-up

## 🔧 SimulIDE Specific Tips

### ESP32 Firmware Loading
1. Right-click ESP32 → Properties
2. Firmware: `../../firmware/.pio/build/esp32dev/firmware.elf`
3. Clock: 240 MHz

### Motor-Encoder Coupling
- In SimulIDE, drag encoder onto motor to couple them
- Encoder OUT connects to motor ENC pin internally

### I2C Pull-ups
- Add 4.7kΩ resistors: SDA→3V3, SCL→3V3

### Simulation Instruments
- **Oscilloscope**: PWM pins 19, 23 (motor), 32 (buzzer)
- **Logic Analyzer**: Encoder pins 14, 26; I2C pins 16, 17
- **Serial Monitor**: 115200 baud, Tools → Serial Monitor

### Setting Sensor Values (During Simulation)
- **HC-SR04**: Right-click → Properties → Set distance
- **MPU6050**: Properties → Set orientation/accel
- **Line Sensors**: Properties → Reflectance % (0-100)
- **Battery**: Properties → Voltage (7.4V → 6.0V for low-batt test)

## ✅ Pre-Simulation Checklist
- [ ] Firmware.elf loaded on ESP32
- [ ] All GNDs common
- [ ] Battery 7.4V connected to TB6612FNG VM
- [ ] Buck converter 5V to ESP32 5V pin
- [ ] 3V3 to all sensor VCCs
- [ ] Motor driver STBY to D4
- [ ] Encoder A pins to D14, D26
- [ ] Line sensors to D36, D39, D34, D35
- [ ] Battery divider to D33
- [ ] I2C pull-ups on D16, D17
- [ ] Emergency stop on D3 with pull-up

## 🚨 Common Issues
| Symptom | Cause | Fix |
|---------|-------|-----|
| ESP32 not booting | Wrong firmware path | Check .elf path |
| Motors not spinning | STBY LOW | D4 must be HIGH |
| No encoder pulses | Not coupled | Drag encoder onto motor |
| I2C timeout | No pull-ups | Add 4.7kΩ on SDA/SCL |
| ADC reads 0 | Wrong pin | Check ADC1 pins (36,39,34,35,33) |
| Battery reads 0 | Divider wrong | 210k/10k = 21:1 ratio |

## 📐 Pinout Quick Reference Card (Printable)

```
┌────────────────────────────────────────────────────────────┐
│  ESP32 DevKitC V1 - Rover Pinout (Option B)                │
├────────────────────────────────────────────────────────────┤
│  3V3  EN  VP  VN  34  35  36  39  25  26  27  14  12  13  │
│  GND  23  22  21  19  18  5   4   0   2   15  13  16  17  │
├────────────────────────────────────────────────────────────┤
│  Motor:  STBY=4  AIN1=5  AIN2=18  PWMA=19                  │
│          BIN1=21 BIN2=22 PWMB=23                            │
│  Enc:    LA=14  LB=25  RA=26  RB=27                         │
│  US:     F_Trig=13 F_Echo=12  R_Trig=2  R_Echo=15          │
│  I2C:    SDA=16  SCL=17  (MPU6050 @ 0x68)                   │
│  Line:   L2=36  L1=39  R1=34  R2=35  (all ADC1)            │
│  Bat:    ADC=33  (21:1 divider)                             │
│  Peri:   Buzzer=32  Estop=3  (pull-up, active LOW)         │
└────────────────────────────────────────────────────────────┘
```
