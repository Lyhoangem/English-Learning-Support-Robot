#include "Testmode.h"
#if RUN_MODE == MODE_MIC_TEST
#include <Arduino.h>
#include "Mic.h"

unsigned long lastPrint = 0;
unsigned long lastData = 0;
float peakHold[2] = {0, 0};

int countHigh(int pin) {
  int n = 0;
  for (int i = 0; i < 1000; i++) { n += digitalRead(pin); delayMicroseconds(20); }
  return n;
}

//Kiem tra chan SD
void testSdPin() {
  pinMode(MIC_SD, INPUT_PULLDOWN); delay(5);
  int withPullDown = countHigh(MIC_SD);
  pinMode(MIC_SD, INPUT_PULLUP); delay(5);
  int withPullUp = countHigh(MIC_SD);
  pinMode(MIC_SD, INPUT);
  Serial.println("__Kiem tra chan SD Khi chua bat I2S");
  Serial.printf("Keo xuong: %d  Keo len: %d\n", withPullDown, withPullUp);
  if (withPullDown < 50 && withPullUp > 950)
    Serial.println("Binh thuong");
  else
    Serial.println("Cam nham/Chao day");
}

//Kiem tra chan SCK va WS
void testClockPins() {
  pinMode(MIC_SCK, INPUT_PULLDOWN); pinMode(MIC_WS, INPUT_PULLDOWN); delay(5);
  int sck = countHigh(MIC_SCK), ws = countHigh(MIC_WS);
  pinMode(MIC_SCK, INPUT); pinMode(MIC_WS, INPUT);
  Serial.println("__Kiem tra chan SCK / WS khi chua bat I2S__");
  Serial.printf("SCK: %d  WS: %d  (binh thuong ca hai gan 0)\n", sck, ws);
}

//Kiem tra chap day chan SD voi chan SCK va WS
bool sdFollows(int drivePin) {
  pinMode(drivePin, OUTPUT);
  digitalWrite(drivePin, HIGH);
  pinMode(MIC_SD, INPUT_PULLDOWN); delay(2);
  int highWhenHigh = countHigh(MIC_SD);
  digitalWrite(drivePin, LOW);
  pinMode(MIC_SD, INPUT_PULLUP); delay(2);
  int highWhenLow = countHigh(MIC_SD);
  pinMode(drivePin, INPUT);
  pinMode(MIC_SD, INPUT);
  return highWhenHigh > 950 && highWhenLow < 50;
}

void testShorts() {
  Serial.println("__Kiem tra chap day voi chan SD__");
  bool withSck = sdFollows(MIC_SCK);
  bool withWs = sdFollows(MIC_WS);
  Serial.printf("SD chap voi SCK (GPIO %d): %s\n", MIC_SCK, withSck ? "co" : "khong");
  Serial.printf("SD chap voi WS  (GPIO %d): %s\n", MIC_WS,  withWs  ? "co" : "khong");
  if (!withSck && !withWs) Serial.println("Khong thay chap day giua SD va SCK/WS");
}

//Kiem tra tin hieu I2S
void dumpRawFrames() {
  int32_t raw[64];
  size_t got = 0;
  delay(300);
  i2s_channel_read(micRx, raw, sizeof(raw), &got, 0); 
  delay(20);
  i2s_channel_read(micRx, raw, sizeof(raw), &got, 100);
  Serial.println("__Ket qua doc mau I2S__");
  for (int i = 0; i < 16 && (i * 2 + 1) < (int)(got / 4); i++) {
    Serial.printf("%2d: %08lX   %08lX\n", i, (unsigned long)(uint32_t)raw[i * 2], (unsigned long)(uint32_t)raw[i * 2 + 1]);
  }
  Serial.println("__Done__");
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {}
  delay(1000);
  testSdPin();
  testClockPins();
  testShorts();
  if (micBegin()) Serial.println("Khoi tao OK");
  else { Serial.println("Loi khoi tao, kiem tra lai chan trong Mic.h"); return; }

  dumpRawFrames();
  lastData = millis();
}

void loop() {
  if (micUpdate()) {
    lastData = millis();
    for (int ch = 0; ch < 2; ch++) {
      if (micPeakOf(ch) > peakHold[ch]) peakHold[ch] = micPeakOf(ch);
    }
  }

  if (millis() - lastData > 1000) {
    Serial.println("Khong nhan duoc du lieu I2S - kiem tra day SCK/WS/SD va nguon 3V3");
    lastData = millis();
  }

  if (millis() - lastPrint >= 500) {
    lastPrint = millis();
    Serial.print("levelL:"); Serial.print(micLevelOf(0), 1);
    Serial.print(",peakL:"); Serial.print(peakHold[0], 1);
    Serial.print(",levelR:"); Serial.print(micLevelOf(1), 1);
    Serial.print(",peakR:"); Serial.println(peakHold[1], 1);
    peakHold[0] = peakHold[1] = 0;
  }
}
#endif