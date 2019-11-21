#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define JOY_X 2
#define JOY_Y 1
#define JOY_BTN 12

enum sensor {POWER_A = 6, POWER_B = 7};
enum digital {DEV_A = 9, DEV_B = 10};
enum mode {ACTIVE, PASSIVE, OPTIONS};


class Device {
  public:
    void powerOn() {
      digitalWrite(devicePort, true);
    }

    void powerOff() {
      digitalWrite(devicePort, false);
    }

    bool isPowerOn() {
      bool result = false;
      for (int i = 0; i < tryCount; ++i) {
        int value = analogRead(powerPort);
        if (value < (middlePower - deviationPower) || value > (middlePower + deviationPower))result = true;
      }
      return result;
    }

    Device(int devicePort, int powerPort): devicePort(devicePort), powerPort(powerPort) {
      pinMode(devicePort, OUTPUT);
      powerOff();
      delay(300);
      this->middlePower = analogRead(powerPort);
    }

    Device() {}

  private:
    int tryCount = 40;
    int middlePower;
    int deviationPower = 10;
    int devicePort;
    int powerPort;
};

class Joystic {

  public:
    enum joystic {MIDDLE, UP, DOWN, RIGHT, LEFT};
    

    Joystic() {}
    Joystic(int joyPortX, int joyPortY, int joyPortButton): joyPortX(joyPortX), joyPortY(joyPortY), joyPortButton(joyPortButton) {
      pinMode(joyPortButton, INPUT_PULLUP);
      buttonValue = FREE;
      joysticMiddleX = analogRead(joyPortX);
      joysticMiddleY = analogRead(joyPortY);
    }

    bool isButtonRelease() {
      bool result = false;
      int state = !digitalRead(joyPortButton);
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

  private:
    enum buttValue {FREE, PRESS, RELEASE};
    int joysticMiddleX;
    int joysticMiddleY;
    int jDeviation = 100;
    int joyPortX;
    int joyPortY;
    int joyPortButton;
    int buttonValue;
};


float voltageDivisor = 24.55;
float voltageDeviation = 0;
int voltagePort = 0;
float upperVoltage = 28.2;
float lowerVoltage = 24.0;
int mode = PASSIVE;
const int ARRSIZE = 250;
float voltArray[ARRSIZE];
int voltArrayPosition = 0;
unsigned long long timer = millis();
const char * anim = "\xa5"":'";
int frameNum = 0;
Device devA;
Device devB;
Device * activeDevice = 0;
Device * passiveDevice = 0;

Joystic joy;

void activeMode();
void passiveMode();
void optionsMode();
void switching();
void voltageTest();
void showInfo();
float getVoltage(int port);
void tuning(const char* text, float &parametr, float minimum, float maximum, float tStep);
bool isSave();
bool timeLeft(int milliseconds);
void animate();

/////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  //Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");

  devA = Device(DEV_A, POWER_A);
  devB = Device(DEV_B, POWER_B);
  activeDevice = &devA;
  passiveDevice = &devB;

  joy = Joystic(JOY_X, JOY_Y, JOY_BTN);

  for (int i = 0; i < ARRSIZE; ++i) voltArray[i] = analogRead(voltagePort) / voltageDivisor;

  lcd.print("Ok");
  //lcd.setCursor(0, 1); lcd.print("middlePower=");
  delay(1000);
}
///////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  if (timeLeft(1000)) {
    showInfo();
    animate();
    if (mode == ACTIVE) activeMode();
  }
  //Serial.print(a); Serial.print("..\n");
  if (mode != OPTIONS) voltageTest();
  if (mode == PASSIVE) passiveMode();

  if (mode == OPTIONS) optionsMode();
  if (joy.isButtonRelease()) mode = OPTIONS;
  delay(5);
}

float getVoltage(int port) {
  float result;
  voltArray[voltArrayPosition] = analogRead(port) / voltageDivisor + voltageDeviation;
  //Serial.print(voltArray[voltArrayPosition]);Serial.print("\n");
  voltArrayPosition++;
  if (voltArrayPosition == ARRSIZE)voltArrayPosition = 0;
  for (int i = 0; i < ARRSIZE; ++i) {
    result += voltArray[i];
  }
  result = result / ARRSIZE;
  result = float(int(result * 10)) / 10;
  return result;
}


void voltageTest() {
  float voltage = getVoltage(voltagePort);
  if (voltage >= upperVoltage) {
    activeDevice -> powerOn();
    mode = ACTIVE;
    //delay(300);
  }
  if (voltage <= lowerVoltage) {
    mode = PASSIVE;
    activeDevice -> powerOff();
    passiveDevice -> powerOff(); //Just in case!
    Device * activeDevice = &devA;
    Device * passiveDevice = &devB;
    //delay(300);
  }
}


void activeMode() {
  if (!activeDevice -> isPowerOn()) switching();
}

void switching() {
  activeDevice -> powerOff();
  if (activeDevice == &devA) {
    activeDevice = &devB;
    passiveDevice = &devA;
  } else {
    activeDevice = &devA;
    passiveDevice = &devB;
  }
}

void passiveMode() {}

void optionsMode() {
  activeDevice -> powerOff();
  float newUpperVoltage = upperVoltage;
  float newLowerVoltage = lowerVoltage;
  float newvoltageDeviation = voltageDeviation;

  tuning("Upper_U", newUpperVoltage, 26.0, 29.0, 0.1);

  tuning("Lower_U", newLowerVoltage, 20.0, 26.0, 0.1);

  tuning("Divisor", newvoltageDeviation, -5.0, 5.0, 0.05);

  if (isSave()) {
    upperVoltage = newUpperVoltage;
    lowerVoltage = newLowerVoltage;
    voltageDeviation = newvoltageDeviation;
  }
  mode = PASSIVE;
}


void showInfo() {
  float voltage = getVoltage(voltagePort);
  //Serial.print(voltage);
  //lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("U=");
  lcd.print(voltage);
  lcd.print("v ");
  lcd.setCursor(9, 0);
  if (mode == ACTIVE && activeDevice == &devA) lcd.print("Dev.A  ");
  if (mode == ACTIVE && activeDevice == &devB) lcd.print("Dev.B  ");
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
      if (joy.getX() == Joystic::RIGHT) {
        parametr += tStep;
        break;
      }
      if (joy.getX() == Joystic::LEFT) {
        parametr -= tStep;
        break;
      }
      if (joy.isButtonRelease()) {
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
      if (joy.getX() != Joystic::MIDDLE || joy.getY() != Joystic::MIDDLE) {
        answer = !answer;
        break;
      }
      if (joy.isButtonRelease()) {
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



void animate() {
  char frame = *(anim + frameNum);
  //if (frameNum == 3) frame = '\x15';
  ++frameNum;

  if (frameNum > 1) frameNum = 0;
  lcd.setCursor(8, 0);
  lcd.print (frame);
}
void debug() {}
