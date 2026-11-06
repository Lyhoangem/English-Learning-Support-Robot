#include "Testmode.h"
#if RUN_MODE == MODE_ROBOT
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include "RoboEyes.h"
#include "ServoCtl.h"
#include "Audio.h"

//___CAU HINH PHAN CUNG
#define TFT_MOSI 7
#define TFT_SCLK 6
#define TFT_CS 10
#define TFT_DC 2
#define TFT_RST 3
#define LED_READY 26 //LED bao "tat ca da chay" (LED nguon noi thang 5 V, khong qua GPIO)
#define SCREEN_W 320
#define SCREEN_H 240
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
RoboEyes<Adafruit_ST7789> robot(tft);

//___CAU HINH THONG SO
//Giao dien
const uint16_t COLOR_BG = ST77XX_BLACK;
const uint16_t COLOR_EYE = ST77XX_CYAN;
const byte FPS = 40;
const byte EYE_WIDTH = 80;
const byte EYE_HEIGHT = 80;
const int EYE_SPACE = 50;
const int EYE_LIFT = 30;

//Cau hinh thoi gian
const unsigned long IDLE_TIMEOUT = 10000; //De yen thi ngap
const unsigned long EMOTE_DURATION = 3750; //Thoi gian hien thi bieu cam
const unsigned long BOOT_CLOSED_MS = 1500; //Thoi gian nham mat khi khoi dong

//Chop mat
const int BLINK_INTERVAL = 3; //Khoang thoi gian giua 2 lan chop
const int BLINK_VARIATION = 2; //Do lech random

//Animation Mo/Dong mat 
const int OPEN_CLOSE_FRAMES = 30;
const int OPEN_CLOSE_DELAY_MS = 20;

//___FSM
enum SystemState {
  STATE_BOOT, //Khoi dong
  STATE_DEFAULT, //Mat default
  STATE_HAPPY, //Vui ve
  STATE_CRYING, //Khoc
  STATE_SHY, //Ngai 
  STATE_CONFUSED, //Boi roi
  STATE_YAWNING, //Ngap*
  STATE_SLEEP, //Ngu
  STATE_DIZZY, //Choang
  STATE_CLAPPING //Vo tay*
};

SystemState currentState = STATE_BOOT;
unsigned long lastInteractionTime = 0;
unsigned long emoteStartTime = 0;
unsigned long stateStartTime = 0; //Moc thoi gian vao trang thai hien tai (dung cho kich ban servo)

#include "Behavior.h" //Kich ban dau, tay, am thanh (can SystemState, robot, stateStartTime o tren)

//___LED TRANG THAI
//0 = moi thu chay: LED sang lien; N > 0 = loi: nhay N lan roi nghi
//1 = am thanh (I2S) loi, 2 = servo (PWM) loi
uint8_t errorCode = 0;

void updateStatusLed() {
  if (errorCode == 0) { digitalWrite(LED_READY, HIGH); return; }
  unsigned long t = millis() % (errorCode * 400UL + 1200);
  digitalWrite(LED_READY, t < errorCode * 400UL && (t % 400) < 200);
}

//___KEY BINDS
const char KEY_SLEEP = '9'; //Ngu
const char KEY_WAKE  = '0'; //Wake_Up

struct KeyBinding {
  char key;
  SystemState state;
};

const KeyBinding EMOTE_KEYS[] = {
  {'0', STATE_DEFAULT},
  {'1', STATE_HAPPY},
  {'2', STATE_CRYING},
  {'3', STATE_SHY},
  {'4', STATE_CONFUSED},
  {'5', STATE_DIZZY},
  {'6', STATE_CLAPPING},
};

//___KIEM TRA TRANG THAI BIEU CAM
bool isTimedEmote(SystemState s) {
  return s == STATE_HAPPY  || s == STATE_CRYING   ||
         s == STATE_SHY    || s == STATE_CONFUSED ||
         s == STATE_DIZZY;
}

