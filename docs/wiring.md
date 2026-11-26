# Rover Wiring Diagram & Pinout Reference

## 📌 Overzicht Alle Verbindingen

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         ESP32 DevKitC V1 (USB-C)                            │
│                        (38-pin, WROOM-32, 3.3V logic)                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 🔌 Pinout Tabel (Definitief - Option B: Encoders verplaatst voor ADC1 vrij)

| Functie | Component | ESP32 GPIO | Type | Opmerking |
|---------|-----------|------------|------|-----------|
| **Motor Driver (TB6612FNG)** | | | | |
| STBY | Motor driver | **GPIO 4** | Output | HIGH = enabled |
| AIN1 | Motor driver | **GPIO 5** | Output | Richting Linker motor |
| AIN2 | Motor driver | **GPIO 18** | Output | Richting Linker motor |
| PWMA | Motor driver | **GPIO 19** | PWM (LEDc Ch 0) | 20kHz PWM |
| BIN1 | Motor driver | **GPIO 21** | Output | Richting Rechter motor |
| BIN2 | Motor driver | **GPIO 22** | Output | Richting Rechter motor |
| PWMB | Motor driver | **GPIO 23** | PWM (LEDc Ch 1) | 20kHz PWM |
| **Encoders (Pololu magnetisch, 12 CPR, 120:1)** | | | | |
| Enc LA | Linker encoder A | **GPIO 14** | Input, interrupt | Kanaal A - snelheid, **via 1k/2k deler** |
| Enc LB | Linker encoder B | **GPIO 25** | Input, interrupt | Kanaal B - richting (niet gebruikt) |
| Enc RA | Rechter encoder A | **GPIO 26** | Input, interrupt | Kanaal A - snelheid, **via 1k/2k deler** |
| Enc RB | Rechter encoder B | **GPIO 27** | Input, interrupt | Kanaal B - richting (niet gebruikt) |
| **Ultrasonic (HC-SR04)** | | | | |
| US Front Trig | Voor sensor | **GPIO 13** | Output | |
| US Front Echo | Voor sensor | **GPIO 12** | Input | **met 1k/2k spanningsdeler!** |
| US Rear Trig | Achter sensor | **GPIO 2** | Output | |
| US Rear Echo | Achter sensor | **GPIO 15** | Input | **met 1k/2k spanningsdeler!** |
| **MPU6050 (I²C via Wire1)** | | | | |
| SDA | IMU | **GPIO 16** | I2C SDA | Wire1 (4.7k pull-up) |
| SCL | IMU | **GPIO 17** | I2C SCL | Wire1 (4.7k pull-up) |
| VCC | IMU | 3.3V | | |
| GND | IMU | GND | | |
| **TCRT5000 Line Sensors (4x) - ALLE OP ADC1** | | | | |
| Line L2 (linksbuiten) | ADC1 | **GPIO 36** | ADC1_CH0 | Werkt met WiFi |
| Line L1 (linksbinnen) | ADC1 | **GPIO 39** | ADC1_CH3 | Werkt met WiFi |
| Line R1 (rechtsbinnen) | ADC1 | **GPIO 34** | ADC1_CH6 | Werkt met WiFi |
| Line R2 (rechtsbuiten) | ADC1 | **GPIO 35** | ADC1_CH7 | Werkt met WiFi |
| **Battery Monitor** | | | | |
| Bat Voltage | 2S 18650 via divider | **GPIO 33** | ADC1_CH5 | **ADC1 - werkt met WiFi!** |
| **Status & Control** | | | | |
| Status LED | Onboard | **GPIO 2** | Output | Active HIGH |
| Emergency Stop | Noodstop knop | **GPIO 3** | Input pull-up | Active LOW, was RX0 |
| Buzzer | Piezo | **GPIO 32** | Output, PWM | Geen boot conflict |
| **I²C (Wire0 - standaard)** | Vrij voor toekomst | **GPIO 21, 22** | | Wordt NIET gebruikt (conflict motor) |

---

## ⚡ Voedingschema

