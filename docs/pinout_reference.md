# ESP32 Rover - Pinout Reference Card
**Print deze pagina (A6 formaat) en houd bij de build**

```
┌────────────────────────────────────────────────────────────────────┐
│                    ESP32 DevKitC V1 (38-pin)                       │
│                         PINOUT OVERZICHT                           │
└────────────────────────────────────────────────────────────────────┘

  ┌── USB-C ──────────────────────────────────────────────────────┐
  │  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  │
  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  │ 3V3 VP  VN  34  35  36  39  25  26  27  14  12  13  GND 5V  │
  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │  │
  │ GND 23  22  21  19  18  5   4   0   2   15  2   4   16  17  │
  │  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  ○  │
  └────────────────────────────────────────────────────────────────┘

  LEGENDA:  ● = gebruikt   ○ = vrij   ⚠ = let op!

⚠ De ESP32 past NIET in een breadboard - verbind met jumperdraden.

════════════════════════════════════════════════════════════════════════
MOTOR DRIVER (TB6612FNG)                    ENCODERS (Pololu 12 CPR, 120:1)
════════════════════════════════════════════════════════════════════════
STBY    → GPIO 4        ●                   Enc LA   → GPIO 14  ● (via 1k/2k deler)
AIN1    → GPIO 5        ●                   Enc LB   → GPIO 25  ● (richting, optioneel)
AIN2    → GPIO 18       ●                   Enc RA   → GPIO 26  ● (via 1k/2k deler)
PWMA    → GPIO 19       ● (LEDc Ch 0)       Enc RB   → GPIO 27  ● (richting, optioneel)
BIN1    → GPIO 21       ●
BIN2    → GPIO 22       ●
PWMB    → GPIO 23       ● (LEDc Ch 1)

PWM: 20kHz, 10-bit. Clamp: MOTOR_MAX_DUTY 716 = 70% → 5,9V bij 8,4V accu
Alleen kanaal A (14, 26) voor snelheid, CHANGE-interrupt (2 flanken)
2880 ticks/wielomwenteling = 13,10 ticks/mm (70mm wiel, 120:1, 12 CPR)

═════════════════════════════════════════════════════════════════════════
ULTRASONIC (HC-SR04)                        MPU6050 (I²C - Wire1)
═════════════════════════════════════════════════════════════════════════
US Front Trig  → GPIO 13  ●                 SDA      → GPIO 16  ● (Wire1)
US Front Echo  → GPIO 12  ● ⚠              SCL      → GPIO 17  ● (Wire1)
US Rear Trig   → GPIO 2   ●                 ADDR     → 0x68
US Rear Echo   → GPIO 15  ● ⚠

⚠ ECHO PINS: 1k/2k spanningsdeler NOODZAKELIJK (5V→3.3V)!
  5V ───[1kΩ]───┬───[2kΩ]─── GND
                │
                └──▶ ESP32 GPIO (3.3V max)

═════════════════════════════════════════════════════════════════════════
TCRT5000 LINE SENSORS (4x)                  BATTERY MONITOR
═════════════════════════════════════════════════════════════════════════
Line L2 (linksbuiten)  → GPIO 36  ● ADC1   Bat Voltage → GPIO 33  ● ADC1
Line L1 (linksbinnen)  → GPIO 39  ● ADC1   Divider: 100k+100k+10k / 10k = 21:1
Line R1 (rechtsbinnen) → GPIO 34  ● ADC1   Max 8.4V → 0.4V (veilig)
Line R2 (rechtsbuiten) → GPIO 35  ● ADC1   Meet op ADC1_CH5 (GPIO33)

✅ ALLE OP ADC1 - Werkt NORMAAL met WiFi, GEEN workaround nodig!
✅ TCRT5000 op 3V3 (BESLISSING) - uitgang < 3,3V, geen delers nodig.
   GPIO 34-39 zijn input-only en NIET 5V-tolerant.
HC-SR04 moet 5V - Echo verplicht via 1k/2k deler.

═════════════════════════════════════════════════════════════════════════
STATUS & CONTROL
═════════════════════════════════════════════════════════════════════════
~~Status LED     → GPIO 2~~    ⚠ VERVALLEN (GPIO2 = US Rear Trig)
Emergency Stop  → GPIO 3    ● (input pull-up, active LOW, was RX0)
Buzzer          → GPIO 32   ● (PWM capable, geen boot conflict)

═════════════════════════════════════════════════════════════════════════
VOEDING
═════════════════════════════════════════════════════════════════════════
2S 18650 pack (⚠ ONBESCHERMD) ──▶ 2S BMS (OT3681) ──▶ schakelaar
   houder1 + ──────────────┐                          │
   houder1 - ──┐           │  BM = middenpunt         │
   houder2 + ──┘           │                          │
   houder2 - ──────────────┴── P-                     │
                                                      ▼
   ── TB6612FNG VM (direct, max 70% duty = 5,9V)
   ── XL4015 Buck IN ──▶ 5.0V OUT ──▶ ESP32 5V pin
                  │
                  ├─▶ TB6612FNG VCC (via 3.3V reg)
                  ├─▶ HC-SR04 VCC (5V)
                  ├─▶ Encoderprintjes VCC (5V)
                  └─▶ Sensoren VCC
   ── Spanningsdeler (21:1) ──▶ GPIO 33 (ADC1)

ALLE GND VERBINDEN (sterpuntsbreiding op perboard)!

Buck XL4015: schroefklemmen IN+/IN-/OUT+/OUT-, geen EN, min 1.25V
              OUT- en IN- zijn beide GND (onderling verbonden)
Accu: 2 losse houders in SERIE; BMS BM op middenpunt; laden in externe lader
      (cellen zijn onbeschermd - nooit kortsluiten of mixen)

═════════════════════════════════════════════════════════════════════════
TESTPUNTEN (na solderen, VOOR power-on)
═════════════════════════════════════════════════════════════════════════
☐ Buck OUT = 5.0V ±0.05V
☐ ESP32 5V pin = 5.0V
☐ ESP32 3.3V pin = 3.3V
☐ TB6612 VCC = 3.3V
☐ HC-SR04 VCC = 5.0V
☐ US Echo (met divider) < 3.3V tijdens trigger
☐ Encoder A na deler: 0 ↔ 3,3V bij draaien (GPIO 14/26)
☐ Bat divider uitgang < 0.5V bij 8.4V bat (op GPIO 33)
☐ Alle GND = 0Ω onderling
☐ Geen kortsluiting VCC-GND
☐ Accu pack na BMS = 6,0-8,4V (op P+ / P-)
☐ Geen kortsluiting op P+ / P- (∞ Ω)

═════════════════════════════════════════════════════════════════════════
SOLDER VOLGORDE
═════════════════════════════════════════════════════════════════════════
1. Headers op breakouts (TB6612, MPU6050, 4x TCRT, 2x HC-SR04)
2. Spanningsdelers op perboard (1k/2k US, 210k/10k Bat)
3. Voeding: 2S houder → Buck IN + TB6612 VM
4. Buck OUT → ESP32 5V, TB6612 VCC, sensoren VCC
5. Alle GND samen (sterpunt op perboard)
6. Signaal draden ESP32 → breakouts
7. Motor draden TB6612 AO1/AO2/BO1/BO2 → motoren
8. Encoder draden → 1k/2k deler → GPIO 14/26 (A) + 25/27 (B)
9. Visuele inspectie + multimeter check
10. Eerste power-on: USB only (geen bat), check rails

═════════════════════════════════════════════════════════════════════════
WEBSOCKET PROTOCOL
═════════════════════════════════════════════════════════════════════════
Pi → Rover (10-50Hz):        Rover → Pi (50-100Hz):
{"T":100,"S":0,"M":0}        {"t":12345,"dF":45,"dR":120,
  T: throttle -100..100        "bat":11.3,"imu":{"r":1,"p":-2},
  S: steer -100..100           "enc":{"L":450,"R":448},
  M: mode 0=man/1=auto/2=clr   "line":0.0,"mode":0,"estop":false}
  E: estop_clear (optioneel)

WiFi AP: Rover_AP / rover123  IP: 192.168.4.1  WS: ws://192.168.4.1:80/ws

═════════════════════════════════════════════════════════════════════════
TROUBLESHOOTING QUICK
═════════════════════════════════════════════════════════════════════════
Geen WiFi AP      → Buck 5V checken, ESP32 3.3V checken
Motor draait niet → STBY HIGH? PWM >0? VM spanning?
Encoder 0         → INPUT_PULLUP? 1k/2k deler? GPIO 14/26 (kanaal A)?
                    10 wielomwentelingen met de hand = 28.800 ticks
MPU6050 niet      → Wire1 (16/17)? I²C scanner draaien?
US meet niet      → Echo spanningsdeler? Trig puls 10µs?
Bat 0V            → Divider 21:1? GPIO 33 (ADC1)?
Noodstop werkt niet → GPIO 3 (pull-up, active LOW)?

═════════════════════════════════════════════════════════════════════════
VEILIGHEID
═════════════════════════════════════════════════════════════════════════
⚠ Cellen zijn ONBESCHERMD (INR18650-35E, 8A) - 2S BMS verplicht!
☐ 2 losse houders in SERIE (niet de dubbele/parallelle houder)
☐ BMS BM-pin exact op het middenpunt tussen de cellen
☐ Nooit direct aan een 18650 solderen
☐ Laden in de EXTERNE lader, cellen buiten de rover
☐ Beide cellen altijd samen, zelfde lading
☐ Buck converter EERST op 5.0V instellen (met multimeter)
☐ Spanningsdelers HC-SR04 Echo GEEN 5V op ESP32!
☐ Noodstop knop (GPIO 3) getest
☐ Batterij < 6.6V = hard stop in firmware
☐ MOTOR_MAX_DUTY op 716 (70%) laten staan - motoren zijn 3-6V
☐ Alle GND verbonden voordat voeding aan
════════════════════════════════════════════════════════════════════════