# Rover → Drone Project (Familie Project)

**Status:** OTRONIC besteld · Romi-aandrijflijn besteld · Firmware klaar · **wacht op levering**

---

## 📦 Hardware Overzicht

| Component | Leverancier | Status |
|-----------|-------------|--------|
| ESP32 DevKitC V1 (USB-C) | OTRONIC | ✅ Besteld |
| TB6612FNG Motor Driver | OTRONIC | ✅ Besteld |
| HC-SR04 Ultrasonic (2x) | OTRONIC | ✅ Besteld |
| MPU-6050 IMU | OTRONIC | ✅ Besteld |
| TCRT5000 1-ch Module (4x) | OTRONIC | ✅ Besteld |
| 2×18650 Houder + Schakelaar | OTRONIC | ✅ Besteld |
| Samsung 18650 3400mAh (2x) | OTRONIC | ✅ Besteld |
| XL4015 Buck Converter | OTRONIC | ✅ Besteld |
| Mini Breadboard 170 | OTRONIC | ✅ Besteld |
| 40-pin Headers (2x) | OTRONIC | ✅ Besteld |
| M3 200-set Standoff/Screw/Nut | OTRONIC | ✅ Besteld |
| Mini sleutel M2.5/M3/M4 | OTRONIC | ✅ Besteld |
| ~~2WD Chassis (DollaTek)~~ | ~~Amazon~~ | ↩️ **Retour** (encoderprintjes ontbraken) |
| Pololu Romi Chassis Kit - Black | Opencircuit | ✅ Besteld |
| Pololu Magnetic Encoder Paar (12 CPR) | Opencircuit | ✅ Besteld |

**Totaal:** ~€132 bruto (~€112 netto na retour DollaTek)

---

## 🚀 Quick Start (wanneer hardware binnen is)

### 1. Firmware uploaden
```bash
cd firmware
pio run --target upload
pio device monitor
```

### 2. Ground Station testen
```bash
cd groundstation
pip install -r requirements.txt
python3 ws_test.py --interactive
```

### 3. WiFi verbinden
- Telefoon/laptop/pi connecten met: **Rover_AP** / **rover123**
- IP van rover: **192.168.4.1**

---

## 📁 Project Structuur

```
rover-drone/
├── firmware/                 # ESP32 PlatformIO project
│   ├── platformio.ini       # Build config
│   └── src/
│       ├── main.cpp         # Hoofdprogramma (Core 0 + Core 1)
│       └── pinout.h         # Alle pin definities
├── groundstation/           # Raspberry Pi / PC software
│   ├── requirements.txt     # Python dependencies
│   └── ws_test.py          # WebSocket test client
├── docs/
│   ├── wiring.md           # Complete wiring diagram + pinout
│   └── pinout_reference.md # Printbare A6 referentiekaart
└── cad/                     # 3D print files (later)
```

> **Presentaties** (Marp) staan **buiten deze repo** in `../slides/` en worden niet
> mee-versioneerd.

---

## 🔌 Belangrijkste Pinout (samenvatting - Option B: Encoders verplaatst)

| Functie | GPIO | Type | Let op |
|---------|------|------|--------|
| Motor STBY | 4 | Output | |
| Motor AIN1/AIN2 | 5, 18 | Output | |
| Motor PWMA | 19 | PWM (LEDc Ch 0) | 20kHz, 10-bit, max 70% duty |
| Motor BIN1/BIN2 | 21, 22 | Output | |
| Motor PWMB | 23 | PWM (LEDc Ch 1) | 20kHz, 10-bit, max 70% duty |
| Encoder LA (L-A) | 14 | Input, interrupt | Kanaal A voor snelheid, **via 1k/2k deler** |
| Encoder LB (L-B) | 25 | Input, interrupt | Richting (niet gebruikt voor snelheid) |
| Encoder RA (R-A) | 26 | Input, interrupt | Kanaal A voor snelheid, **via 1k/2k deler** |
| Encoder RB (R-B) | 27 | Input, interrupt | Richting (niet gebruikt voor snelheid) |
| US Front Trig/Echo | 13, 12 | Out/In | Echo: **1k/2k spanningsdeler!** |
| US Rear Trig/Echo | 2, 15 | Out/In | Echo: **1k/2k spanningsdeler!** |
| MPU6050 SDA/SCL | 16, 17 | I2C (Wire1) | Niet Wire0 (conflict motor) |
| Line L2 (linksbuiten) | 36 | ADC1_CH0 | Werkt met WiFi |
| Line L1 (linksbinnen) | 39 | ADC1_CH3 | Werkt met WiFi |
| Line R1 (rechtsbinnen) | 34 | ADC1_CH6 | Werkt met WiFi |
| Line R2 (rechtsbuiten) | 35 | ADC1_CH7 | Werkt met WiFi |
| Battery ADC | 33 | ADC1_CH5 | Werkt met WiFi, divider 21:1 |
| Status LED | 2 | Output | Onboard |
| Buzzer | 32 | Output, PWM | Geen boot conflict |
| Emergency Stop | 3 | Input, pull-up | Active LOW, GPIO3 (RX0) |

---

## ⚡ Belangrijke Hardware Gotchas

