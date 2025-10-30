// ПЕРЕМЕННЫЕ

// Драйвер мотора
#define MOTOR_IN1 8  
#define MOTOR_IN2 9    
// ENA подключаем напрямую к +5V (для максимальной скорости)

// Сервопривод для поворота передних колес
#define STEERING_SERVO 3  

// Дальнометр (возможны траблы, тк не знаю модель)
#define TRIG_PIN 11  
#define ECHO_PIN 12  
#define MAX_DISTANCE 200

// Светодиоды
#define LED1 2     
#define LED2 4     
#define LED3 5     
#define LED4 6     

// Зуммер
#define BUZZER 7     

bool isActive = false;
unsigned long alarmTime = 0;
const unsigned long ALARM_DURATION = 300000; // 5 минут

int steeringAngle = 90;      // Угол сервопривода (90 - прямо) - ЭТО НУЖНО ПРОВЕРЯТЬ ПРИ ЗАПУСКЕ, ВОЗМОЖНО ТАМ ДРУГОЕ ЗНАЧЕНИЕ

// ФУНКЦИИ ДЛЯ ДРАЙВЕРА

void setupMotor() 
{
    pinMode(MOTOR_IN1, OUTPUT);
    pinMode(MOTOR_IN2, OUTPUT);
  
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
}

void moveForward() {
    digitalWrite(MOTOR_IN1, HIGH);
    digitalWrite(MOTOR_IN2, LOW);
}

void moveBackward() {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, HIGH);
}

void stopMoving() {
    digitalWrite(MOTOR_IN1, LOW);
    digitalWrite(MOTOR_IN2, LOW);
}

// ФУНКЦИИ ДЛЯ СЕРВОПРИВОДА

void setupServo() {
    pinMode(STEERING_SERVO, OUTPUT);
}

void setSteering(int angle) {
  // angle: 0-180, где 90 - прямо, <90 - влево, >90 - вправо
  int pulseWidth = map(angle, 0, 180, 500, 2400);
  
  digitalWrite(STEERING_SERVO, HIGH);
  delayMicroseconds(pulseWidth);
  digitalWrite(STEERING_SERVO, LOW);
  delay(20); // нужно для стабильной работы (на +- 50 Hz)
  
  steeringAngle = angle;
}

// ФУНКЦИИ ДЛЯ ДАЛЬНОМЕТРА

int checkDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH);
  int distance = duration * 0.034 / 2;
  
  if (distance > MAX_DISTANCE || distance <= 0) {
    return 0;
  }
  return distance;
}

// ОСНОВНОЙ КОД

void setup() {
  // Настройка драйвера мотора
  setupMotor();
  
  // Настройка сервопривода
  setupServo();
  
  // Настройка светодиодов и зуммера
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  
  // Настройка дальнометра
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Устанавливаем прямолинейное движение и остановку
  setSteering(90);
  stopMoving();
  
  // Инициализация Serial для связи с Uno
  Serial.begin(9600);
}

void loop() {
  // Проверка команды от Arduino Uno
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 'S') {
      isActive = true;
      alarmTime = millis();
      startAlarm();
    } else if (command == 'T') {
      isActive = false;
      stopAlarm();
    }
  }
  
  if (isActive) {
    // Проверка времени работы
    if (millis() - alarmTime > ALARM_DURATION) {
      isActive = false;
      stopAlarm();
      return;
    }
    
    // Основная логика движения
    int distance = checkDistance();
    
    if (distance > 25 || distance == 0) {
      // Нет препятствий - едем вперед со случайными поворотами
      moveForward();
      randomSteering();
    } else {
      // Препятствие обнаружено - объезжаем
      avoidObstacle();
    }
    
    // Мигание светодиодов и звук
    blinkLights();
    playBuzzer();
    
  } else {
    stopAlarm(); 
  }
  
  delay(100);
}

// ДВИЖЕНИЕ

void startAlarm() {
  moveForward();
  setSteering(90); // Начинаем движение прямо
}

void stopAlarm() {
  stopMoving();
  setSteering(90); // Возвращаем колеса прямо
  turnOffLights();
  noTone(BUZZER);
}

void randomSteering() {
  // Случайные повороты каждые 3 секунды
  if (millis() % 3000 < 100) {
    // Случайный угол между 60 и 120 градусами
    steeringAngle = random(60, 121);
    setSteering(steeringAngle);
  }
}

void avoidObstacle() {
  // 1. Останавливаемся
  stopMoving();
  delay(500);
  
  // 2. Немного сдаем назад
  setSteering(90); // Прямо назад
  moveBackward();
  delay(300);
  stopMoving();
  delay(200);
  
  // 3. Выбираем направление для объезда
  if (random(0, 2) == 0) {
    // Поворот налево
    setSteering(60); // Резко налево
    moveForward();
    delay(800); // Едем с поворотом
  } else {
    // Поворот направо
    setSteering(120); // Резко направо
    moveForward();
    delay(800); // Едем с поворотом
  }
  
  // 4. Возвращаемся к прямолинейному движению
  setSteering(90);
}

// СВЕТ И ЗВУК

void blinkLights() {
  static unsigned long lastBlink = 0;
  static bool lightsOn = false;
  
  if (millis() - lastBlink > 300) {
    lightsOn = !lightsOn;
    digitalWrite(LED1, lightsOn);
    digitalWrite(LED2, lightsOn);
    digitalWrite(LED3, lightsOn);
    digitalWrite(LED4, lightsOn);
    lastBlink = millis();
  }
}

void turnOffLights() {
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(LED3, LOW);
  digitalWrite(LED4, LOW);
}

void playBuzzer() {
  static unsigned long lastTone = 0;
  static bool toneOn = false;
  
  if (millis() - lastTone > 500) {
    toneOn = !toneOn;
    if (toneOn) {
      tone(BUZZER, 1000);
    } else {
      noTone(BUZZER);
    }
    lastTone = millis();
  }
}