#include <Wire.h>
#include <RTClib.h>
#include <LiquidCrystal.h>  // Для LCD дисплея на шилде

// ПЕРЕМЕННЫЕ

// LCD дисплей
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Кнопки на шилде
#define BTN_SET 2      // Кнопка установки/подтверждения
#define BTN_UP 3       // Кнопка вверх
#define BTN_DOWN 4     // Кнопка вниз

// RTC модуль
RTC_DS3231 rtc;

// Время будильника
int alarmHour = 10;      // Часы будильника (7:00 по умолчанию)
int alarmMinute = 0;    // Минуты будильника
bool alarmEnabled = true; // Включен ли будильник

// Переменные для меню установки
bool settingMode = false;  // Режим установки времени
int settingStep = 0;       // Текущий шаг установки: 0-часы, 1-минуты, 2-сохранение

// Состояние будильника
bool isAlarmTriggered = false;
unsigned long alarmStartTime = 0;

// ========== ФУНКЦИЯ НАСТРОЙКИ ==========

void setup() {
  // Настройка кнопок
  pinMode(BTN_SET, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  
  // Инициализация LCD
  lcd.begin(16, 2);
  lcd.print("Robot Alarm");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");
  
  // Инициализация RTC
  if (!rtc.begin()) {
    lcd.clear();
    lcd.print("RTC error!");
    while (1);
  }
  
  // Если RTC потерял время, устанавливаем компиляционное время
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  
  delay(2000);
  lcd.clear();
  
  // Настройка Serial для связи с Nano
  Serial.begin(9600);
}

// ========== ГЛАВНЫЙ ЦИКЛ ==========

void loop() {
  // Получаем текущее время
  DateTime now = rtc.now();
  
  // Обработка кнопок
  handleButtons();
  
  // Отображение информации на LCD
  updateDisplay(now);
  
  // Проверка срабатывания будильника
  if (alarmEnabled && !isAlarmTriggered && 
      now.hour() == alarmHour && 
      now.minute() == alarmMinute &&
      now.second() == 0) {
    triggerAlarm();
  }
  
  // Автоматическое выключение будильника через 5 минут
  if (isAlarmTriggered && millis() - alarmStartTime > 300000) {
    stopAlarm();
  }
  
  delay(200); // Небольшая задержка для стабильности
}

// ========== ФУНКЦИИ ОБРАБОТКИ КНОПОК ==========

void handleButtons() {
  // Кнопка SET - вход/подтверждение в режиме установки
  if (digitalRead(BTN_SET) == LOW) {
    delay(250); // Защита от дребезга
    
    if (!settingMode) {
      // Вход в режим установки
      settingMode = true;
      settingStep = 0;
    } else {
      // Переход к следующему шагу установки
      settingStep++;
      if (settingStep > 2) {
        // Выход из режима установки
        settingMode = false;
        // Сохраняем настройки
      }
    }
    
    while(digitalRead(BTN_SET) == LOW); // Ждем отпускания кнопки
  }
  
  // Обработка кнопок только в режиме установки
  if (settingMode) {
    // Кнопка UP - увеличение значения
    if (digitalRead(BTN_UP) == LOW) {
      delay(150);
      if (settingStep == 0) {
        alarmHour = (alarmHour + 1) % 24;
      } else if (settingStep == 1) {
        alarmMinute = (alarmMinute + 1) % 60;
      }
      while(digitalRead(BTN_UP) == LOW);
    }
    
    // Кнопка DOWN - уменьшение значения
    if (digitalRead(BTN_DOWN) == LOW) {
      delay(150);
      if (settingStep == 0) {
        alarmHour = (alarmHour - 1 + 24) % 24;
      } else if (settingStep == 1) {
        alarmMinute = (alarmMinute - 1 + 60) % 60;
      }
      while(digitalRead(BTN_DOWN) == LOW);
    }
  } else {
    // В обычном режиме кнопка DOWN отключает/включает будильник
    if (digitalRead(BTN_DOWN) == LOW) {
      delay(250);
      alarmEnabled = !alarmEnabled;
      if (isAlarmTriggered) {
        stopAlarm();
      }
      while(digitalRead(BTN_DOWN) == LOW);
    }
  }
}

// ========== ФУНКЦИИ ОТОБРАЖЕНИЯ НА LCD ==========

void updateDisplay(DateTime now) {
  lcd.clear();
  
  if (settingMode) {
    // Режим установки времени
    displaySettingMode();
  } else if (isAlarmTriggered) {
    // Режим сработавшего будильника
    lcd.print("ALARM ACTIVE!");
    lcd.setCursor(0, 1);
    lcd.print("Press DOWN to stop");
  } else {
    // Обычный режим - показ времени и будильника
    displayNormalMode(now);
  }
}

void displayNormalMode(DateTime now) {
  // Первая строка - текущее время
  lcd.print("Time: ");
  printTwoDigits(now.hour());
  lcd.print(":");
  printTwoDigits(now.minute());
  
  // Вторая строка - будильник и статус
  lcd.setCursor(0, 1);
  lcd.print("Alarm: ");
  printTwoDigits(alarmHour);
  lcd.print(":");
  printTwoDigits(alarmMinute);
  
  // Индикатор статуса будильника
  if (alarmEnabled) {
    lcd.print(" ON");
  } else {
    lcd.print(" OFF");
  }
}

void displaySettingMode() {
  lcd.print("Set Alarm:");
  lcd.setCursor(0, 1);
  
  switch (settingStep) {
    case 0: // Установка часов
      lcd.print("Hours: ");
      printTwoDigits(alarmHour);
      lcd.print("  <--");
      break;
      
    case 1: // Установка минут
      lcd.print("Minutes: ");
      printTwoDigits(alarmMinute);
      lcd.print(" <--");
      break;
      
    case 2: // Подтверждение
      lcd.print("Save? SET=yes");
      break;
  }
}

void printTwoDigits(int number) {
  if (number < 10) {
    lcd.print("0");
  }
  lcd.print(number);
}

// УПРАВЛЕНИЕ БУДИЛЬНИКОМ

void triggerAlarm() {
  isAlarmTriggered = true;
  alarmStartTime = millis();
  Serial.write('S'); // Отправка команды старта на Nano
}

void stopAlarm() {
  if (isAlarmTriggered) {
    isAlarmTriggered = false;
    Serial.write('T'); // Отправка команды остановки на Nano
    digitalWrite(13, LOW); // Выключаем светодиод
  }
}