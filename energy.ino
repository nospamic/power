#include <LiquidCrystal_I2C.h>
#include <Wire.h> 

LiquidCrystal_I2C lcd(0x27,16,2);
enum joystic{MIDDLE, UP, DOWN, RIGHT, LEFT};
enum sensor{POWER_A = 6, POWER_B = 7};
enum digital{DEV_A = 9, DEV_B = 10};
enum mode{ACTIVE, PASSIVE, OPTIONS};

float voltageDivisor = 22.53;
int tryCount = 40;
int middlePower;
int deviationPower = 10;
int voltagePort = 0;
float upperVoltage = 28.2;
float lowerVoltage = 24.0;
int activeDevice = DEV_A;
int activeSensor = POWER_A;
int mode = PASSIVE;

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


void setup() {
  lcd.init();                      
  lcd.backlight();                 
  lcd.setCursor(0,0);             
  lcd.print("Starting...");  

  pinMode(12,INPUT_PULLUP);
  pinMode(DEV_A, OUTPUT);
  pinMode(DEV_B, OUTPUT);

  deviceOn(DEV_A, false);
  deviceOn(DEV_B, false);
  delay(600);
  middlePower = analogRead(POWER_A);
  
  lcd.print("Ok");
  lcd.setCursor(0,1);
  lcd.print("middlePower=");
  lcd.print(middlePower);
  delay(600);
}

void loop() {
  showInfo();
  if(mode != OPTIONS) voltageTest(); 
  if(mode == PASSIVE) passiveMode();
  if(mode == ACTIVE) activeMode();
  if(mode == OPTIONS) optionsMode();
  
  
  
  
  //if(isButton()) {lcd.print("X");}
  //lcd.print(getY());
  //lcd.print("A-");
  //if(isPowerOn(POWER_A))lcd.print("on");
  //lcd.print(" B-");
  //if(isPowerOn(POWER_B))lcd.print("on");
  delay(500);
}

float getVoltage(int port){
  return analogRead(port)/ voltageDivisor;
}

bool isButton(){
  bool result;
  if(!digitalRead(12)==HIGH){result=true;}else{result=false;}
  return result;  
  }

int getY(){
  int result;
  int y = analogRead(1);
  if(y<300)result=UP;
  if(y>600)result=DOWN;
  if(y>=300 && y<=500)result = MIDDLE;
  return result;
  }

int getX(){
  int result;
  int x = analogRead(2);
  if(x<300)result=RIGHT;
  if(x>500)result=LEFT;
  if(x>=300 && x<=500)result = MIDDLE;
  return result;
  }

bool isPowerOn(int port){
  bool result = false;
  for (int i=0; i<tryCount; ++i){
    int value = analogRead(port);
    if(value<(middlePower - deviationPower) || value>(middlePower + deviationPower))result = true;
  }
  return result;
}

void activeMode(){
  if(!isPowerOn(activeDevice)) switching();  
}

void passiveMode(){}

void optionsMode(){
  
  
  }
  
void deviceOn(int port, bool state){
  digitalWrite(port, state);
}

void switching(){
  deviceOn(activeDevice, false);
  if(activeDevice == DEV_A){
    activeDevice = DEV_B;
    activeSensor = POWER_B;
  }
  if(activeDevice == DEV_B){
    activeDevice = DEV_A;
    activeSensor = POWER_A;
  }
}

void voltageTest(){
  if(getVoltage(voltagePort) >= upperVoltage){
    mode = ACTIVE;
    deviceOn(activeDevice, true);
  }
  if(getVoltage(voltagePort) <= lowerVoltage){
    mode = PASSIVE;
    deviceOn(activeDevice, false); 
  }
}

void showInfo(){
  lcd.clear();
  lcd.setCursor(0,0); 
  lcd.print("U=");
  lcd.print(getVoltage(voltagePort));
  lcd.print("v ");
  if(mode == ACTIVE && activeDevice == DEV_A) lcd.print("Dev.A");
  if(mode == ACTIVE && activeDevice == DEV_B) lcd.print("Dev.B");
  if(mode == PASSIVE)lcd.print("Passive");
  lcd.setCursor(0,1);
  lcd.print("Button->options");
}
