#include <LiquidCrystal_I2C.h>
#include <Wire.h>



LiquidCrystal_I2C lcd(0x27, 16, 2);
enum joystic {MIDDLE, UP, DOWN, RIGHT, LEFT};
enum sensor {POWER_A = 6, POWER_B = 7};
enum digital {DEV_A = 9, DEV_B = 10};
enum mode {ACTIVE, PASSIVE, OPTIONS};
enum buttValue {FREE, PRESS, RELEASE};

float voltageDivisor = 24.55;
int tryCount = 40;
int middlePower;
int joysticMiddleX;
int joysticMiddleY;
int jDeviation = 100;
int deviationPower = 10;
int voltagePort = 0;
int joyPortX = 2;
int joyPortY = 1;
float upperVoltage = 28.2;
float lowerVoltage = 24.0;
int activeDevice = DEV_A;
int activeSensor = POWER_A;
int mode = PASSIVE;
int buttonValue = FREE;
const int ARRSIZE = 250;
float voltArray[ARRSIZE];
int voltArrayPosition = 0;
unsigned long long timer = millis();
const char * anim = "\xa5"":'";
int frameNum = 0;

void activeMode();
void passiveMode();
void optionsMode();
void deviceOn(int port, bool state);
void switching();
void voltageTest();
void showInfo();
float getVoltage(int port);
bool isButtonRelease();
int getX();
int getY();
bool isPowerOn(int port);
void tuning(const char* text, float &parametr, float minimum, float maximum, float tStep);
bool isSave();
bool timeLeft(int milliseconds);
void animate();



void setup() {
  //Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");

  pinMode(12, INPUT_PULLUP);
  pinMode(DEV_A, OUTPUT);
  pinMode(DEV_B, OUTPUT);

  for (int i = 0; i < ARRSIZE; ++i) voltArray[i] = analogRead(voltagePort) / voltageDivisor;

  deviceOn(DEV_A, false);
  deviceOn(DEV_B, false);
  delay(600);
  middlePower = analogRead(POWER_A);

  joysticMiddleX = analogRead(joyPortX);
  joysticMiddleY = analogRead(joyPortY);

  lcd.print("Ok");
  lcd.setCursor(0, 1);
  lcd.print("middlePower=");
  lcd.print(middlePower);
  delay(1500);
}

void loop() {
  if (timeLeft(1000)){ 
    showInfo();
    animate();
  }
  
  //Serial.print(a);Serial.print("..\n");
  if (mode != OPTIONS) voltageTest();
  if (mode == PASSIVE) passiveMode();
  if (mode == ACTIVE) activeMode();
  if (mode == OPTIONS) optionsMode();
  if (isButtonRelease()) mode = OPTIONS;
  delay(20);
}

float getVoltage(int port) {
  float result;
  voltArray[voltArrayPosition] = analogRead(port) / voltageDivisor;
  voltArrayPosition++;
  if (voltArrayPosition == ARRSIZE)voltArrayPosition = 0;
  for (int i = 0; i < ARRSIZE; ++i) {
    result += voltArray[i];
  }
  result = result / ARRSIZE;
  result = float(int(result * 10)) / 10;
  return result;
}


bool isButtonRelease() {
  bool result = false;
  int state = !digitalRead(12);
  if (buttonValue == FREE && state == HIGH) buttonValue = PRESS;
  if (buttonValue == PRESS && state == LOW) {
    buttonValue = FREE;
    result = true;
  }
  return result;
}


int getY() {
  int result;
  int y = analogRead(joyPortY);
  if (y < joysticMiddleY - jDeviation) result = UP;
  if (y > joysticMiddleY + jDeviation) result = DOWN;
  if (y >= joysticMiddleY - jDeviation && y <= joysticMiddleY + jDeviation)result = MIDDLE;
  return result;
}

int getX() {
  int result;
  int x = analogRead(joyPortX);
  if (x < joysticMiddleY - jDeviation) result = RIGHT;
  if (x > joysticMiddleY + jDeviation) result = LEFT;
  if (x >= joysticMiddleY - jDeviation && x <= joysticMiddleY + jDeviation) result = MIDDLE;
  return result;
}

bool isPowerOn(int port) {
  delay(300);
  bool result = false;
  for (int i = 0; i < tryCount; ++i) {
    int value = analogRead(port);
    if (value < (middlePower - deviationPower) || value > (middlePower + deviationPower))result = true;
  }
  //lcd.clear();result?lcd.print("isPowerON"):lcd.print("isPowerOFF");delay(1000);
  return result;
}


