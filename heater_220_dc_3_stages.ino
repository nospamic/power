#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);


#define VOLT_PORT 0 //on PCB 0
#define WATER_TEMP_PORT 6 //on PCB Power_A
#define WATER_TEMP_BUS 8 //D8

enum digital {DEV_A = 8, DEV_B = 7, DEV_C = 9, CONTACTOR = 3};//on PCB 6,7, (9, 3)joystic(93...)
enum modes {PASSIVE, STAGE_ONE, STAGE_TWO, STAGE_THREE};
OneWire oneWire(WATER_TEMP_BUS);
DallasTemperature sensor(&oneWire);

void setOptimalMode();
void setMode(int mode);
void voltageTest();
void showInfo();
float getVoltage();
float getCurrentPower();
void setCurrentTemp();
bool timeLeft(int milliseconds);
void diodeOn(bool);

class Heater {
  public:
    
    void turnOn() {
      digitalWrite(devicePort, true);
      turnedOn = true;
    }

    void turnOff() {
      digitalWrite(devicePort, false);
      turnedOn = false;
    }

    float getPower() {
      float voltage = getVoltage();
      return voltage * voltage / resistance;
    }

    bool isTurnedOn(){
      return turnedOn;  
    }
    Heater(int devicePort, float resistance): devicePort(devicePort), resistance(resistance) {
      pinMode(devicePort, OUTPUT);
      turnOff();
    }

    Heater() {}

  private:
    int devicePort;
    float resistance;
    bool turnedOn;
};

class Contactor{
public:
  Contactor(){}
  Contactor(int devicePort) : devicePort(devicePort)
  {
    pinMode(devicePort, OUTPUT);
    off();
  }
  void on(){
      delay(200);
      digitalWrite(devicePort, true);
  }
  void off(){
    digitalWrite(devicePort, false);
    delay(5000);
  }
private:
  int devicePort;
};

const float resist_1 = 32.0f;
const float resist_2 = 32.0f;
const float resist_3 = 32.0f;
const float maxWaterTemp = 55.0f;
float currentTemp = 0.0f;
float voltageDivisor = 3.84f;
float voltageDeviation = 0.0f;
float lastVoltage;
float upperVoltage = 230.0f;
float lowerVoltage = 5.0f;
int currentMode = PASSIVE;
float dayPower = 0;
const int ARRSIZE = 2;//capacity
float voltArray[ARRSIZE];
int voltArrayPosition = 0;
const int diodePort = 13;
unsigned long long timer = millis();
Heater heater_1;
Heater heater_2;
Heater heater_3;
Contactor contactor;






///////////////////////////////////////  SETUP  //////////////////////////////////////////////
void setup() {
  analogReference(EXTERNAL);
  pinMode(diodePort, OUTPUT);
  sensor.begin();
  //Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  heater_1 = Heater(DEV_A, resist_1);
  heater_2 = Heater(DEV_B, resist_2);
  heater_3 = Heater(DEV_C, resist_3);
  contactor = Contactor(CONTACTOR);
  
  for (int i = 0; i < ARRSIZE; ++i) voltArray[i] = analogRead(VOLT_PORT) / voltageDivisor;
  
  lcd.print("Ok");
  delay(1000);
  lcd.clear();
  currentMode = PASSIVE;
  lastVoltage = getVoltage();
  setOptimalMode();
}


///////////////////////////////////////   LOOP   ////////////////////////////////////////////////
void loop() {
  if (timeLeft(490)) {
    showInfo();
    setCurrentTemp();
  }
  if(abs(lastVoltage - getVoltage()) > 25.0){
    setOptimalMode();
    delay(100);
    lastVoltage = getVoltage();
  }
  delay(5);
}

float getVoltage() {
  float result;
  voltArray[voltArrayPosition] = analogRead(VOLT_PORT) / voltageDivisor + voltageDeviation;
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


float getCurrentPower(){
  float result = 0.0;
  if(heater_1.isTurnedOn()){
    result += heater_1.getPower();  
  }
  if(heater_2.isTurnedOn()){
    result += heater_2.getPower();  
  }
  if(heater_3.isTurnedOn()){
    result += heater_3.getPower();  
  }
  return result; 
}

void setMode(int mode){
  heater_1.turnOff();
  heater_2.turnOff();
  heater_3.turnOff();
  if(mode == PASSIVE){
    currentMode = PASSIVE;
    delay(300);
  }
  if(mode == STAGE_ONE){
    currentMode = STAGE_ONE;
    heater_1.turnOn();
    delay(300);
  }
  if(mode == STAGE_TWO){
    currentMode = STAGE_TWO;
    heater_1.turnOn();
    heater_2.turnOn();
    delay(300);
  }
  if(mode == STAGE_THREE){
    currentMode = STAGE_THREE;
    heater_1.turnOn();
    heater_2.turnOn();
    heater_3.turnOn();
    delay(300);
  }
}

void setOptimalMode(){
  lcd.setCursor(0, 0);
  lcd.print(" TEST ");
    setMode(PASSIVE);
    if(getVoltage() < upperVoltage || currentTemp > maxWaterTemp){
      contactor.off();
      return;
    }
    float maxPower = 0.0f;
    setMode(STAGE_ONE);
    if (getCurrentPower() > maxPower){
      maxPower = getCurrentPower();
      contactor.on();
    }else{
      setMode(PASSIVE);
      return;
    }
    setMode(STAGE_TWO);
    if (getCurrentPower() > maxPower){
      maxPower = getCurrentPower();
    }else{
      setMode(STAGE_ONE);
      return;
    }
    setMode(STAGE_THREE);
    if (getCurrentPower() > maxPower){
      return;
    }else{
      setMode(STAGE_TWO);
    }
}

void setCurrentTemp(){
  sensor.requestTemperatures();
  currentTemp = sensor.getTempCByIndex(0);
}

void showInfo() {
  float voltage = getVoltage();
  lcd.setCursor(0, 0);
  lcd.print("U=");
  lcd.print(int(voltage));
  lcd.print("v ");
  
  if (currentMode != PASSIVE){
    lcd.setCursor(9, 0);
    if(currentMode == STAGE_ONE) lcd.print(" (0..) ");
    if(currentMode == STAGE_TWO) lcd.print(" (00.) ");
    if(currentMode == STAGE_ONE) lcd.print(" (000) ");
    lcd.setCursor(0, 1);
    float currentPower = getCurrentPower(); //watt
    lcd.print("P=");
    lcd.print((int(currentPower/1000*100))/100.0);
    lcd.print("kW ");
    lcd.setCursor(9, 1);
    lcd.print("t=");
    lcd.print(currentTemp);
    dayPower += (currentPower/7200); //watt
  }
  if (currentMode == PASSIVE){
    lcd.setCursor(9, 0);
    if (currentTemp > maxWaterTemp){
      //lcd.print(currentTemp);
      lcd.print("OVERHEAT");
    }else{
      lcd.print(" ------ ");
      lcd.setCursor(0, 1);
      lcd.print("Last kW:");
      lcd.print("(");
      lcd.print((int(dayPower/1000*100))/100.0);
      lcd.print(") ");
    }
  }
  delay(5);
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
