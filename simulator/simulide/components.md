# SimulIDE Component Reference for Rover Project

## 📦 Core Components

### ESP32 DevKitC V1
- **Menu**: Microcontroller → ESP32
- **Properties**: 
  - Firmware: `../../firmware/.pio/build/esp32dev/firmware.elf`
  - Clock: 240 MHz
  - Flash: 4MB
- **Pins**: 38 pins (match DevKitC V1 pinout)

### TB6612FNG Motor Driver
- **Menu**: IC → Motor Driver → TB6612FNG
- **Pins**: STBY, AIN1, AIN2, PWMA, BIN1, BIN2, PWMB, AO1, AO2, BO1, BO2, VM, VCC, GND
- **Notes**: Dual H-bridge, 1.2A continuous per channel

### DC Motor
- **Menu**: Electromechanical → DC Motor
- **Pins**: +, - (or 1, 2)
- **Properties**: Can connect encoder for feedback

### Rotary Encoder
- **Menu**: Sensors → Rotary Encoder
- **Pins**: A, B, GND, VCC, OUT (quadrature output)
- **Properties**: Pulses per revolution: 20

### HC-SR04 Ultrasonic
- **Menu**: Sensors → Ultrasonic → HC-SR04
- **Pins**: VCC, GND, TRIG, ECHO
- **Properties**: Set distance in simulation

### MPU6050 IMU
- **Menu**: Sensors → IMU → MPU6050
- **Pins**: VCC, GND, SDA, SCL, AD0, INT
- **I2C Address**: 0x68 (AD0=GND) or 0x69 (AD0=VCC)

### IR Reflectance Sensor (Line Follower)
- **Menu**: Sensors → IR Reflectance
- **Pins**: VCC, GND, OUT
- **Properties**: Reflectance level (0-100%)

### Voltage Source (Battery)
- **Menu**: Sources → Battery
- **Properties**: Voltage: 7.4V (2S Li-ion)

### DC-DC Buck Converter
- **Menu**: Power → DC-DC Converter
- **Properties**: Output: 5V

### Resistors (Voltage Divider)
- **Menu**: Passive → Resistor
- **Values**: 210kΩ (2×100k + 10k), 10kΩ
- **Ratio**: 21:1 (8.4V → 0.4V)

### Pushbutton
- **Menu**: Input → Pushbutton
- **Use**: Emergency stop (active LOW with pull-up)

### Buzzer
- **Menu**: Output → Buzzer
- **Pin**: ESP32 GPIO 32

## 🔌 ESP32 Pinout Reference (DevKitC V1)

```
        USB-C
         │
    ┌────┴────┐
    │  ESP32  │
    └────┬────┘
         │
Left Side (J1)          Right Side (J2)
─────────────────       ─────────────────
3V3      1  2   GND     3V3     1  2   5V
EN       3  4   VP/VN   GND     3  4   17
34       5  6   35      16      5  6   15
36       7  8   39      13      7  8   12
25       9  10  26      14      9  10  4
27       11 12  14      0       11 12  2
GND      13 14  5V                 13 14  GND
```

### Option B GPIO Mapping
| Function | GPIO | Side |
|----------|------|------|
| Motor STBY | 4 | Right |
| Motor AIN1 | 5 | Right |
| Motor AIN2 | 18 | Left |
| Motor PWMA | 19 | Left |
| Motor BIN1 | 21 | Left |
| Motor BIN2 | 22 | Left |
| Motor PWMB | 23 | Left |
| Encoder LA | 14 | Right |
| Encoder LB | 25 | Left |
| Encoder RA | 26 | Left |
| Encoder RB | 27 | Left |
| US Front Trig | 13 | Right |
| US Front Echo | 12 | Right |
| US Rear Trig | 2 | Right |
| US Rear Echo | 15 | Right |
| I2C SDA | 16 | Right |
| I2C SCL | 17 | Right |
| Line L2 | 36 | Left |
| Line L1 | 39 (VN) | Left |
| Line R1 | 34 | Left |
| Line R2 | 35 | Left |
| Battery ADC | 33 | Left |
| Buzzer | 32 | Left |
| Emergency Stop | 3 | Right |

## ⚡ Power Distribution

```
┌─────────────────────────────────────────────────────────────┐
│                    2S Li-ion (7.4V)                         │
└────────────────────────────┬────────────────────────────────┘
                             │
              ┌──────────────┼──────────────┐
              ▼              ▼              ▼
        ┌──────────┐   ┌──────────┐   ┌──────────────┐
        │ TB6612FNG│   │ XL4015   │   │ Voltage      │
        │ VM       │   │ Buck IN  │   │ Divider      │
        │ (Motor)  │   │ (5V Out) │   │ (210k/10k)   │
        └──────────┘   └────┬─────┘   └──────┬───────┘
                            │                 │
                    ┌───────┼───────┐         │
                    ▼       ▼       ▼         ▼
               ┌──────┐ ┌──────┐ ┌──────┐ ┌────────┐
               │ESP32 │ │TB6612│ │HC-SR04│ │Line    │
               │5V Pin│ │VCC   │ │VCC x2│ │Sensors │
               └──────┘ └──────┘ └──────┘ └────────┘
                    │       │       │       │
                    └───────┴───────┴───────┘
                                 │
                                GND
                    (ALL GNDs CONNECTED - STAR GROUND)
```

## 📝 Notes for SimulIDE

1. **ESP32 Firmware**: Load `.elf` file, not `.bin`
2. **Motor Encoder**: Connect encoder OUT to motor ENC pin in SimulIDE
3. **I2C Pull-ups**: Add 4.7kΩ resistors on SDA/SCL to 3V3
4. **HC-SR04 Echo**: SimulIDE handles 5V→3.3V internally
5. **Voltage Divider**: 210kΩ (2×100k + 10k) / 10kΩ = 21:1 ratio
6. **Simulation Speed**: Start at 1x for debugging, increase for motor tests
6. **Logic Analyzer**: Use for PWM (pins 19, 23), encoders (14, 26), I2C (16, 17)
