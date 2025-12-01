// Драйвер мотора - задние колеса
#define MOTOR_IN1 4  
#define MOTOR_IN2 5    

// Драйвер сервопривода - передние колеса
#define SERVO_IN3 2  
#define SERVO_IN4 3

// Дальнометры
#define TRIG_PIN 6  
#define ECHO_PIN_LEFT 8  
#define ECHO_PIN_RIGHT 7

// Пин для приема команд от Arduino Uno
#define COMMAND_PIN 12

// Константы
#define MAX_DISTANCE 200
#define OBSTACLE_DISTANCE 25
#define AVOID_TIME 1000
#define BACKUP_TIME 300

int steeringAngle = 90;
bool isActive = false;
unsigned long alarmStartTime = 0;
const unsigned long ALARM_DURATION = 300000; 
unsigned long lastRandomTurn = 0;
const unsigned long RANDOM_TURN_INTERVAL = 3000;

// Переменные для обработки сигнала
bool lastCommandState = HIGH;
bool currentCommandState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 50;

void setup() {
  // Настройка драйвера мотора
  pinMode(MOTOR_IN1, OUTPUT);
  pinMode(MOTOR_IN2, OUTPUT);
  
  // Настройка сервопривода
  pinMode(SERVO_IN3, OUTPUT);
  pinMode(SERVO_IN4, OUTPUT);
  
  // Настройка дальнометров
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN_LEFT, INPUT);
  pinMode(ECHO_PIN_RIGHT, INPUT);
  
  // Настройка пина для команд от Uno
  pinMode(COMMAND_PIN, INPUT_PULLUP);
  
  // Начальная позиция - прямо
  setSteering(90);
  stopMoving();
}

void loop() {
  // Проверка команды от Arduino Uno
  checkCommand();
  
  if (isActive) {
    // Проверка времени работы будильника
    if (millis() - alarmStartTime > ALARM_DURATION) {
      stopAlarm();
      return;
    }
    
    int leftDist, rightDist;
    checkDistances(leftDist, rightDist);
    
    // Объезд препятствий
    avoidObstacles(leftDist, rightDist);
    
  }
  
  delay(50);
}

void checkCommand() {
  bool reading = digitalRead(COMMAND_PIN);
  
  // Если состояние изменилось
  if (reading != lastCommandState) {
    lastDebounceTime = millis();
  }
  
  // Если прошло достаточно времени после последнего изменения
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    // Если состояние стабилизировалось и отличается от текущего
    if (reading != currentCommandState) {
      currentCommandState = reading;
      
      // Обработка команды
      if (currentCommandState == LOW) {
        // Сигнал LOW - активировать будильник
        if (!isActive) {
          startAlarm();
        }
      } else {
        // Сигнал HIGH - деактивировать будильник
        if (isActive) {
          stopAlarm();
        }
      }
    }
  }
  
  lastCommandState = reading;
}

// ФУНКЦИИ ДЛЯ ДАЛЬНОМЕТРОВ
void checkDistances(int &leftDistance, int &rightDistance) {
  leftDistance = getDistance(ECHO_PIN_LEFT);
  delay(10);
  rightDistance = getDistance(ECHO_PIN_RIGHT);
}

int getDistance(int echoPin) {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;
  
  if (distance > MAX_DISTANCE || distance <= 0) {
    return MAX_DISTANCE;
  }
  return distance;
}

// ФУНКЦИИ ДЛЯ ДВИЖЕНИЯ
void startAlarm() {
  isActive = true;
  alarmStartTime = millis();
  setSteering(90);
  moveForward();
}

void stopAlarm() {
  isActive = false;
  stopMoving();
  setSteering(90);
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

// ФУНКЦИИ ДЛЯ РУЛЕВОГО УПРАВЛЕНИЯ
void setSteering(int angle) {
  
  if (angle < 90) {
    // Поворот влево
    digitalWrite(SERVO_IN3, HIGH);
    digitalWrite(SERVO_IN4, LOW);
    delay(map(angle, 0, 89, 50, 5));
  } else if (angle > 90) {
    // Поворот вправо
    digitalWrite(SERVO_IN3, LOW);
    digitalWrite(SERVO_IN4, HIGH);
    delay(map(angle, 91, 180, 5, 50));
  } else {
    // Прямо - остановка
    digitalWrite(SERVO_IN3, LOW);
    digitalWrite(SERVO_IN4, LOW);
  }
  
  steeringAngle = angle;
  delay(10);
}

// ЛОГИКА ОБЪЕЗДА ПРЕПЯТСТВИЙ
void avoidObstacles(int leftDist, int rightDist) {
  // Если нет препятствий - едем вперед со случайными поворотами
  if (leftDist > OBSTACLE_DISTANCE && rightDist > OBSTACLE_DISTANCE) {
    moveForward();
    randomSteering();
  }
  // Препятствие слева - поворачиваем направо
  else if (leftDist <= OBSTACLE_DISTANCE && rightDist > OBSTACLE_DISTANCE) {
    avoidRight();
  }
  // Препятствие справа - поворачиваем налево
  else if (rightDist <= OBSTACLE_DISTANCE && leftDist > OBSTACLE_DISTANCE) {
    avoidLeft();
  }
  // Препятствие прямо - выбираем лучшее направление
  else if (leftDist <= OBSTACLE_DISTANCE && rightDist <= OBSTACLE_DISTANCE) {
    avoidObstacleAhead(leftDist, rightDist);
  }
}

void randomSteering() {
  if (millis() - lastRandomTurn > RANDOM_TURN_INTERVAL) {
    int randomAngle = random(60, 121); // от 60 до 120 градусов
    setSteering(randomAngle);
    lastRandomTurn = millis();
  }
}

void avoidLeft() {
  setSteering(60); // Резко налево
  moveForward();
  delay(AVOID_TIME);
  setSteering(90); // Возвращаем прямо
}

void avoidRight() {
  setSteering(120); // Резко направо
  moveForward();
  delay(AVOID_TIME);
  setSteering(90); // Возвращаем прямо
}

void avoidObstacleAhead(int leftDist, int rightDist) {
  
  // 1. Останавливаемся
  stopMoving();
  delay(500);
  
  // 2. Немного сдаем назад
  setSteering(90);
  moveBackward();
  delay(BACKUP_TIME);
  stopMoving();
  delay(200);
  
  // 3. Выбираем направление с большим пространством
  if (leftDist > rightDist) {
    setSteering(45); // Резко налево
    moveForward();
    delay(AVOID_TIME * 1.5);
  } else {
    setSteering(135); // Резко направо
    moveForward();
    delay(AVOID_TIME * 1.5);
  }
  
  // 4. Возвращаемся к прямолинейному движению
  setSteering(90);
}