1. **Buck converter EERST instellen op 5.0V** (met multimeter) **VOOR** ESP32 aansluiten!
2. **HC-SR04 Echo pins** MOETEN via 1k/2k spanningsdeler (5V → 3.3V)
3. **MPU6050 op Wire1** (GPIO 16/17), niet Wire0 (conflict motor driver BIN1/BIN2 op 21/22)
4. **Encoders op GPIO 14, 25, 26, 27** - alleen kanaal A (14, 26) voor snelheid, B (25, 27) voor richting. **Alle kanalen via 1k/2k deler** (encoderprintje draait op 5V)
5. **Motoren max 70% duty** (`MOTOR_MAX_DUTY` 716) - 2S vol is 8,4V maar de Romi-motoren zijn 3-6V. Nooit verhogen.
6. **Alle line sensors + battery op ADC1** (GPIO 33, 34, 35, 36, 39) - **geen ADC2 workaround meer nodig!**
7. **Alle GND** verbinden (sterpuntsbreiding op perboard)
8. **TB6612FNG STBY** (GPIO 4) moet HIGH voor motoren werken
9. **Buzzer op GPIO 32** (niet GPIO 0 - boot strapping conflict)
10. **Emergency stop op GPIO 3** (niet GPIO 1 - TX0 serial conflict)
11. **Romi extended backshaft**: motorasje moet achteraan uitsteken, anders past de encodermagneet niet - direct bij uitpakken checken
12. **TCRT5000 onder de bodemplaat** (Romi-bodemvrijheid ~10mm), 3-5mm boven de grond

---

## 🧪 Test Volgorde

| Stap | Wat | Hoe |
|------|-----|-----|
| 1 | Headers solderen | TB6612, MPU6050, 4x TCRT, 2x HC-SR04 |
| 2 | Buck converter | Instellen 5.0V, meten met multimeter |
| 3 | Breadboard opbouw | Voeding, spanningsdelers, signalen |
| 4 | Chassis montage | Romi: motoren in clips, wielen, caster, zelfgemaakt sensordek |
| 5 | Encoders monteren | Magneet op achteras, printje solderen (VCC/GND/A/B) |
| 6 | **Encoderverificatie** | 10 wielomwentelingen met de hand = **28.800 ticks** |
| 7 | Elektronica mounten | M3 set + nylon spacers |
| 8 | Firmware upload | `pio run -t upload` |
| 9 | Serial monitor | Check boot logs, WiFi AP start |
| 10 | WiFi connect | Tel/PC → Rover_AP / rover123 |
| 11 | WS test | `python ws_test.py --interactive` |
| 12 | Eerste rijtest | WASD besturing |

---

## 📡 Communicatie Protocol

**Pi → Rover (10-50Hz):**
```json
{"T":100,"S":0,"M":0}
```
- T: Throttle -100..100
- S: Steer -100..100  
- M: Mode 0=Manual, 1=Auto

**Rover → Pi (50-100Hz):**
```json
{"t":12345,"dF":45,"dR":120,"bat":11.3,"imu":{"r":1,"p":-2},"enc":{"L":450,"R":448}}
```
- t: timestamp ms
- dF/dR: afstand front/rear cm
- bat: batterij voltage
- imu: roll/pitch/yaw graden
- enc: encoder tellers
- line: lijnpositie -1.0..+1.0
- mode/estop: status flags

---

## 🛠 Volgende Stappen (Sprint 1-4)

- [ ] **Sprint 1:** Fundament - Rover rijdt met joystick
- [ ] **Sprint 2:** Zintuigen & Veiligheid - Sensoren, noodstop, telemetrie
- [ ] **Sprint 3:** Autonoom - PID, lijnvolgen, obstakelvermijden
- [ ] **Sprint 4:** Pi Intelligence - Camera, CV, Web Dashboard, Mission Planner

---

## 📚 Documentatie

- `docs/wiring.md` - Complete schakeling, pinout, testpunten
- `firmware/src/pinout.h` - Alle GPIO definities
- `firmware/src/main.cpp` - Hoofdprogramma met Core 0/1 taken
- `groundstation/ws_test.py` - Test client met joystick support

---

## 🆘 Troubleshooting

| Probleem | Oorzaak | Oplossing |
|----------|---------|-----------|
| ESP32 boot niet | GPIO 0/2/12/15 hoog tijdens boot | Check strapping pins, spanningsdelers (buzzer nu op GPIO 32) |
| Geen WiFi AP | Voeding instabiel | Buck converter checken, 5V meten |
| Motor draait niet | STBY laag, PWM 0, VM geen spanning | `digitalWrite(STBY,HIGH)`, buck→VM check |
| Encoder telt niet / dubbel | Pull-up ontbreekt, 1k/2k deler vergeten, beide kanalen geteld | `INPUT_PULLUP`, deler op 14/26, alleen kanaal A tellen |
| Encoder telt half/onregelmatig | Magneet niet goed op de as, luchtspleet te groot | Magneet opnieuw persen, printje dichter bij de as |
| Snelheid/topsnelheid te laag | `MOTOR_MAX_DUTY` of lege accu | 70% clamp is bewust; accu laden of 6V-rail overwegen |
| MPU6050 niet gevonden | I²C adres, verkeerde Wire | `Wire1.begin(16,17)`, I2C scanner draaien |
| HC-SR04 meet niet | Echo pin 5V op ESP32 | **Spanningsdeler 1k/2k!** |
| Bat 0V | Divider 21:1? GPIO 33 (ADC1) | Check spanningsdeler, ADC pin 33 |
| WiFi instabiel / disconnect | ADC2 lezing tijdens WiFi | **Opgelost:** alle sensors nu op ADC1 |
| Noodstop werkt niet | Verkeerde pin (was GPIO 1) | Check GPIO 3 (pull-up, active LOW) |

---

**Veel bouwplezier! 🚗💨**  
Vragen? Check `docs/wiring.md` of serial monitor output.