```
                    ┌─────────────────────┐
                    │   2x 18650 (2S)     │
                    │   7.4V nominal      │
                    │   8.4V max          │
                    └──────────┬──────────┘
                               │
                    ┌──────────▼──────────┐
                    │ 2S Battery Holder   │
                    │   met schakelaar    │
                    └──────────┬──────────┘
                               │
              ┌────────────────┼────────────────┐
              ▼                ▼                ▼
       ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
       │  TB6612FNG  │ │ XL4015 Buck │ │ Spannings-  │
       │  VM (motor) │ │  Converter  │ │ deler (bat) │
       │  7.4V direct│ │  IN: 7.4V   │ │  210k/10k   │
       └──────┬──────┘ │  OUT: 5.0V  │ └──────┬──────┘
              │        └──────┬──────┘        │
              ▼               ▼               ▼
       ┌─────────────┐ ┌─────────────┐ ┌─────────────┐
       │ 2x Romi     │ │  ESP32      │ │  ESP32 ADC  │
       │ 120:1 motor │ │  5V pin     │ │  GPIO33     │
       │ (via TB6612)│ │             │ │             │
       └─────────────┘ └─────────────┘ └─────────────┘
              │               │               │
              └───────────────┼───────────────┘
                              ▼
                       ┌─────────────┐
                       │   GND COMMON │
                       │  (alle massa's│
                       │   verbonden)  │
                       └─────────────┘
```

---

## ⚡ Motorspanning: 2S (8,4 V) vs motor (3–6 V)

De TB6612FNG geeft `VM` **1:1** door aan de motoren. Volle 2S = 8,4 V, maar de
Romi 120:1 motoren zijn 3–6 V. Daarom begrenst de firmware de duty cycle:

| | Waarde |
|---|---|
| PWM-resolutie | 10-bit (0–1023) |
| `MOTOR_MAX_DUTY` | **716** (70 %) |
| Motorspanning bij volle accu | 8,4 × 0,70 = **5,9 V** |
| Motorspanning bij 6,6 V (leeg) | 4,6 V |
| `MAX_SPEED_MM_S` | 350 mm/s |

Gevolg: de topsnelheid zakt mee met de accu. De PID compenseert dat tot het
setpoint niet meer haalbaar is.

**Niet doen:** `MOTOR_MAX_DUTY` op 1023 zetten "voor meer snelheid". Bij 8,4 V
staat er dan 40 % te veel op de motor; borstels en gearbox slijten snel en de
motor wordt heet.

**Nette oplossing later:** een aparte 6 V-rail voor `VM` (buck of DC-DC),
buiten het huidige budget.

---

## 🔧 TB6612FNG Breakout Pinout

```
TB6612FNG Breakout (top view, pins downward)

  ┌─────────────────────────────────────┐
  │  VM    VCC   GND   STBY  AIN1  AIN2 │  ← Motor Power, Logic, Standby, In A
  │  PWMA  BIN1  BIN2  PWMB  GND   VCC  │  ← PWM A, In B, PWM B, GND, Logic
  │  AO1   AO2   BO1   BO2              │  ← Motor outputs
  └─────────────────────────────────────┘

Onze wiring:
  VM     → 2S Battery (7.4V) direct
  VCC    → 3.3V (van ESP32 of buck 5V via 3.3V regulator)
  GND    → Common GND
  STBY   → GPIO 4
  AIN1   → GPIO 5
  AIN2   → GPIO 18
  PWMA   → GPIO 19 (LEDc Ch 0)
  BIN1   → GPIO 21
  BIN2   → GPIO 22
  PWMB   → GPIO 23 (LEDc Ch 1)
  AO1/AO2 → Linker motor (rood/zwart)
  BO1/BO2 → Rechter motor (rood/zwart)
```

---

## 📏 HC-SR04 Spanningsdeler (NOODZAKELIJK!)

**HC-SR04 werkt op 5V, Echo pin geeft 5V uit. ESP32 GPIO is 3.3V max!**

```
HC-SR04 Echo pin (5V) ──┬──▶ ESP32 GPIO (3.3V max)
                        │
                       [1kΩ]
                        │
                       [2kΩ]
                        │
                       GND

Berekening: Vout = 5V × (2k / (1k+2k)) = 3.33V ✓ Veilig!

Doe dit voor BEIDE sensoren:
  - Front Echo (GPIO 12) → 1k/2k divider
  - Rear Echo  (GPIO 15) → 1k/2k divider
```