//___KIEM TRA TRANG THAI MOT LAN
bool isOneShotFinished(SystemState s) {
  return (s == STATE_YAWNING  && !robot.yawning) ||
         (s == STATE_CLAPPING && !robot.clapping);
}

//___CHUYEN TRANG THAI BIEU CAM
void startEmote(unsigned char mood) {
  robot.setMood(mood);
  emoteStartTime = millis();
}

bool transitioning = false;
SystemState pendingState = STATE_DEFAULT;

void applyState(SystemState newState) {
  //TAT TRANG THAI CU
  robot.setMood(MOOD_DEFAULT);
  robot.setAutoblinker(false);

  //BAT TRANG THAI MOI
  if (newState != STATE_DEFAULT && newState != STATE_SLEEP) {
    robot.snapOpen();
  }

  switch (newState) {
    case STATE_DEFAULT:
      robot.snapClosed();
      robot.setAutoblinker(true, BLINK_INTERVAL, BLINK_VARIATION);
      break;
    case STATE_HAPPY: startEmote(HAPPY); break;
    case STATE_CRYING: startEmote(CRYING); break;
    case STATE_SHY: startEmote(SHY); break;
    case STATE_CONFUSED: startEmote(CONFUSED); break;
    case STATE_YAWNING: startEmote(YAWNING); break;
    case STATE_DIZZY: startEmote(DIZZY); break;
    case STATE_CLAPPING: startEmote(CLAPPING); break;
    case STATE_SLEEP:
      robot.close();
      break;
    case STATE_BOOT:
      break;
  }
  behaviorEnter(newState, currentState);
  stateStartTime = millis();
  currentState = newState;
}

void switchState(SystemState newState) {
  if (transitioning) {
    if (newState == STATE_DEFAULT) {
      transitioning = false;
      robot.open();
      robot.setAutoblinker(true, BLINK_INTERVAL, BLINK_VARIATION);
      return;
    }
    if (newState != STATE_SLEEP) {
      pendingState = newState;
      return;
    }
    transitioning = false;
  }

  if (currentState == newState) return;

  if (currentState == STATE_DEFAULT && newState != STATE_SLEEP) {
    transitioning = true;
    pendingState = newState;
    robot.setAutoblinker(false);
    robot.close();
    return;
  }
  applyState(newState);
}

void goToDefault() {
  switchState(STATE_DEFAULT);
  lastInteractionTime = millis();
}

void runFrames(int frames, int delayMs) {
  for (int i = 0; i < frames; i++) {
    robot.update();
    delay(delayMs);
  }
}

void enterSleep() {
  switchState(STATE_SLEEP);
  runFrames(OPEN_CLOSE_FRAMES, OPEN_CLOSE_DELAY_MS);
  tft.fillScreen(COLOR_BG);
}

//___KHOI DONG
void setupDisplay() {
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.init(240, 320);
  tft.setRotation(1);
  tft.fillScreen(COLOR_BG);
}

void setupEyes() {
  robot.setDisplayColors(COLOR_BG, COLOR_EYE);
  robot.begin(SCREEN_W, SCREEN_H, FPS);
  robot.setWidth(EYE_WIDTH, EYE_WIDTH);
  robot.setHeight(EYE_HEIGHT, EYE_HEIGHT);
  robot.setSpacebetween(EYE_SPACE);
  robot.eyeLift = EYE_LIFT;
  robot.centerEyes();
}

//Nham mat, robot vuon vai + am bao bat may
void bootAnimation() {
  robot.close();
  robot.eyeLheightCurrent = 1;
  robot.eyeRheightCurrent = 1;
  behaviorEnter(STATE_BOOT, STATE_BOOT);
  stateStartTime = millis();

  unsigned long wakeTimer = millis();
  while (millis() - wakeTimer < BOOT_CLOSED_MS + 500) {
    robot.update();
    behaviorUpdate(STATE_BOOT, millis() - stateStartTime);
    updateStatusLed();
  }
}

