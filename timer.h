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
    if (millis() < 5000){ 
      startTime = millis();
    }
    return result;
  }

  void restart(){
    startTime = millis();
  }
  
private:
  unsigned long long startTime;
};
