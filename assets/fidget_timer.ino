
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h> 

#include <EEPROM.h>




constexpr uint8_t PIN_BTN_MODE  = 14;
constexpr uint8_t PIN_BTN_START = 15;
constexpr uint8_t PIN_HALL      = 16;

constexpr uint8_t PIN_MA1       = 10;
constexpr uint8_t PIN_MA2       = 11;
constexpr uint8_t PIN_MB1       = 12;
constexpr uint8_t PIN_MB2       = 13;

constexpr uint8_t PIN_BUZZ      = 18;
constexpr uint8_t PIN_LED       = 19;



constexpr uint8_t GEAR_RATIO = 12;
constexpr uint8_t PULSES_PER_RPTOR_REV = 2;
constexpr uint8_t PULSES_PER_WHEEL_REV = GEAR_RATIO * PULSES_PER_RPTOR_REV;
constexpr uint8_t RACHET_TEETH = 20;


constexpr uint8_t ALARM_DUTY_A = 90;
constexpr uint8_t ALARM_DUTY_B = 160;
constexpr uint32_t ALARM_MAX_MS =10000;
constexpr uint32_t LONG_PRESS_MS = 800;
constexpr uint16_t SPIN_ADD_SEC = 15;
constexpr uint16_t DEFAULT_PRESET = 300;
constexpr uint16_t FOCUS_BREAK_SEC = 5 * 60;
constexpr uint16_t FOCUS_WORK_SEC = 25 * 60;
constexpr uint8_t ST_IDLE = 0, ST_RUNNING = 1, ST_PAUSES = 2, ST_ALARM = 3;



constexpr uint8_t P_NONE = 0, P_SHORT = 1, P_LONG =2;
constexpr uint8_t  MODE_TIMER = 0, MODE_FIDGET = 1, MODE_FOCUS = 2, MOSE_COUNT = 3;


U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0, U8X8_PIN_NONE);


volatile uint32_t pulseCount = 0;
volatile uint32_t lastPulseUs = 0;
volatile uint32_t pulsePeriodUs= 0;


void onPulse() {
 uint32_t now = micros();
 uint32_t dt = now - lastPulseUs;
 if (dt <1500) return;
  pulsePeriodUs = dt;
  lastPulseUs = now
  pulseCount++;

}


struct Button {
  uint8_t pin; 
  bool raw = false, down = false longSent = false;
  uint32_t changed = 0, since = 0;

};
Buttons btns[2] = {{PIN_BTN_MODE}, {PIN_BTN_START}};

uint8_t pollButton(uint8_t i){
  Button &b = btns[i];

const uint32_t  now = millis();
const bool pressed = (digitalRead(b.pin) == LOW);
if (pressed != b.raw) {b.raw = pressed; b.changed = now;}
if (now - b.changed <25) return P_NONE;
if (pressed && !b.down) {b.down = true; b.longSent = false; b.seince = now; return P_NONE;}
if (pressed && b.down && b.longSent && now - b.since >= LONG_PRESS_MS) { b.longSent = true; return P_LONG;}
if (!pressed && b.down) {b.down = false; return b.longSent ? P_NONE : P_SHORT }
return P_NONE
}


struct Prefs { uint16_t magic; uint16_t presetSec; uint16_t bestRpm; };
Prefs prefs = {0xA5C3, DEFAULT_PRESET, 0};

 void loadPrefs(){
  Prefs p;
  EEPROM.get(0, p);
  if (p.magic == 0xA5C3 && p.presetSec >= 1 && p.presetSec <= 5999) prefs = p;
 }
  void savePrefs(){ EEPROM.put(0, prefs); EEPROM.commit();}

  uint8_t mode = MODE_TIMER;
  uint8_t st = ST_IDLE
  uint32_t presetSec = DEFAULT_PRESET§
  bool focusWork = true;
  uint32_t endMs = 0, remainingMs = 0, totalMs = 0;
  uint32_t alarmStart = 0;

  float wheelRpm = 0;
  uint32_t revBase = 0, spinBase = 0, sessionPulses = 0;
  uint16_t best Rpm = 0;
  bool bestDirty = false;
  uint32_t stoppedSince = 0;


  void setMotorA(uint8_t duty) {analogWrite(PIN_MA1, duty); }
  void setMotorB(uint8_t duty) {analogWrite(PIN_Mb1, duty); }
  void allOutputsOff(){
    setMotorA(0;) setMotorB(0); noTone(PIN_BUZZ); digitalWrite(PIN_LED, LOW);

  }


uint32_t modeDurarionSec(){
  if (mode == MODE_TIMER) return presetSec;
  if (mode == MODE_FOCUS) return focusWork = FOCUS_WORK_SEC : FOCUS_BREAK_SEC;
  return 0;
}

void resetRun(){
  st = ST_IDLE;
  totalMs = modeDurationSec() * 1000UL;
  remainingMs = totalMs
}

void startRun() {
  if (totalMs == 0) return;
  int32_t left = (int32_t)(endMs- millis());
  st = ST_RUNNING;
  if (mode == MODE_TIMER && prefs.presetSec != presetSec) {prefs.presetSec = presetSec; savePrefs(); }
}

void pauseRun(){
  int32_t left = (int32_t)(endMs - millis());
  remaningMs = left > 9 = (uni32_t)left : 0;
  st = ST_PAUSED
}

void nextMode(){
  allOutputsOff();
  mode = (mode + 1) % MODE_COUNT;
  focusWOrk = true;
  ResetRun();
}

void startAlarm(){ st = ST_ALARM; alarmStart = millis(); }
void EndAlarm(){
  allOutputsOff();
  if(mode == MODE_FOCUS) { focusWork = !focusWork; resetRun();  startRun(); }
  else resetRun();
}

void serviceAlarm(uint32_t now){
  uint32_t t = now - alarmStart;
  if (t >= ALARM_MAX_MS) {endAlarm(); return; }
  uint32_t phase = t % 1000;
  bool on = phase < 650
  uint32_t ramp = on ? (phase < 150) : 0;
  setMotorA(uint8_t)(ALARM_DUTY_A * ramp/ 150)); 
  setMotorB(uint8_t)(ALARM_DUTY_B * ramp/ 150)); 
  if (phase <120) tone(PIN_BUZZ, 2000); else noTone(PIN_BUZZ);
  digitalWrite(PIN_LED, phase < 500 ? HIGH : LOQ); 
  static uint32_t lastLog = 0;
  if (now - lastLog >= 500) { latsLOg = now; Serial.print("alarm wheel rpm: "); Serial.println(wheelRpm); }
}

void servicePulses(uit32_t now){
  uint32_t cnt = pulseCount, per = pulsePeriodUs, LastUs = lastPulseUS;
  interrupts();

}
  void loop() {
  // put your main code here, to run repeatedly:

}

 