void checkStateTimers() {
  if (transitioning) {
    if (robot.eyesClosed()) {
      transitioning = false;
      applyState(pendingState);
    }
    return;
  }
  if (isOneShotFinished(currentState)) {
    goToDefault();
    return;
  }
  if (isTimedEmote(currentState) && millis() - emoteStartTime >= EMOTE_DURATION) {
    goToDefault();
    return;
  }
  if (currentState == STATE_DEFAULT && millis() - lastInteractionTime > IDLE_TIMEOUT) {
    switchState(STATE_YAWNING);
  }
}

//___LENH CHINH SERVO / AM THANH QUA SERIAL (Serial Monitor chon "Newline")
// s <id> <us>  : dua servo id (0 dau, 1 tay trai, 2 tay phai) toi xung <us> (500-2500)
// z <id>       : lay xung hien tai lam goc 0      n <id> / x <id> : lam gioi han min / max
// i <id>       : dao chieu servo                   q : thoat che do chinh tay
// w            : luu hieu chinh vao flash          p : in hieu chinh
// v <0-100>    : am luong                          t <0-8> : phat thu am thanh
void handleCommand(char cmd) {
  String rest = Serial.readStringUntil('\n');
  rest.trim();
  int sp = rest.indexOf(' ');
  int a = rest.toInt();
  int b = sp > 0 ? rest.substring(sp + 1).toInt() : 0;
  bool ok = true;
  switch (cmd) {
    case 's': ok = servoRaw(a, b); break;
    case 'z': ok = servoCalFromRaw(a, CAL_ZERO); break;
    case 'n': ok = servoCalFromRaw(a, CAL_MIN); break;
    case 'x': ok = servoCalFromRaw(a, CAL_MAX); break;
    case 'i': ok = servoFlip(a); break;
    case 'q': servoRawOffAll(); break;
    case 'w': ok = servoSaveCal(); break;
    case 'p': servoPrintCal(); break;
    case 'v': audioSetVolume(a / 100.0f); break;
    case 't': ok = a >= 0 && a < SND_COUNT; if (ok) audioPlay((SoundId)a); break;
  }
  Serial.printf("%c %s -> %s\n", cmd, rest.c_str(), ok ? "OK" : "LOI (sai so servo, hoac chua dung lenh s truoc)");
}

//___NHAN PHIM TU SERIAL
void handleSerialInput() {
  if (Serial.available() <= 0) return;
  char input = Serial.read();
  if (input && strchr("szniqwpvt", input)) { handleCommand(input); return; }
  if (currentState == STATE_SLEEP) {
    if (input == KEY_WAKE) goToDefault();
    return;
  }
  if (input == KEY_SLEEP) {
    enterSleep();
    return;
  }

  for (const KeyBinding &binding : EMOTE_KEYS) {
    if (binding.key == input) {
      lastInteractionTime = millis();
      switchState(binding.state);
      return;
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  pinMode(LED_READY, OUTPUT);
  digitalWrite(LED_READY, LOW);

  setupDisplay();
  setupEyes();
  if (!audioBegin()) { errorCode = 1; Serial.println("LOI: khong khoi tao duoc I2S cho loa"); }
  if (!servoBegin()) { errorCode = 2; Serial.println("LOI: khong khoi tao duoc PWM cho servo"); }
  else servoPowerUp(POSE_REST); //Bat tung servo mot, cach nhau 250 ms
  bootAnimation();
  goToDefault();
  Serial.println(errorCode ? "Khoi dong xong nhung co loi, xem LED" : "Khoi dong xong, moi thu dang chay");
}

void loop() {
  if (currentState != STATE_SLEEP) {
    robot.update();
    checkStateTimers();
  }
  behaviorUpdate(currentState, millis() - stateStartTime);
  handleSerialInput();
  updateStatusLed();
}

#endif