---

## 🔋 Batterij Spanningsdeler

```
2S 18650 (max 8.4V) ──┬──▶ ESP32 ADC (max 3.3V)
                      │
                     [100kΩ]
                      │
                     [100kΩ]
                      │
                     [10kΩ]
                      │
                     GND

Totaal: 210kΩ boven, 10kΩ onder
Ratio: 21:1
Vout_max = 8.4V / 21 = 0.4V ✓ Veilig voor ESP32 ADC (0-3.3V)

Meet op GPIO 33 (ADC1_CH5, input only)
✅ ADC1 - werkt NORMAAL met actieve WiFi, geen workaround nodig!
```

---

## 🎯 Encoder Wiring (Pololu magnetisch, 12 CPR) — **Nieuwe pinnen (Option B)**

```
Romi 120:1 motor (extended backshaft):
  Vooras   → Wiel
  Achteras → magneetje (opgeperst op de as)
                ↑
          encoderprintje (hall-sensor)

Encoderprintje (VCC / GND / A / B):
  VCC → 5V (buck)                      (printje mag 2,7-18V)
  GND → common GND
  A   → 1k/2k deler → GPIO 14 (links) / GPIO 26 (rechts)
  B   → 1k/2k deler → GPIO 25 (links) / GPIO 27 (rechts)  [niet gebruikt]

WAAROM DE DELER:
  Het printje wordt op 5V gevoed en zet zijn uitgang dus ook op 5V.
  ESP32-GPIO's zijn NIET 5V-tolerant. Zelfde deler als bij de HC-SR04:

  5V ──[1kΩ]──┬──[2kΩ]── GND
              │
              └──▶ ESP32 GPIO (3,33V)

Alleen kanaal A (GPIO 14, 26) wordt geteld voor snelheid, met een
CHANGE-interrupt: beide flanken -> 2x resolutie.
Kanaal B (GPIO 25, 27) is optioneel voor richtingsdetectie in de toekomst.
```

### Resolutie

| | Waarde |
|---|---|
| Encoder | 12 CPR (pulsen per motoromwenteling) |
| Getelde flanken | 2 (opgaand + neergaand) |
| Gearbox | 120:1 |
| **Ticks per wielomwenteling** | **2880** (12 × 2 × 120) |
| Wielomtrek | π × 70 = 219,9 mm |
| **Resolutie** | **13,10 ticks/mm** |
| Verificatie | 10 wielomwentelingen met de hand = **28.800 ticks** |

---

## 📐 TCRT5000 4-Kanaals Array Montage

```
Onderkant chassis (kijk van onderen):

    Vooruit rijden →
    
    ┌─────────────────────────────────────┐
    │  L2    L1    R1    R2               │  ← Sensor volgorde
    │  █     █     █     █                │
    │  │     │     │     │                │
    │  ▼     ▼     ▼     ▼                │
    │  └─────────────────────────────────┘
    │        Zwart lijn (15-20mm breed)   │
    └─────────────────────────────────────┘

GPIO mapping:
  L2 (Linksbuiten)  → GPIO 36
  L1 (Linksbinnen)  → GPIO 39
  R1 (Rechtsbinnen) → GPIO 34
  R2 (Rechtsbuiten) → GPIO 35

Afstand sensoren: ~15-20mm tussen middelpunten
Hoogte boven grond: 3-5mm
```

---

## 🔍 MPU6050 Oriëntatie

```
MPU6050 op chassis gemonteerd:

       Vooruit ↑
       
       Z-as ↑ (boven)
       │
       │   X-as → (rechts)
       │  /
       │ /
       └──→ Y-as (vooruit)

Montage: Plat op chassis, chip naar boven.
Let op: Geen magnetometer → yaw drift!
```

---

## 🧪 Testpunten Checklist (na solderen)

