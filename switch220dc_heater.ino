#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define JOY_X 2
#define JOY_Y 1
#define JOY_BTN 9 //on PCB 9
#define UP_BTN 2
#define DOWN_BTN 4
#define VOLT_PORT 0 //on PCB 0


enum sensor {POWER_A = 6, POWER_B = 7};
enum digital {DEV_A = 6, DEV_B = 7};//on PCB 6,7
enum mode {ACTIVE, PASSIVE, OPTIONS};


class Device {
  public:
    float powerValue = 0.0;
    
    void powerOn() {
      delay(200);
      digitalWrite(devicePort, true);
    }

    void powerOff() {
      digitalWrite(devicePort, false);
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

    bool isPowerOn() {
      return (this->getPower() > this->minPower);
    }

    void setPower(){
      this->powerValue = getPower();
    }
    
    Device(int devicePort, int powerPort): devicePort(devicePort), powerPort(powerPort) {
      pinMode(devicePort, OUTPUT);
      powerOff();
      delay(300);
      this->middlePower = analogRead(powerPort);
    }

    Device() {}

  private:
    int middlePower;
    float minPower = 0.15;
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



    bool isUp(){
      int state = digitalRead(UP_BTN);
      return (state == HIGH);
      }

    bool isDown(){
      int state = digitalRead(DOWN_BTN);
      return (state == HIGH);
      }
      
  private:
    enum buttValue {FREE, PRESS, RELEASE};
    int joysticMiddleX;
    int joysticMiddleY;
    int jDeviation = 200;
    int joyPortX;
    int joyPortY;
    int joyPortButton;
    int buttonValue;
    
};


float voltageDivisor = 3.84;
float voltageDeviation = 0;
float upperVoltage = 230.0;
float lowerVoltage = 5.0;
int mode = PASSIVE;
float dayPower = 0;
const int ARRSIZE = 2;//capacity
float voltArray[ARRSIZE];
int voltArrayPosition = 0;
const int diodePort = 13;
unsigned long long timer = millis();
const char * anim = "\xa5"":'";
int frameNum = 0;
Device net;


Joystic joy;

void activeMode();
void passiveMode();
void optionsMode();
void switching();
void voltageTest();
void batteryDetect();
void showInfo();
float getVoltage(int port);
void tuning(const char* text, float &parametr, float minimum, float maximum, float tStep);
bool isSave();
bool timeLeft(int milliseconds);
void animate();
void diodeOn(bool);

///////////////////////////////////////  SETUP  //////////////////////////////////////////////
void setup() {
  analogReference(EXTERNAL);
  pinMode(diodePort, OUTPUT);
  //Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");

  net = Device(DEV_A, POWER_A);
  
  

  joy = Joystic(JOY_X, JOY_Y, JOY_BTN);

  for (int i = 0; i < ARRSIZE; ++i) voltArray[i] = analogRead(VOLT_PORT) / voltageDivisor;
  
  lcd.print("Ok");
  delay(1000);
  lcd.clear();
  mode = PASSIVE;
}
///////////////////////////////////////   LOOP   ////////////////////////////////////////////////
void loop() {
  if (timeLeft(490)) {
    showInfo();
    animate();
    
  }
  //Serial.print(a); Serial.print("..\n");
  if (mode != OPTIONS) voltageTest();
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
  result = result / float(ARRSIZE);
  result = round(result);
  return result;
}


void voltageTest() {
  float voltage = getVoltage(VOLT_PORT);
  if (voltage > upperVoltage && mode == PASSIVE) {
    net.powerOn();
    mode = ACTIVE;
    diodeOn(true);
    delay(2000);
    lcd.clear();
  }
  if (voltage < lowerVoltage && mode == ACTIVE) {
    net.powerOff();
    mode = PASSIVE;
    diodeOn(false);
    delay(10000);
    dayPower = 0;
  }
}


void activeMode() {}


void passiveMode() {}

void optionsMode() {
  net.powerOff();
  float newUpperVoltage = upperVoltage;
  float newLowerVoltage = lowerVoltage;
 
  tuning("Upper_U", newUpperVoltage, upperVoltage - 100, upperVoltage + 100, 0.1);

  tuning("Lower_U", newLowerVoltage, lowerVoltage - 20, lowerVoltage + 20, 0.1);

  if (isSave()) {
    upperVoltage = newUpperVoltage;
    lowerVoltage = newLowerVoltage;
    
  }
  mode = PASSIVE;
}


void showInfo() {
  float voltage = getVoltage(VOLT_PORT);
  //lcd.clear();
  //lcd.print(joy.isUp());
  //lcd.print(" ");
  //lcd.print(joy.isDown());
  lcd.setCursor(0, 0);
  lcd.print("U=");
  lcd.print(int(voltage));
  lcd.print("v ");
  
  if (mode == ACTIVE){
    lcd.setCursor(9, 0);
    lcd.print(" ACTIVE ");
    lcd.setCursor(0, 1);
    float currentPower = voltage*voltage/15; //watt
    lcd.print("P=");
    lcd.print((int(currentPower/1000*100))/100.0);
    lcd.print("kW ");
    lcd.setCursor(9, 1);
    lcd.print("(");
    dayPower += (currentPower/7200); //watt
    lcd.print((int(dayPower/1000*100))/100.0);
    lcd.print(") ");
    
  }
  if (mode == PASSIVE){
    lcd.setCursor(9, 0);
    lcd.print(" ------ ");
    lcd.setCursor(0, 1);
    lcd.print("Last kW:");
  }
  
  
  
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
    long int exitCounter = 0;
    long int exitMaximum = 6000;
    while (true) {
      ++exitCounter;
      if (joy.isUp()) {
        parametr += tStep;
        break;
      }
      if (joy.isDown()) {
        parametr -= tStep;
        break;
      }
      if (joy.isButtonRelease() || exitCounter > exitMaximum) {
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
    long int exitCounter = 0;
    long int exitMaximum = 6000;
    while (true) {
      ++exitCounter;
      if (joy.isUp() || joy.isDown()) {
        answer = !answer;
        break;
      }
      if (joy.isButtonRelease() || exitCounter > exitMaximum) {
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

void diodeOn(bool isOn){
  if(isOn){
    digitalWrite(diodePort, HIGH);
  }
  else{
    digitalWrite(diodePort, LOW);  
  }
}
void debug() {
  for(int i=0; i<=7;++i){
    Serial.print(i); Serial.print("-");Serial.print(analogRead(i));Serial.print("   ");
  }
  Serial.print("\n");
  }
