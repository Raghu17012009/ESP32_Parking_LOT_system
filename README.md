# 🚗 Smart Parking Spot (Arduino)

A smart parking spot indicator built on Arduino. An LDR and light beam detect whether a car is in the spot, an 8×8 LED matrix shows the spot's status, and a buzzer sounds an alarm if someone parks in a reserved spot. The spot can be reserved and released with either a push button or an RFID card.

## Versions

| File | Reservation method | Description |
|------|--------------------|-------------|
| `with_button.ino` | Push button | Basic version. Press the button to toggle the reservation. |
| `with_card_reader.ino` | RFID cards (MFRC522) | Advanced version. Registered cards toggle the reservation, with card storage in EEPROM and a master card for adding new cards. |

## Features

- **Car detection:** an LED beam shines on an LDR. When the car blocks the light, the reading drops below a threshold.
- **Adjustable sensitivity:** a potentiometer sets the detection threshold, so it works in different lighting conditions.
- **8×8 LED matrix display:**
  - ✔ Checkmark: spot is vacant
  - ✖ "X": spot is occupied
  - **R** : spot is reserved
- **Unauthorized parking alarm:** if the spot is reserved and a car is detected, the buzzer plays an alternating two-tone siren (1500 Hz / 1000 Hz) and the event is logged to Serial.

### RFID version only (`with_card_reader.ino`)

- **Card-based reserve/release:** tap a registered card to reserve the spot, tap again to release it.
- **Master card and learn mode:** tap the master card to enter learn mode (it times out after 10 s), then tap new cards to register them.
- **Persistent storage:** up to 20 cards (4-byte UIDs) are stored in EEPROM and survive power cycles.
- **Audio feedback:** different beep patterns for reserved, released, registered, already registered, denied and storage-full events.
- **Cooldown:** a 2-second delay between accepted taps prevents accidental double toggling.
- **Serial commands:** send `l` to list stored cards and `c` to clear all cards (9600 baud).

## Hardware Required

**Both versions**
- Arduino Uno (or compatible)
- LDR (photoresistor) and a resistor for the voltage divider
- LED (light beam source)
- 10 kΩ potentiometer
- 8×8 LED matrix with MAX7219 driver
- Buzzer
- Jumper wires and breadboard

**Button version:** push button
<img width="645" height="734" alt="image" src="https://github.com/user-attachments/assets/f8ea7932-25f2-49a9-bda9-679f60fd679b" />

**RFID version:** MFRC522 RFID reader and RFID cards/tags
<img width="1022" height="700" alt="image" src="https://github.com/user-attachments/assets/e67e6abd-ecfb-49c6-9ec1-16ce1f0bcfc4" />

## Pin Connections

| Component | `with_button.ino` | `with_card_reader.ino` |
|-----------|-------------------|--------------|
| LDR | A0 | A0 |
| Potentiometer | A1 | A1 |
| Beam LED | D3 | D3 |
| Buzzer | D4 | D4 |
| Push button | D2 (to GND, internal pull-up) | not used |
| LED matrix DIN / CLK / CS | D12 / D11 / D10 | D8 / D7 / D6 |
| MFRC522 SDA (SS) / RST | not used | D10 / D9 |
| MFRC522 SCK / MOSI / MISO | not used | D13 / D11 / D12 (hardware SPI) |

> ⚠️ The LED matrix pins differ between the two with_card_readeres, so wire it according to the version you use.

## Libraries

Install these from the Arduino Library Manager:

- [`LedControl`](https://github.com/wayoda/LedControl)
- [`MFRC522`](https://github.com/miguelbalboa/rfid) (RFID version only)
- `SPI` and `EEPROM` (built in)

## Getting Started

1. Clone the repository:
   ```bash
   git clone https://github.com/<your-username>/<your-repo>.git
   ```
2. Open `with_button.ino` or `with_card_reader.ino` in the Arduino IDE.
3. Install the required libraries.
4. Wire the circuit using the pin table above.
5. Select your board and port, then upload.
6. Turn the potentiometer until the matrix shows ✔ with the spot empty and ✖ with the beam blocked.

### Setting up the master card (RFID version)

1. Upload the with_card_reader and open the Serial Monitor at **9600 baud**.
2. Tap the card you want as your master card and note its UID.
3. Replace the UID in the code:
   ```cpp
   const byte MASTER_UID[4] = {0xC0, 0xFF, 0xEE, 0x99};
   ```
4. Re-upload. Tap the master card to enter learn mode, then tap each card you want to register.

## How It Works

```
Reserved?
 ├─ Yes ─ show "R" ─ car present? ─ Yes → siren alarm
 │                               └ No  → silent
 └─ No  ─ car present? ─ Yes → show "X" (occupied)
                       └ No  → show ✔ (vacant)
```

A car is detected when `analogRead(LDR) < analogRead(POT)`, so the potentiometer acts as the sensitivity knob.

## Future Improvements

- Multiple parking spots with a central display
- Wi-Fi/IoT integration for remote status
- Reservation timeout
- Support for 7- and 10-byte RFID UIDs

## License

This project is open source under the [MIT License](LICENSE).
