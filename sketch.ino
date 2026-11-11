#include <SPI.h>
#include <MFRC522.h>
#include <LedControl.h>
#include <EEPROM.h>

// Pin Definitions
const int LDR_PIN = A0;
const int POT_PIN = A1;
const int BEAM_LED_PIN = 3;
const int BUZZER_PIN = 4;
const int RFID_SS_PIN = 10;
const int RFID_RST_PIN = 9;

LedControl lc = LedControl(8, 7, 6, 1);
MFRC522 rfid(RFID_SS_PIN, RFID_RST_PIN);

// Master card: tap it to enter/exit learn mode (change to your own card's UID)
const byte MASTER_UID[4] = {0xC0, 0xFF, 0xEE, 0x99};

// EEPROM layout: [0] = magic, [1] = card count, [2...] = 4 bytes per card
const int MAX_CARDS = 20;
const int ADDR_MAGIC = 0;
const int ADDR_COUNT = 1;
const int ADDR_DATA = 2;
const byte EEPROM_MAGIC = 0xA5;

const unsigned long CARD_COOLDOWN = 2000;   // ms between accepted taps
const unsigned long LEARN_TIMEOUT = 10000;  // ms learn mode stays open

// Custom 8x8 Pixel Art Patterns (Bitmaps)
byte vacantIcon[8] = {
  B00000000, B10000000, B01000000, B00100000,
  B00010001, B00001010, B00000100, B00000000
}; // Checkmark

byte occupiedIcon[8] = {
  B10000001, B01000010, B00100100, B00011000,
  B00011000, B00100100, B01000010, B10000001
}; // "X"

byte reservedIcon[8] = {
  B00111111, B01000001, B01000001, B00111111,
  B00110001, B01100001, B11000001, B00000000
}; // "R"

// System States
bool isReserved = false;
bool alarmPrinted = false;
unsigned long lastCardTime = 0;
byte cardCount = 0;
bool learnMode = false;
unsigned long learnStart = 0;

void setup() {
  pinMode(BEAM_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BEAM_LED_PIN, HIGH);

  lc.shutdown(0, false);
  lc.setIntensity(0, 4);
  lc.clearDisplay(0);

  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();

  loadCards();
  Serial.print("Smart parking ready. Stored cards: ");
  Serial.println(cardCount);
  Serial.println("Tap master card to learn new cards. Serial: 'l' = list, 'c' = clear all.");
}

void loop() {
  handleSerial();
  checkCard();

  // Learn mode timeout
  if (learnMode && millis() - learnStart > LEARN_TIMEOUT) {
    learnMode = false;
    Serial.println("Learn mode timed out.");
  }

  // Sensors
  int ldrValue = analogRead(LDR_PIN);
  int threshold = analogRead(POT_PIN);
  bool carIsPresent = (ldrValue < threshold);

  // Decision tree
  if (isReserved) {
    drawPattern(reservedIcon);

    if (carIsPresent) 
    {
      if (!alarmPrinted) {
        Serial.println("ALARM: Unauthorized parking in reserved spot!");
        alarmPrinted = true;
      }
      if (millis() / 250 % 2 == 0) 
      {
        tone(BUZZER_PIN, 1500);
      }
      else tone(BUZZER_PIN, 1000);
    } 
    else {
      noTone(BUZZER_PIN);
      alarmPrinted = false;
    }

  } else if (carIsPresent) {
    drawPattern(occupiedIcon);
    noTone(BUZZER_PIN);
    alarmPrinted = false;

  } else {
    drawPattern(vacantIcon);
    noTone(BUZZER_PIN);
    alarmPrinted = false;
  }

  delay(50);
}

// ---------- EEPROM card storage ----------

void loadCards() {
  if (EEPROM.read(ADDR_MAGIC) != EEPROM_MAGIC) 
  {   // first run: initialize
    EEPROM.update(ADDR_MAGIC, EEPROM_MAGIC);
    EEPROM.update(ADDR_COUNT, 0);
  }
  cardCount = EEPROM.read(ADDR_COUNT);
  if (cardCount > MAX_CARDS) cardCount = 0;        // corrupt data guard
}

