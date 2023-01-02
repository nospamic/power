
#include "display.h"
#include "timer.h"
#include "joystic.h"



#define JOY_X 2
#define JOY_Y 1
#define JOY_BTN 9 //on PCB 9
#define VOLT_PORT 0 //on PCB 0


enum sensor {POWER_A = 6, POWER_B = 7};
enum digital {DEV_A = 6, DEV_B = 7};//on PCB 6,7
enum mode {ACTIVE, PASSIVE, OPTIONS, REQUIRED};


class Device {
  public:
    float powerValue;
    bool isRequired;
    bool isTurnedOn;
    
    void powerOn() {
      digitalWrite(devicePort, true);
      isTurnedOn = true;
      delay(300);
      this->setPower();
    }

    void powerOff() {
      digitalWrite(devicePort, false);
      isTurnedOn = false;
      this->powerValue = 0.0;
    }

    float getPower() {
      float maxValue = 0.0;
      int tryCount = 300;
      int currentValue = 0;
      for (int i = 0; i < tryCount; ++i) {
        currentValue = analogRead(powerPort) - this->middlePower;
        if (currentValue > maxValue) maxValue = currentValue;
      }
      return (round((maxValue/50.0)*10.0))/10.0;
    }

    bool hasLoad() {
      return (this->getPower() > this->minPower);
    }

    void setPower(){
      this->powerValue = getPower();
    }
    
    Device(int devicePort, int powerPort): devicePort(devicePort), powerPort(powerPort) {
      minPower = 0.15f;
      powerValue = 0.0f;
      isRequired = false;
      isTurnedOn = false;
      pinMode(devicePort, OUTPUT);
      powerOff();
      delay(300);
      this->middlePower = analogRead(powerPort);
    }

    Device() {}

  private:
    int middlePower;
    float minPower;
    int devicePort;
    int powerPort;
};



float voltageDivisor = 15.56;
float voltageDeviation = 0;
float upperVoltage = 56.4;
float lowerVoltage = 48.0;
int mode = PASSIVE;
int requiredTestCounter = 0;
Device devA;
Device devB;
Device * activeDevice = NULL;
Device * passiveDevice = NULL;
Timer timerInfo;
Timer timerAnimate;
Joystic joy;
Icons icons;

void activeMode();
void passiveMode();
void optionsMode();
void switching();
void voltageTest();
void requiredTest();
void setRequiredActive();
void batteryDetect();
void showInfo();
float getVoltage(int port);
void tuning(const char* text, float &parametr, float minimum, float maximum, float tStep);
Device* selectDevice(const char*text);
bool isSave();

/////////////////////////////////////////  SETUP  ////////////////////////////////////////////////////
void setup() {
  analogReference(EXTERNAL);
  //Serial.begin(9600);
  display::prepare();
  icons.prepare();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  devA = Device(DEV_A, POWER_A);
  devB = Device(DEV_B, POWER_B);
  activeDevice = &devA;
  passiveDevice = &devB;
  joy = Joystic(JOY_X, JOY_Y, JOY_BTN);
  lcd.print("Ok");
  delay(1000);
  batteryDetect();
  delay(1500);
  lcd.clear();
}

//////////////////////////////////////////  LOOP  //////////////////////////////////////////////
void loop() {
  if (timerInfo.timeLeft(1000)) {
    showInfo();
    if (mode == ACTIVE) activeMode();
    if (mode == PASSIVE) passiveMode();
  }
  //Serial.print(a); Serial.print("..\n");
  if (mode != OPTIONS) voltageTest();
  if (mode == OPTIONS) optionsMode();
  if (joy.isButtonRelease()) mode = OPTIONS;
  if (timerAnimate.timeLeft(500)) {
    icons.animate(7, 0);
  }
  delay(5);
}


float getVoltage(int port) {
  float result = analogRead(port) / voltageDivisor + voltageDeviation;
  return float(round(result * 10.0)) / 10.0f;
}


void batteryDetect(){
  float voltage = getVoltage(VOLT_PORT);
  lcd.clear();
  if (voltage <= 15.0 && voltage >= 2.0){
    upperVoltage = 14.1;
    lowerVoltage = 12.0;
    lcd.print("Battery: 12v");
  }
  if (voltage <= 32.0 && voltage > 15.0){
    upperVoltage = 28.2;
    lowerVoltage = 24.0;
    lcd.print("Battery: 24v");
  }
  if (voltage > 32.0){
    upperVoltage = 56.4;
    lowerVoltage = 48.0;
    lcd.print("Battery: 48v");
  }
  if (voltage < 2.0){
    lcd.print("No battery!");
    lcd.setCursor(0, 1);
    lcd.print("Connect & restart...");
  }
   
}


void voltageTest() {
  float voltage = getVoltage(VOLT_PORT);
  if (voltage >= upperVoltage) {
    passiveDevice -> powerOff();
    activeDevice -> powerOn();
    mode = ACTIVE;
    return;
  }
  if (voltage < lowerVoltage && mode == ACTIVE) {
    activeDevice -> powerOff();
    passiveDevice -> powerOff(); //Just in case!
    mode = PASSIVE;
    setRequiredActive();
  }  
}


