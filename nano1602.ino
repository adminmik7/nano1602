/*
 * PC & DHT11 Monitor — Arduino Nano + LCD1602 (I2C)
 * Line 1: CPU load + RAM usage
 * Line 2: Temperature + Humidity (from DHT11) or Status
 * Data via USB Serial @ 9600 baud
 *
 * Format:  CPU:XX|RAM:XX|TEMP:XX|HUM:XX
 *
 * Wiring:
 *   LCD1602 (I2C): SDA->A4, SCL->A5, VCC->5V, GND->GND
 *   DHT11: DATA->D2, VCC->5V, GND->GND
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>

// LCD1602 — адрес 0x27 (если не работает, попробуй 0x3F)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// DHT11 Setup
#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
bool dhtFound = false;

// ─── Состояние ───────────────────────────────────────────
int cpuLoad  = 0;
int ramUsage = 0;
float tempVal = 0.0;
float humVal = 0.0;

unsigned long lastUpdate = 0;
bool usbConnected = false;

static const unsigned long TIMEOUT_MS = 3000;
static const byte MAX_BUF = 64;
char buffer[MAX_BUF];
byte bufPos = 0;
static bool waitingShown = false;

// ─── SETUP ───────────────────────────────────────────────
void setup() {
  Serial.begin(9600);

  // LCD Init
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  PC & DHT Mon  ");
  lcd.setCursor(0, 1);
  lcd.print("  Starting...   ");

  // DHT11 Init
  dht.begin();
  delay(1000);
  // Пробуем прочитать, чтобы понять, подключен ли он
  float t = dht.readTemperature();
  if (!isnan(t)) {
    dhtFound = true;
    lcd.setCursor(0, 1);
    lcd.print("  DHT11 Found!   ");
  } else {
    dhtFound = false;
    lcd.setCursor(0, 1);
    lcd.print(" No DHT11 Sensor ");
  }
  delay(1500);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Waiting for PC ");
  lcd.setCursor(0, 1);
  lcd.print("    via USB     ");
}

// ─── LOOP ────────────────────────────────────────────────
void loop() {
  // Читаем Serial
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (!usbConnected) {
      usbConnected = true;
      lcd.clear();
    }

    if (c == '\n') {
      buffer[bufPos] = '\0';
      parseData(buffer);
      bufPos = 0;
      memset(buffer, 0, sizeof(buffer));
    } else if (bufPos < MAX_BUF - 1) {
      buffer[bufPos++] = c;
    }
  }

  // Проверка потери связи
  if (millis() - lastUpdate > TIMEOUT_MS) {
    if (usbConnected) {
      usbConnected = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("USB Lost!");
      lcd.setCursor(0, 1);
      lcd.print("Waiting...");
    }
  }

  if (usbConnected) {
    updateDisplay();
  }
  
  delay(200);
}

// ─── Парсинг данных ─────────────────────────────────────
void parseData(char* data) {
  char* cpuPtr = strstr(data, "CPU:");
  char* ramPtr = strstr(data, "RAM:");
  char* tempPtr = strstr(data, "TEMP:");
  char* humPtr = strstr(data, "HUM:");

  if (cpuPtr) cpuLoad = atoi(cpuPtr + 4);
  if (ramPtr) ramUsage = atoi(ramPtr + 4);
  
  // Обновляем данные с датчика, если они пришли по USB, 
  // или берем локально, если датчик есть
  if (tempPtr) {
    char tempStr[8];
    strncpy(tempStr, tempPtr + 5, 7);
    tempStr[7] = '\0';
    tempVal = atof(tempStr);
  } else if (dhtFound) {
    tempVal = dht.readTemperature();
  }
  
  if (humPtr) {
    char humStr[8];
    strncpy(humStr, humPtr + 4, 7);
    humStr[7] = '\0';
    humVal = atof(humStr);
  } else if (dhtFound) {
    humVal = dht.readHumidity();
  }

  lastUpdate = millis();
  usbConnected = true;
  Serial.println("OK");
}

// ─── Главный цикл дисплея ──────────────────────────────
void updateDisplay() {
  if (!usbConnected) return;

  // Строка 1: CPU и RAM
  lcd.setCursor(0, 0);
  lcd.print("CPU:");
  if (cpuLoad < 10) lcd.print(" ");
  lcd.print(cpuLoad);
  lcd.print("%  RAM:");
  if (ramUsage < 10) lcd.print(" ");
  lcd.print(ramUsage);
  lcd.print("% ");

  // Строка 2: Температура и Влажность или статус
  lcd.setCursor(0, 1);
  if (dhtFound) {
    if (isnan(tempVal) || isnan(humVal)) {
       lcd.print("Sensor Error!   ");
    } else {
       lcd.print("T:");
       lcd.print(tempVal, 1);
       lcd.print((char)223); // Градус
       lcd.print("C  H:");
       lcd.print(humVal, 1);
       lcd.print("% ");
    }
  } else {
    lcd.print(" No DHT11 Sensor ");
  }
}
