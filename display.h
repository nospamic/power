#include <LiquidCrystal_I2C.h>
#include <Wire.h>


LiquidCrystal_I2C lcd(0x27, 16, 2);

const int symbolCount = 4;
byte customChar[symbolCount][8] = {
 
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
  0b00000}
};
