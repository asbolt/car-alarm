#include <LiquidCrystal.h>
#include <Wire.h>
#include "RTClib.h"

// LCD дисплей
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// RTC модуль
RTC_DS1307 rtc;

// Кнопки на шилде
#define BTN_ANALOG A0

// Пин для управления Nano
#define NANO_CONTROL_PIN 3

// Зуммер и светодиоды
#define BUZZER_PIN 11
#define LED_PIN 12

// Константы
int alarmHour = 10;
int alarmMinute = 1;
bool alarmEnabled = true;
bool settingMode = false;
int settingStep = 0;
bool isAlarmTriggered = false;

unsigned long lastBlinkTime = 0;
unsigned long lastBuzzerTime = 0;
bool ledState = false;
bool buzzerState = false;
const unsigned long BLINK_INTERVAL = 500;
const unsigned long BUZZER_INTERVAL = 300;

void setup() {
  // Инициализация LCD
  lcd.begin(16, 2);
  lcd.print("Robot Alarm");
  lcd.setCursor(0, 1);
  lcd.print("Initializing RTC");
  Wire.begin();
  
  // Инициализация RTC
  if (!rtc.begin()) {
    lcd.clear();
    lcd.print("RTC not found!");
    while (1);
  }
  
  // Настройка пина для управления Nano
  pinMode(NANO_CONTROL_PIN, OUTPUT);
  digitalWrite(NANO_CONTROL_PIN, HIGH);
  
  // Настройка зуммера и светодиодов
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);
  
  delay(2000);
  lcd.clear();
}

void loop() {
  // Получение реального времени с RTC
  DateTime now = rtc.now();
  
  // Обработка кнопок
  handleButtons();
  
  // Управление светодиодами и зуммером при срабатывании будильника
  handleAlarmEffects();
  
  // Отображение информации
  updateDisplay(now);
  
  // Проверка будильника
  if (alarmEnabled && !isAlarmTriggered && 
      now.hour() == alarmHour && 
      now.minute() == alarmMinute &&
      now.second() == 0) {
    triggerAlarm();
  }
  
  delay(200);
}

void handleAlarmEffects() {
  if (isAlarmTriggered) {
    unsigned long currentTime = millis();
    
    // Мигание светодиодов
    if (currentTime - lastBlinkTime >= BLINK_INTERVAL) {
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
      lastBlinkTime = currentTime;
    }
    
    // Пропикивание зуммером
    if (currentTime - lastBuzzerTime >= BUZZER_INTERVAL) {
      buzzerState = !buzzerState;
      if (buzzerState) {
        tone(BUZZER_PIN, 1000, 200);
      }
      lastBuzzerTime = currentTime;
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
  }
}

int readButton() {
  int buttonValue = analogRead(BTN_ANALOG);
  if (buttonValue < 50) return 1;      // RIGHT
  else if (buttonValue < 150) return 2; // UP
  else if (buttonValue < 350) return 3; // DOWN
  else if (buttonValue < 500) return 4; // LEFT
  else if (buttonValue < 750) return 5; // SELECT
  else return 0;                       // NONE
}

void handleButtons() {
  int button = readButton();
  
  if (button == 5) { // SELECT
    delay(150);
    if (!settingMode) {
      settingMode = true;
      settingStep = 0;
    } else {
      settingStep++;
      if (settingStep > 2) settingMode = false;
    }
    delay(250);
  }
  
  if (settingMode) {
    if (button == 2) { // UP
      delay(150);
      if (settingStep == 0) alarmHour = (alarmHour + 1) % 24;
      else if (settingStep == 1) alarmMinute = (alarmMinute + 1) % 60;
      delay(250);
    }
    
    if (button == 3) { // DOWN
      delay(150);
      if (settingStep == 0) alarmHour = (alarmHour - 1 + 24) % 24;
      else if (settingStep == 1) alarmMinute = (alarmMinute - 1 + 60) % 60;
      delay(250);
    }
  } else {
    if (button == 4 || button == 1) { // RIGHT/LEFT - вкл/выкл будильник
      delay(150);
      alarmEnabled = !alarmEnabled;
      if (isAlarmTriggered) stopAlarm();
      delay(250);
    }
  }
}

void updateDisplay(DateTime now) {
  lcd.clear();
  
  if (settingMode) {
    lcd.print("Set Alarm:");
    lcd.setCursor(0, 1);
    switch (settingStep) {
      case 0: lcd.print("Hours: "); printTwoDigits(alarmHour); lcd.print("  <--"); break;
      case 1: lcd.print("Minutes: "); printTwoDigits(alarmMinute); lcd.print(" <--"); break;
      case 2: lcd.print("Save? SEL=yes"); break;
    }
  } else if (isAlarmTriggered) {
    lcd.print("ALARM ACTIVE!");
    lcd.setCursor(0, 1);
    lcd.print("Pres R/L to stop");
  } else {
    // Обычный режим с реальным временем (без секунд)
    lcd.print("Time: ");
    printTwoDigits(now.hour());
    lcd.print(":");
    printTwoDigits(now.minute());
    
    lcd.setCursor(0, 1);
    lcd.print("Alarm: ");
    printTwoDigits(alarmHour);
    lcd.print(":");
    printTwoDigits(alarmMinute);
    lcd.print(alarmEnabled ? " ON" : " OFF");
  }
}

void printTwoDigits(int number) {
  if (number < 10) lcd.print("0");
  lcd.print(number);
}

void triggerAlarm() {
  isAlarmTriggered = true;
  digitalWrite(NANO_CONTROL_PIN, LOW);
  Serial.println("ALARM: Машинка запущена!");
}

void stopAlarm() {
  if (isAlarmTriggered) {
    isAlarmTriggered = false;
    digitalWrite(NANO_CONTROL_PIN, HIGH);
    digitalWrite(LED_PIN, LOW);
    noTone(BUZZER_PIN);
    Serial.println("ALARM STOP: Все системы выключены");
  }
}