void voltageTest() {
  float voltage = getVoltage(voltagePort);
  if (voltage >= upperVoltage) {
    deviceOn(activeDevice, true);
    mode = ACTIVE;
    //lcd.clear();activeDevice == DEV_A?lcd.print("DEV_A - on"):lcd.print("DEV_B - on");
    delay(300);
  }
  if (voltage <= lowerVoltage) {
    mode = PASSIVE;
    deviceOn(DEV_A, false);
    deviceOn(DEV_B, false);
    activeDevice = DEV_A;
    activeSensor = POWER_A;
    //lcd.clear();lcd.print("DEV_A_B - off");
    delay(300);
  }
}


void activeMode() {
  if (!isPowerOn(activeSensor)) switching();
}

void switching() {

  deviceOn(activeDevice, false);
  if (activeDevice == DEV_A) {
    activeDevice = DEV_B;
    activeSensor = POWER_B;
  } else {
    activeDevice = DEV_A;
    activeSensor = POWER_A;
  }
  //lcd.clear();
  //lcd.print("Switch to ");activeDevice == DEV_A?lcd.print("DEV_A"):lcd.print("DEV_B");
  //delay(1000);
}

void passiveMode() {}

void optionsMode() {
  deviceOn(activeDevice, false);
  float newUpperVoltage = upperVoltage;
  float newLowerVoltage = lowerVoltage;
  float newVoltageDivisor = voltageDivisor;

  tuning("Upper_U", newUpperVoltage, 26.0, 29.0, 0.1);
  //delay(500);
  tuning("Lower_U", newLowerVoltage, 20.0, 26.0, 0.1);
  //delay(500);
  tuning("Divisor", newVoltageDivisor, 15.0, 30.0, 0.05);
  //delay(400);
  //debug();
  //delay(300);
  if (isSave()) {
    upperVoltage = newUpperVoltage;
    lowerVoltage = newLowerVoltage;
    voltageDivisor = newVoltageDivisor;
  }
  mode = PASSIVE;
}

void deviceOn(int port, bool state) {
  digitalWrite(port, state);
}



void showInfo() {
  float voltage = getVoltage(voltagePort);
  //Serial.print(voltage);
  //lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("U=");
  lcd.print(voltage);
  lcd.print("v ");
  if (mode == ACTIVE && activeDevice == DEV_A) lcd.print("Dev.A  ");
  if (mode == ACTIVE && activeDevice == DEV_B) lcd.print("Dev.B  ");
  if (mode == PASSIVE)lcd.print("Passive ");
  lcd.setCursor(0, 1);
  lcd.print("Button ""\x7e"" options");
  delay(5);
}

void tuning(const char* text, float &parametr, float minimum, float maximum, float tStep) {

  bool isExit = false;
  while (!isExit) {
    lcd.clear();
    lcd.print("Options: ");
    lcd.print(text);
    lcd.setCursor(0, 1);
    lcd.print("\x7f""(-) ");
    lcd.print(parametr);
    lcd.print(" (+)""\x7e");
    delay(200);
    while (true) {
      if (getX() == RIGHT) {
        parametr += tStep;
        break;
      }
      if (getX() == LEFT) {
        parametr -= tStep;
        break;
      }
      if (isButtonRelease()){
        isExit = true;
        break;
      }
    delay(5);
    }
    if (parametr < minimum) parametr += tStep;
    if (parametr > maximum) parametr -= tStep;
  }
}

bool isSave() {
  bool answer = false;
  bool isExit = false;
  while (!isExit) {
    lcd.clear();
    lcd.print("Save?");
    if (!answer) {
      lcd.setCursor(7, 0); lcd.print("< No > ");
    }
    if (answer) {
      lcd.setCursor(7, 0); lcd.print("< Yes > ");
    }
    delay(400);
    while (true) {
      if (getX() != MIDDLE || getY() != MIDDLE) {
        answer = !answer;
        break;
      }
      if (isButtonRelease()) {
        isExit = true;
        break;
      }
      delay(5);
    }
  }
  return answer;
}


bool timeLeft(int milliseconds) {
  bool result = false;
  if (millis() > (timer + milliseconds)) {
    result = true;
    timer = millis();
  }
  if (millis() < 5000) timer = millis();
  return result;
}



void animate(){
  char frame = *(anim + frameNum);
  //if (frameNum == 3) frame = '\x15';
  ++frameNum;
  
  if (frameNum > 1) frameNum = 0;
  lcd.setCursor(8,0);
  lcd.print (frame);
}
void debug() {}
