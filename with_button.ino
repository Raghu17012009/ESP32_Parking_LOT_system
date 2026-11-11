#include <LedControl.h>

// Pin Definitions
const int LDR_PIN = A0;
const int POT_PIN = A1;
const int BEAM_LED_PIN = 3;
const int BUTTON_PIN = 2;
const int BUZZER_PIN = 4;

// LED Matrix Pins (DIN, CLK, CS, Number of displays)
LedControl lc = LedControl(12, 11, 10, 1);

// Custom 8x8 Pixel Art Patterns (Bitmaps)
byte vacantIcon[8] = {
  B00000000, B10000000, B01000000, B00100000,
  B00010001, B00001010, B00000100, B00000000
};// Checkmark symbol

byte occupiedIcon[8] = {
  B10000001, B01000010, B00100100, B00011000,
  B00011000, B00100100, B01000010, B10000001
}; // "X" symbol

byte reservedIcon[8] = {
  B00111111, B01000001, B01000001, B00111111,
  B00110001, B01100001, B11000001, B00000000
}; // "R" symbol

// System States
bool isReserved = false;
int lastButtonState = HIGH;

void setup() {
  pinMode(BEAM_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(BEAM_LED_PIN, HIGH);   // Constant light beam source

  lc.shutdown(0, false);              // Wake up the display
  lc.setIntensity(0, 4);              // Brightness (0-15)
  lc.clearDisplay(0);

  Serial.begin(9600);
}

void loop() {
  // 1. Check button for manual reserve toggle
  int currentButtonState = digitalRead(BUTTON_PIN);

  if (lastButtonState == HIGH && currentButtonState == LOW) {
    isReserved = !isReserved;
    delay(200);                       // Debounce
    noTone(BUZZER_PIN);
  }
  lastButtonState = currentButtonState;

  // 2. Read environment sensors
  int ldrValue = analogRead(LDR_PIN);
  int threshold = analogRead(POT_PIN);

  // 3. Compare sensor reading to potentiometer threshold
  bool carIsPresent = (ldrValue < threshold);

  // 4. Decision tree
  if (isReserved) {
    drawPattern(reservedIcon);

    if (carIsPresent) {
      Serial.println("ALARM: Unauthorized parking in reserved spot!");

      // Alternating two-tone siren, switching every 250 ms
      if ((millis() / 250) % 2 == 0) {
        tone(BUZZER_PIN, 1500);
      } else {
        tone(BUZZER_PIN, 1000);
      }
    } else {
      noTone(BUZZER_PIN);             // Reserved but clear
    }

  } else if (carIsPresent) {
    drawPattern(occupiedIcon);        // Legally parked
    noTone(BUZZER_PIN);

  } else {
    drawPattern(vacantIcon);          // Empty and available
    noTone(BUZZER_PIN);
  }

  delay(50);
}

// Helper: render an 8-row bitmap on the matrix
void drawPattern(byte patternArray[]) {
  for (int rowIndex = 0; rowIndex < 8; rowIndex++) {
    lc.setRow(0, rowIndex, patternArray[rowIndex]);
  }
}
