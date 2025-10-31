#include <LiquidCrystal.h>

// LCD дисплей
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Кнопки на шилде
#define BTN_ANALOG A0

// Переменные времени (эмулируем RTC)
int currentHour = 10;
int currentMinute = 0;
int currentSecond = 0;

// Будильник
int alarmHour = 10;
int alarmMinute = 1;
bool alarmEnabled = true;
bool settingMode = false;
int settingStep = 0;
bool isAlarmTriggered = false;

void setup() {
  // Инициализация LCD
  lcd.begin(16, 2);
  lcd.print("Robot Alarm");
  lcd.setCursor(0, 1);
  lcd.print("RTC Disabled");
  
  delay(2000);
  
  // Настройка Serial для связи с Nano
  Serial.begin(9600);
}

void loop() {
  // Эмуляция времени (вместо RTC)
  updateTime();
  
  // Обработка кнопок
  handleButtons();
  
  // Отображение информации
  updateDisplay();
  
  // Проверка будильника
  if (alarmEnabled && !isAlarmTriggered && 
      currentHour == alarmHour && 
      currentMinute == alarmMinute &&
      currentSecond == 0) {
    triggerAlarm();
  }
  
  delay(200);
}

void updateTime() {
  static unsigned long lastTimeUpdate = 0;
  if (millis() - lastTimeUpdate > 1000) {
    currentSecond++;
    if (currentSecond >= 60) {
      currentSecond = 0;
      currentMinute++;
      if (currentMinute >= 60) {
        currentMinute = 0;
        currentHour++;
        if (currentHour >= 24) {
          currentHour = 0;
        }
      }
    }
    lastTimeUpdate = millis();
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

void updateDisplay() {
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
    lcd.print("Press R/L to stop");
  } else {
    // Обычный режим
    lcd.print("Time: ");
    printTwoDigits(currentHour);
    lcd.print(":");
    printTwoDigits(currentMinute);
    
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
  Serial.write('S');
}

void stopAlarm() {
  if (isAlarmTriggered) {
    isAlarmTriggered = false;
    Serial.write('T');
  }
}