| Test | Verwacht | Werkwijze |
|------|----------|-----------|
| Buck converter OUT | 5.0V ±0.05V | Multimeter op buck OUT vs GND |
| ESP32 5V pin | 5.0V | Multimeter op ESP32 5V pin |
| ESP32 3.3V pin | 3.3V | Multimeter op ESP32 3.3V pin |
| TB6612FNG VCC | 3.3V | Multimeter op TB6612 VCC pin |
| HC-SR04 VCC | 5V | Multimeter op sensor VCC |
| HC-SR04 Echo (met divider) | <3.3V | Meten tijdens trigger |
| Batterij divider uitgang | <0.5V | Meten op **GPIO33** bij 8.4V bat |
| Encoder A (na 1k/2k deler) | 0 ↔ 3,3V wisselt | Meten op GPIO14/26, wiel langzaam met de hand draaien |
| Motor-duty bij T=100 | max 70 % (≈5,9V gemiddeld) | Firmware-clamp `MOTOR_MAX_DUTY` |
| Alle GND verbonden | 0Ω | Continuïteit metertje |
| Geen kortsluiting VCC-GND | ∞Ω | Meten voor voeding aansluiten |

---

## 📝 Soldervolgorde

1. **Headers op breakouts** (TB6612FNG, MPU6050, 4x TCRT5000, HC-SR04)
2. **Spanningsdelers** op perboard/breadboard (1k/2k voor US, 100k/100k/10k voor bat)
3. **Hoofdvoeding**: 2S houder → buck IN, 2S houder → TB6612 VM
4. **Buck OUT** → ESP32 5V pin, TB6612 VCC (via 3.3V reg), sensoren VCC
5. **Alle GND** samen (sterpuntsbreiding op perboard)
6. **Signaal draden** ESP32 → breakouts (check pinout tabel!)
7. **Motor draden** TB6612 AO1/AO2/BO1/BO2 → motoren (Romi: direct op de motorpennen)
8. **Encoder draden** encoderprintje A/B → 1k/2k deler → ESP32 GPIO 14/26 (A) en 25/27 (B)
9. **Visuele inspectie** + multimeter check
10. **Eerste power-on** met USB (geen batterij), check 3.3V/5V rails

---

## ⚠️ Waarschuwingen & Gotchas

1. **GPIO 0**: NIET meer gebruikt voor buzzer (verplaatst naar GPIO 32). Laat los tijdens boot!
2. **GPIO 1**: NIET meer gebruikt voor noodstop (verplaatst naar GPIO 3).
3. **GPIO 3 (Emergency stop)**: Is RX0. Werkt na boot met USB_CDC_ON_BOOT=1. Active LOW met pull-up.
4. **GPIO 12 (US Front Echo)**: Is strapping pin (MTDI). Werkt na boot, maar **moet laag zijn tijdens boot**! Spanningsdeler zorgt ervoor.
5. **GPIO 15 (US Rear Echo)**: Is strapping pin (MTDO). Moet laag zijn tijdens boot.
6. **GPIO 14, 25, 26, 27**: Encoder pinnen - input met pull-up. **Alle signalen via 1k/2k deler** (encoderprintje draait op 5V). GPIO 34-39 zijn de enige echt input-only pinnen.
7. **GPIO 32, 33, 34, 35, 36, 39**: ADC1 pinnen - **werken NORMAAL met WiFi** (geen ADC2 workaround meer!).
8. **I²C op Wire1 (GPIO 16/17)**: Wire0 (GPIO 21/22) conflict met motor driver!
9. **TB6612FNG STBY (GPIO 4)**: Moet HIGH voor motoren werken. Default LOW = motors vrij.
10. **Buck converter**: **EERST instellen op 5.0V** met multimeter VOOR ESP32 aansluiten!
11. **Motorspanning**: 2S vol = 8,4 V op motoren van 3-6 V. De firmware begrenst op 70 % duty; nooit `MOTOR_MAX_DUTY` verhogen zonder de motoren te vervangen.
12. **Extended backshaft**: het Romi-motorasje moet aan de achterkant uitsteken, anders past de encodermagneet niet. Direct bij uitpakken controleren.
13. **Romi bodemvrijheid ≈ 10 mm**: de TCRT5000's moeten **onder** de bodemplaat, 3-5 mm boven de grond. Niet op het dek monteren.
14. **HC-SR04 voor**: zit geen bracket op de Romi - voorzie een randje aan het zelfgemaakte dek of print er een.