// Returns the card's index, or -1 if not stored
int findCard(const byte *uid) 
{
  for (int i = 0; i < cardCount; i++) {
    bool match = true;
    for (int j = 0; j < 4; j++) {
      if (EEPROM.read(ADDR_DATA + i * 4 + j) != uid[j]) { match = false; break; }
    }
    if (match) return i;
  }
  return -1;
}

bool addCard(const byte *uid) 
{
  if (cardCount >= MAX_CARDS) return false;
  for (int j = 0; j < 4; j++) EEPROM.update(ADDR_DATA + cardCount * 4 + j, uid[j]);
  cardCount++;
  EEPROM.update(ADDR_COUNT, cardCount);
  return true;
}

void clearCards() 
{
  cardCount = 0;
  EEPROM.update(ADDR_COUNT, 0);
}

void listCards() {
  Serial.print("Stored cards: ");
  Serial.println(cardCount);
  for (int i = 0; i < cardCount; i++) {
    Serial.print(i + 1);
    Serial.print(":");
    for (int j = 0; j < 4; j++) {
      byte b = EEPROM.read(ADDR_DATA + i * 4 + j);
      Serial.print(b < 0x10 ? " 0x0" : " 0x");
      Serial.print(b, HEX);
    }
    Serial.println();
  }
}

void handleSerial() {
  while (Serial.available()) 
  {
    char c = Serial.read();
    if (c == 'l') listCards();
    else if (c == 'c') {
      clearCards();
      Serial.println("All stored cards cleared.");
    }
  }
}

// ---------- RFID handling ----------

void printUid() {
  Serial.print("Card UID:");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i] < 0x10 ? " 0x0" : " 0x");
    Serial.print(rfid.uid.uidByte[i], HEX);
  }
  Serial.println();
}

void beep(int freq, int ms) {
  tone(BUZZER_PIN, freq, ms);
  delay(ms + 30);
}

void checkCard() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) return;

  printUid();

  if (rfid.uid.size != 4) {
    Serial.println("Unsupported UID length (only 4-byte UIDs are handled).");
    beep(400, 300);
  }
  else if (memcmp(rfid.uid.uidByte, MASTER_UID, 4) == 0) {
    // Master card toggles learn mode
    learnMode = !learnMode;
    learnStart = millis();
    if (learnMode) {
      Serial.println("LEARN MODE ON: tap a new card to register it.");
      beep(1800, 120); beep(2400, 120);
    } else {
      Serial.println("Learn mode off.");
      beep(2400, 120); beep(1800, 120);
    }
  }
  else if (learnMode) {
    // Register a new card
    if (findCard(rfid.uid.uidByte) >= 0) {
      Serial.println("Already registered.");
      beep(600, 200);
    } else if (addCard(rfid.uid.uidByte)) {
      Serial.print("Card registered. Total cards: ");
      Serial.println(cardCount);
      beep(2000, 100); beep(2000, 100); beep(2600, 150);
      learnStart = millis();                       // keep the window open for more cards
    } else {
      Serial.println("Storage full. Clear cards with 'c' first.");
      beep(400, 400);
    }
  }
  else {
    // Normal mode: registered card toggles the reservation
    int idx = findCard(rfid.uid.uidByte);
    if (idx >= 0) {
      if (millis() - lastCardTime > CARD_COOLDOWN) {
        lastCardTime = millis();
        isReserved = !isReserved;
        alarmPrinted = false;
        noTone(BUZZER_PIN);

        Serial.print("Card ");
        Serial.print(idx + 1);
        if (isReserved) {
          Serial.println(" RESERVED the spot.");
          beep(1800, 120); beep(2400, 120);
        } else {
          Serial.println(" RELEASED the spot.");
          beep(2400, 120); beep(1800, 120);
        }
        noTone(BUZZER_PIN);
      }
    } else {
      Serial.println("Access denied: unknown card.");
      beep(400, 300);
    }
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}

void drawPattern(byte patternArray[]) {
  for (int rowIndex = 0; rowIndex < 8; rowIndex++) {
    lc.setRow(0, rowIndex, patternArray[rowIndex]);
  }
}