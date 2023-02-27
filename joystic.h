#define JOY_X 2
#define JOY_Y 1
#define JOY_BTN 9 //on PCB 9

class Joystic {
  public:
    enum joystic {MIDDLE, UP, DOWN, RIGHT, LEFT};
    Joystic() {}
    Joystic(int joyPortX, int joyPortY, int joyPortButton): joyPortX(joyPortX), joyPortY(joyPortY), joyPortButton(joyPortButton) {
      pinMode(joyPortButton, INPUT_PULLUP);
      buttonValue = FREE;
      joysticMiddleX = getMiddle(joyPortX);
      joysticMiddleY = getMiddle(joyPortY);
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
      if (y >= joysticMiddleY - jDeviation && y <= joysticMiddleY + jDeviation)result = MIDDLE;
      if (y < joysticMiddleY - jDeviation || y < 5) result = UP;
      if (y > joysticMiddleY + jDeviation || y > 1020) result = DOWN;
      return result;
    }
    
    int getX() {
      int result;
      int x = analogRead(joyPortX);
      if (x >= joysticMiddleY - jDeviation && x <= joysticMiddleY + jDeviation) result = MIDDLE;
      if (x < joysticMiddleY - jDeviation || x < 5) result = RIGHT;
      if (x > joysticMiddleY + jDeviation || x > 1020) result = LEFT;
      return result;
    }
    
    int getMiddleX(){
      return joysticMiddleX;
    }

    int getXval(){
      return analogRead(joyPortX);
    }
    
  private:
    enum buttValue {FREE, PRESS, RELEASE};
    int joysticMiddleX;
    int joysticMiddleY;
    int jDeviation = 400;
    int joyPortX;
    int joyPortY;
    int joyPortButton;
    int buttonValue;
    
    int getMiddle(int port){
      int middle = 0;
      int mesureCount = 20;
      for(int i = 0; i < mesureCount; ++i){
        middle += analogRead(port);
        delay(100);
      }
      return int(middle / mesureCount);
    }
};
