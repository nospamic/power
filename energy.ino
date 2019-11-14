#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
enum joystic {MIDDLE, UP, DOWN, RIGHT, LEFT};
enum sensor {POWER_A = 6, POWER_B = 7};
enum digital {DEV_A = 9, DEV_B = 10};
enum mode {ACTIVE, PASSIVE, OPTIONS};

float voltageDivisor = 24.80;
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
int buttonRepeat = 20;
const int ARRSIZE = 50;
float voltArray[ARRSIZE];
int voltArrayPosition = 0;

void activeMode();
void passiveMode();
void optionsMode();
void deviceOn(int port, bool state);
void switching();
void voltageTest();
void showInfo();
float getVoltage(int port);
bool isButton();
int getX();
int getY();
bool isPowerOn(int port);
void tuning(const char* text, float &parametr, float minimum, float maximum, float tStep);
bool isSave();
void debug();


void setup() {
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");

  pinMode(12, INPUT_PULLUP);
  pinMode(DEV_A, OUTPUT);
  pinMode(DEV_B, OUTPUT);

  for(int i=0; i<ARRSIZE; ++i) voltArray[i] = analogRead(voltagePort)/voltageDivisor;

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
  showInfo();
  if (mode != OPTIONS) voltageTest();
  if (mode == PASSIVE) passiveMode();
  if (mode == ACTIVE) activeMode();
  if (mode == OPTIONS) optionsMode();

  for (int i = 0; i < buttonRepeat; ++i) {
    if (isButton())mode = OPTIONS;
    delay(20);
  }
}

float getVoltage(int port) {
  float result;
  voltArray[voltArrayPosition] = analogRead(port)/ voltageDivisor;
  voltArrayPosition++;
  if(voltArrayPosition == ARRSIZE)voltArrayPosition = 0;
  for(int i=0; i<ARRSIZE; ++i){
    result += voltArray[i];
  }
  return result/ARRSIZE;
}

bool isButton() {
  bool result;
  if (!digitalRead(12) == HIGH) {
    result = true;
  } else {
    result = false;
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
  }else{
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
  delay(400);
  tuning("Lower_U", newLowerVoltage, 20.0, 26.0, 0.1);
  delay(400);
  tuning("Divisor", newVoltageDivisor, 15.0, 30.0, 0.05);
  //delay(400);
  //debug();
  delay(200);
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
  //lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("U=");
  lcd.print(voltage);
  lcd.print("v ");
  if (mode == ACTIVE && activeDevice == DEV_A) lcd.print("Dev.A  ");
  if (mode == ACTIVE && activeDevice == DEV_B) lcd.print("Dev.B  ");
  if (mode == PASSIVE)lcd.print("Passive ");
  lcd.setCursor(0, 1);
  lcd.print("Button->options");
  delay(10);
}

void tuning(const char* text, float &parametr, float minimum, float maximum, float tStep) {
  while (!isButton()) {
    lcd.clear();
    lcd.print("Options: ");
    lcd.print(text);
    lcd.setCursor(0, 1);
    lcd.print("<(-) ");
    lcd.print(parametr);
    lcd.print(" (+)>");
    delay(200);
    while (!isButton()) {
      if (getX() == RIGHT) {
        parametr += tStep;
        break;
      }
      if (getX() == LEFT) {
        parametr -= tStep;
        break;
      }
      delay(50);
    }
    if (parametr < minimum) parametr += tStep;
    if (parametr > maximum) parametr -= tStep;
  }
}

bool isSave() {
  bool answer = false;
  while (!isButton()) {
    lcd.clear();
    lcd.print("Save?");
    if (!answer) {
      lcd.setCursor(7, 0); lcd.print("< No > ");
    }
    if (answer) {
      lcd.setCursor(7, 0); lcd.print("< Yes > ");
    }
    delay(400);
    while (!isButton()) {
      if (getX() != MIDDLE || getY() != MIDDLE) {
        answer = !answer;
        break;
      }
      delay(50);
    }
  }
  delay(200);
  return answer;
}

void debug(){
  lcd.clear();
  deviceOn(DEV_A, true); deviceOn(DEV_B, false);
  delay(300);
  if(isPowerOn(POWER_A))lcd.print("POWER_A");
  deviceOn(DEV_B, true); deviceOn(DEV_A, false);
  delay(300);
  if(isPowerOn(POWER_B))lcd.print("POWER_B");
  
  while(!isButton()){
    lcd.setCursor(0,1);  
    lcd.print("Vp-");
    lcd.print(analogRead(voltagePort));
   
    
    delay(400);
  }
}
