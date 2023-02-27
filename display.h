#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define SYMBOL_COUNT 8
LiquidCrystal_I2C lcd(0x27, 16, 2);

namespace display{
  void prepare(){
    lcd.init();
    lcd.backlight();
  }
}
byte customChar[SYMBOL_COUNT][8] = {
   {0b11100,
    0b10100,
    0b11100,
    0b00000,
    0b00000,
    0b00111,
    0b00101,
    0b00111},
   
   {0b00111,
    0b00101,
    0b00111,
    0b00000,
    0b00000,
    0b11100,
    0b10100,
    0b11100},
  
   {0b00000,
    0b00000,
    0b00111,
    0b11101,
    0b10111,
    0b11100,
    0b00000,
    0b00000},
  
   {0b00000,
    0b00000,
    0b11100,
    0b10111,
    0b11101,
    0b00111,
    0b00000,
    0b00000},
  
   {0b00000, /// right arrow
    0b11111,
    0b11011,
    0b10001,
    0b11011,
    0b11111,
    0b00000,
    0b00000},
  
   {0b10000,
    0b11000,
    0b11100,
    0b11110,
    0b11100,
    0b11000,
    0b10000,
    0b00000},
  
   {0b00001, /// left arrow
    0b00011,
    0b00111,
    0b01111,
    0b00111,
    0b00011,
    0b00001,
    0b00000},
  
   {0b00000,
    0b11111,
    0b11111,
    0b10001,
    0b11111,
    0b11111,
    0b00000,
    0b00000}
  };

class Icons{
public:
  enum Arrows{RIGHT_ARROW_1 = 0, RIGHT_ARROW_2 = 1, LEFT_ARROW_1 = 2, LEFT_ARROW_2 = 3};
  Icons(){
    frameCount = 4;
    currentFrame = 0;
  }
  
  void prepare(){
     for (int i = 0; i < SYMBOL_COUNT; ++i){
      lcd.createChar(i, customChar[i]);
      }
  }
  
   void animate(int x, int y) {
    ++currentFrame;
    if (currentFrame >= frameCount) {
      currentFrame = 0;
    }
    lcd.setCursor(x, y);
    lcd.write((byte)currentFrame);
  }

  void rightArrow(int x, int y){
    lcd.setCursor(x, y);
    lcd.write((byte)frameCount + RIGHT_ARROW_1);
    lcd.write((byte)(frameCount + RIGHT_ARROW_2));
  }
  
  void leftArrow(int x, int y){
    lcd.setCursor(x, y);
    lcd.write((byte)frameCount + LEFT_ARROW_1);
    lcd.write((byte)(frameCount + LEFT_ARROW_2));
  }
  
private:
  int frameCount;
  int currentFrame;
};