void setRequiredActive(){
  if(devB.isRequired){         //required device forever first
      activeDevice = &devB;
      passiveDevice = &devA;
    }else{
      activeDevice = &devA;
      passiveDevice = &devB;
    }  
}

  
void activeMode() { ////////////////////////////////////////////////   ACTIVE    //////////////////////////////////////////////
  if (passiveDevice->isRequired) ++requiredTestCounter;
  if (requiredTestCounter > 120){ //1 min = 120 requiredTestCounter
      requiredTestCounter = 0;
      switching();
  }
  if (!activeDevice -> hasLoad()) switching();
}


void switching() {               //active mode only
  activeDevice->powerOff();
  passiveDevice->powerOff();
  if (activeDevice == &devA) {
    activeDevice = &devB;
    passiveDevice = &devA;
  } else {
    activeDevice = &devA;
    passiveDevice = &devB;
  }
  activeDevice -> powerOn();
}

void passiveMode() {            ////////////////////////////////   PASSIVE /////////////////////////////////////////////////////
  setRequiredActive();
  if(activeDevice->isRequired && activeDevice->isTurnedOn == false){
    activeDevice->powerOn();
  }
}

void optionsMode() {
  activeDevice -> powerOff();
  float newUpperVoltage = upperVoltage;
  float newLowerVoltage = lowerVoltage;
  float newvoltageDeviation = voltageDeviation;

  tuning("Upper_U", newUpperVoltage, upperVoltage - 10, upperVoltage + 10, 0.1);

  tuning("Lower_U", newLowerVoltage, lowerVoltage - 10, lowerVoltage + 10, 0.1);

  tuning("Correct", newvoltageDeviation, -5.0, 5.0, 0.05);

  Device* requiredDevice = selectDevice("Required:");

  if (isSave()) {
    upperVoltage = newUpperVoltage;
    lowerVoltage = newLowerVoltage;
    voltageDeviation = newvoltageDeviation;
    devA.isRequired = false;
    devB.isRequired = false;
    if (requiredDevice != NULL)requiredDevice->isRequired = true;
  }
  mode = PASSIVE;
  lcd.clear();
}


void showInfo() {
  float voltage = getVoltage(VOLT_PORT);
  //debug();
  //Serial.print(voltage);Serial.print("\n");
  //lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("U=");
  lcd.print(int(voltage));
  lcd.print(".");
  lcd.print(int((voltage - int(voltage))*10));
  lcd.print(" ");
  lcd.setCursor(9, 0);
  if (mode == ACTIVE && activeDevice == &devA) lcd.print(" A");
  if (mode == ACTIVE && activeDevice == &devB) lcd.print(" B");
  if (mode == ACTIVE) {
    lcd.print(int(activeDevice->powerValue));
    lcd.print(".");
    lcd.print(int((activeDevice->powerValue - int(activeDevice->powerValue)) * 10));
    lcd.print("kW");
  }
  if (mode == PASSIVE)lcd.print("Passive ");
  if (mode == REQUIRED)lcd.print("Require ");
  lcd.setCursor(0, 1);
  lcd.print("Press to options");//("Button ""\x7e"" options");
  delay(5);
}

void tuning(const char* text, float &parametr, float minimum, float maximum, float tStep) {

  bool isExit = false;
  lcd.clear();
  lcd.print("Options: ");
  lcd.print(text);
  while (!isExit) {
    lcd.setCursor(5, 1);
    lcd.print(" ");
    lcd.print(parametr);
    lcd.print(" ");
    delay(200);
    lcd.setCursor(1, 1);
    lcd.print(" ");
    icons.leftArrow(2, 1);
    icons.rightArrow(13, 1);
    lcd.print(" ");
    while (true) {
      if (joy.getX() == Joystic::RIGHT) {
        parametr += tStep;
        icons.rightArrow(14, 1);
        break;
      }
      if (joy.getX() == Joystic::LEFT) {
        parametr -= tStep;
        icons.leftArrow(1, 1);
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

Device* selectDevice(const char* text) {
  enum items{EMPTY, DEVA, DEVB};
  int answer = EMPTY;
  if(devA.isRequired) answer = DEVA;
  if(devB.isRequired) answer = DEVB;
  bool isExit = false;
  while (!isExit) {
    lcd.clear();
    lcd.print(text);
    if (answer > DEVB) answer = EMPTY;
    if (answer == EMPTY) {
      lcd.setCursor(0, 1); lcd.print("< Empty >  ");
    }
    if (answer == DEVA) {
      lcd.setCursor(0, 1); lcd.print("< Dev. A > ");
    }
    if (answer == DEVB) {
      lcd.setCursor(0, 1); lcd.print("< Dev. B > ");
    }
    delay(400);
    while (true) {
      if (joy.getX() != Joystic::MIDDLE || joy.getY() != Joystic::MIDDLE) {
        ++answer;
        break;
      }
      if (joy.isButtonRelease()) {
        isExit = true;
        break;
      }
      delay(5);
    }
  }
  Device *result;
  if(answer == DEVA) result = &devA;
  if(answer == DEVB) result = &devB;
  if(answer == EMPTY) result = NULL;
  return result;
}



void debug() {
  for(int i=0; i<=7;++i){
    Serial.print(i); Serial.print("-");Serial.print(analogRead(i));Serial.print("   ");
  }
  Serial.print("\n");
  }
