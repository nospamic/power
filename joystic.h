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
