#include <TM1637Display.h>
#include <Arduino.h>

// HC-SR04
#define echoPin 4
#define trigPin 3

// Push buttons and outputs
#define modePin 2
#define incTimePin 5
#define decTimePin 6
#define startTimerPin 7
#define buzzerPin 10
#define ledPin 11

// 4-digit display pins
#define CLK 8
#define DIO 9
TM1637Display display(CLK, DIO);

//FUNCTION FOR LOOP
void handleTimerMode();
void resetSystem();
void handlePushUpCounter();

//initiliaze global variable
float duration; // microsecond
float d_cm;
long numDisplay = 100;
boolean trigUp = false;
boolean trigDown = false;
float counterPushUp = 0;

//initialize timer count
volatile boolean timerMode = false;
volatile long timerCountdown = 0;
volatile boolean timerRunning = false;

boolean lastModePinState = HIGH;

void setup() {
  Serial.begin(9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(modePin, INPUT_PULLUP);
  pinMode(incTimePin, INPUT_PULLUP);
  pinMode(decTimePin, INPUT_PULLUP);
  pinMode(startTimerPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(modePin),handleTimerMode, CHANGE);


  display.setBrightness(4);
  display.clear();
  delay(500);
  display.showNumberDecEx(numDisplay, false, true, 4, 0);
}

void loop() {
  
  boolean currentModePinState = digitalRead(modePin);

  if (currentModePinState == LOW && lastModePinState == HIGH) {
    timerMode = !timerMode;
    resetSystem();
    delay(500); // Debounce delay
  }

  lastModePinState = currentModePinState;

  if (timerMode) {
    handleTimerMode();
  } else {
    handlePushUpCounter();
  }
}

void resetSystem() {

  trigUp = false;
  trigDown = false;
  counterPushUp = 0;
  timerRunning = false;
  timerCountdown = 0;
  digitalWrite(buzzerPin, LOW);
  digitalWrite(ledPin, LOW);
  display.clear();
}

void handlePushUpCounter() {
  // Generate pulse trigger
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Receive and convert time (us) to cm
  duration = pulseIn(echoPin, HIGH);
  d_cm = duration * 0.034 / 2.0;

  // Trigger body detection
  if (d_cm > 15 && d_cm <= 30) {
    trigUp = true;
  } else if (d_cm < 10) {
    trigDown = true;
  }

  // Counter
  if (trigUp == true && trigDown == true) {
    counterPushUp += 0.5;
    trigUp = false;
    trigDown = false;

    // Light up LED and sound buzzer
    digitalWrite(ledPin, HIGH);
    digitalWrite(buzzerPin, HIGH);
    delay(100); // Buzzer and LED on duration
    digitalWrite(ledPin, LOW);
    digitalWrite(buzzerPin, LOW);
  }

  // Display push-up count
  display.showNumberDecEx((int)counterPushUp, false, true, 4, 0);
}

void handleTimerMode() {
  if (digitalRead(incTimePin) == LOW) {
    timerCountdown += 1;
    display.showNumberDecEx(timerCountdown, false, true, 4, 0);
    delay(200); // Debounce delay
  }

  if (digitalRead(decTimePin) == LOW && timerCountdown > 0) {
    timerCountdown -= 1;
    display.showNumberDecEx(timerCountdown, false, true, 4, 0);
    delay(200); // Debounce delay
  }

  if (digitalRead(startTimerPin) == LOW && !timerRunning && timerCountdown > 0) {
    timerRunning = true;
    delay(200); // Debounce delay
  }

  if (timerRunning) {
    if (timerCountdown > 0) {
      delay(1000);
      timerCountdown -= 1;
      display.showNumberDecEx(timerCountdown, false, true, 4, 0);
    } else {
      timerRunning = false;
      digitalWrite(buzzerPin, HIGH); // Sound buzzer when countdown ends
    }
  }
}