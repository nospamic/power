#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);


#define VOLT_PORT 0 //on PCB 0
#define WATER_TEMP_PORT 6 //on PCB Power_A
#define WATER_TEMP_BUS 8 //D8

enum digital {DEV_A = 6, DEV_B = 7, DEV_C = 9, CONTACTOR = 3};//on PCB 6,7, (9, 3)joystic(93...)
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
bool timeLeft(long int milliseconds);
bool timeLeftTest(long int milliseconds);
void diodeOn(bool);
void showCurrentMode();
void showDayPower();
void showTemp();
void addDayPower();

const float resist_1 = 32.0f;
const float resist_2 = 32.0f;
const float resist_3 = 32.0f;
const float maxWaterTemp = 55.0f;
float currentTemp = 0.0f;
float voltageDivisor = 3.84f;
float voltageDeviation = 0.0f;
float currentPower = 0.0f;
float currentVoltage = 0.0f;
float lastVoltage;
float upperVoltage = 232.0f;
float lowerVoltage = 160.0f;
int currentMode = PASSIVE;
float dayPower = 0;
const int diodePort = 13;

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
      return currentVoltage * currentVoltage / resistance;
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
      isOn = true;
      delay(400);
  }
  void off(){
    digitalWrite(devicePort, false);
    isOn = false;
    delay(10000);
  }
  bool isTurnedOn(){
    return isOn;
  }
private:
  int devicePort;
  bool isOn;
};


class Timer {
public:
  Timer(){
    startTime = millis();
  }
  bool timeLeft(long int milliseconds) {
  bool result = false;
  if (millis() > (startTime + milliseconds)) {
    result = true;
    startTime = millis();
  }
  if (millis() < 5000) startTime = millis();
  return result;
}
private:
  unsigned long long startTime;
};



Timer timerInfo;
Timer timerTest;
Heater heater_1;
Heater heater_2;
Heater heater_3;
Contactor contactor;

byte customChar0[8] = {
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};

byte customChar1[8] = {
  0b11111,
  0b10001,
  0b10001,
  0b00000,
  0b00000,
  0b10001,
  0b10001,
  0b11111
};



///////////////////////////////////////  SETUP  //////////////////////////////////////////////
void setup() {
  analogReference(EXTERNAL);
  pinMode(diodePort, OUTPUT);
  sensor.begin();
  //Serial.begin(9600);
  lcd.init();
  lcd.createChar(0, customChar0);
  lcd.createChar(1, customChar1);
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  heater_1 = Heater(DEV_A, resist_1);
  heater_2 = Heater(DEV_B, resist_2);
  heater_3 = Heater(DEV_C, resist_3);
  contactor = Contactor(CONTACTOR);
  lcd.clear();
  setOptimalMode();
  lastVoltage = getVoltage();
 
}


///////////////////////////////////////   LOOP   ////////////////////////////////////////////////
void loop() {
  if(currentTemp > maxWaterTemp || currentTemp < 0){
    setOptimalMode();
    lastVoltage = getVoltage();
  }
  if(timerTest.timeLeft(180000)){
    setOptimalMode();
    lastVoltage = getVoltage();
    addDayPower();
  }
  
  if(abs(lastVoltage - getVoltage()) > 25.0){
    setOptimalMode();
    lastVoltage = getVoltage();
  }
  
  if (timerInfo.timeLeft(500)) {
    showInfo();
    setCurrentTemp();
  }
  delay(5);
}


float getVoltage() {
  delay(500);
  currentVoltage = round(analogRead(VOLT_PORT) / voltageDivisor + voltageDeviation);
  return currentVoltage;
}


float getCurrentPower(){
  float result = 0.0;
  float voltage = getVoltage();
  if(heater_1.isTurnedOn()){
    result += heater_1.getPower();  
  }
  if(heater_2.isTurnedOn()){
    result += heater_2.getPower();  
  }
  if(heater_3.isTurnedOn()){
    result += heater_3.getPower();  
  }
  currentPower = result;
  return result; 
}

void setMode(int mode){
  heater_1.turnOff();
  heater_2.turnOff();
  heater_3.turnOff();
  if(mode == PASSIVE){
    currentMode = PASSIVE;
  }
  if(mode == STAGE_ONE){
    currentMode = STAGE_ONE;
    heater_1.turnOn();
  }
  if(mode == STAGE_TWO){
    currentMode = STAGE_TWO;
    heater_1.turnOn();
    heater_2.turnOn();
  }
  if(mode == STAGE_THREE){
    currentMode = STAGE_THREE;
    heater_1.turnOn();
    heater_2.turnOn();
    heater_3.turnOn();
  }
  showCurrentMode();
}

void setOptimalMode(){
  lcd.setCursor(0, 0);
  lcd.print(" TEST ");
    setMode(PASSIVE);
    currentVoltage = getVoltage();
    if(currentVoltage < lowerVoltage || currentTemp > maxWaterTemp){
      if(contactor.isTurnedOn()){
        contactor.off();
      }
      return;
    }
    if(currentVoltage > upperVoltage){
      float maxPower = 0.0f;
      if(!contactor.isTurnedOn()){
        contactor.on();
        dayPower = 0.0;
      }
      setMode(STAGE_ONE);
      currentPower = getCurrentPower();
      if (currentPower > maxPower){
        maxPower = currentPower;
      }else{
        setMode(PASSIVE);
        return;
      }
      setMode(STAGE_TWO);
      currentPower = getCurrentPower();
      if (currentPower > maxPower){
        maxPower = currentPower;
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
}

void setCurrentTemp(){
  sensor.requestTemperatures();
  currentTemp = sensor.getTempCByIndex(0);
}


void addDayPower(){
  dayPower += (currentPower/20); //watt
}


void showInfo() {
  lcd.setCursor(0, 0);
  lcd.print("U=");
  lcd.print(int(currentVoltage));
  lcd.print("v ");
  if (currentMode != PASSIVE){
    showCurrentMode();
    lcd.setCursor(0, 1);
    lcd.print("P=");
    currentPower = currentVoltage * currentVoltage / (resist_1 / float(currentMode)); //heaters have same resistance
    lcd.print((int(currentPower/1000*100))/100.0);
    lcd.print("kW ");
    showTemp();
  }
  if (currentMode == PASSIVE){
    lcd.setCursor(9, 0);
    if (currentTemp > maxWaterTemp){
      lcd.print("OVERHEAT");
    }else{
      showCurrentMode();
      showTemp();
      showDayPower();
    }
  }
}

void showDayPower(){
  lcd.setCursor(0, 1);
  lcd.print("kW:");
  lcd.print((int(dayPower/100.0f))/10.0f);
  lcd.print("  ");
}

void showTemp(){
  lcd.setCursor(9, 1);
  lcd.print(" t=");
  lcd.print(round(currentTemp));
  lcd.print("C ");
}

void showCurrentMode(){
  lcd.setCursor(9, 0);
    if(currentMode == STAGE_ONE) {
      lcd.print(" -");
      lcd.write((byte)0);
      lcd.write((byte)1);
      lcd.write((byte)1);
      lcd.print("- ");
    }
    if(currentMode == STAGE_TWO) {
      lcd.print(" -");
      lcd.write((byte)0);
      lcd.write((byte)0);
      lcd.write((byte)1);
      lcd.print("- ");
    }
    if(currentMode == STAGE_THREE) {
      lcd.print(" -");
      lcd.write((byte)0);
      lcd.write((byte)0);
      lcd.write((byte)0);
      lcd.print("- ");
    }
    if(currentMode == PASSIVE) {
      lcd.print(" ----- ");
    }
}
