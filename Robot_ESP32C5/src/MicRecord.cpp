#include "Testmode.h"
#if RUN_MODE == MODE_MIC_RECORD
#include <Arduino.h>
#include "Mic.h"
#include "MicFilter.h"

//___Ghi am
#define REC_SECONDS 5 //Thoi gian ghi am(s)
#define GAIN 4.0f //He so khuech dai 
#define SERIAL_BAUD 921600
#define SEND_RAW 1 //1: Gui tho + loc / 0: Chi gui loc

//___Bo loc (0 de tat)
#define HPF_HZ 80 //Thong so loc thong cao (cat duoi 80Hz)
#define NOTCH_HZ 0 //Chan hep
#define LPF_HZ 0 //Thong so loc thong thap (cat tren 0Hz)

//___Startup cho mic
#define STARTUP_MS ((uint32_t)(262144.0 * 1000.0 / (64.0 * MIC_RATE)) + 50)

static Biquad hpf, notchF, lpf;
static int16_t pcmOut[MIC_FRAMES * 2];
static unsigned long micStartMs = 0;

static inline float filterSample(float s) {
  return lpf.process(notchF.process(hpf.process(s)));
}

//Chuyen tu 24-bit float sang PCM16, co khuech dai
static inline int16_t toPcm16(float v) {
  v = v * GAIN / 256.0f;
  if (v > 32767.0f) return 32767;
  if (v < -32768.0f) return -32768;
  return (int16_t)v;
}

//Doc tu DMA I2S, tra ve so khung da doc
static int readFrames() {
  size_t got = 0;
  i2s_channel_read(micRx, micBuf, sizeof(micBuf), &got, portMAX_DELAY);
  return got / (2 * sizeof(int32_t));
}

void setup() {
  Serial.setTxBufferSize(8192); 
  Serial.begin(SERIAL_BAUD);
  while (!Serial && millis() < 3000) {}

  hpf.setHighpass(MIC_RATE, HPF_HZ);
  notchF.setNotch(MIC_RATE, NOTCH_HZ);
  lpf.setLowpass(MIC_RATE, LPF_HZ);

  if (!micBegin()) {
    Serial.println("Micro I2S: LOI khoi tao, kiem tra lai chan trong Mic.h");
    while (true) delay(1000);
  }
  micStartMs = millis();
  Serial.printf("San sang. Loc: HPF=%d Hz, NOTCH=%d Hz, LPF=%d Hz, gui ban tho=%d\n", HPF_HZ, NOTCH_HZ, LPF_HZ, SEND_RAW);
}

static void recordAndSend() {
  while (millis() - micStartMs < STARTUP_MS) delay(10); //Cho startup

  //Xoa bo du lieu cu trong bo dem I2S
  size_t got = 0;
  for (int i = 0; i < 8; i++) i2s_channel_read(micRx, micBuf, sizeof(micBuf), &got, 0);

  //Cho bo loc chay de on dinh, gui bo dem dau tien di
  for (uint32_t n = 0; n < MIC_RATE / 5; ) {
    int frames = readFrames();
    for (int i = 0; i < frames; i++) filterSample(micBuf[i * 2 + MIC_CHANNEL] >> 8);
    n += frames;
  }

  const uint32_t total = (uint32_t)MIC_RATE * REC_SECONDS;
  const int channels = SEND_RAW ? 2 : 1;

  //Title cho script Python nhan du lieu WAV
  Serial.printf("WAVSTART %d %d %lu\n", MIC_RATE, channels, (unsigned long)total);

  uint32_t sent = 0;
  while (sent < total) {
    int frames = readFrames();
    int n = 0;
    for (int i = 0; i < frames && sent + n < total; i++) {
      float s = micBuf[i * 2 + MIC_CHANNEL] >> 8; //Mau goc 24-bit, chuyen sang float
      float y = filterSample(s);
      if (SEND_RAW) {
        pcmOut[n * 2] = toPcm16(s); //Ban tho
        pcmOut[n * 2 + 1] = toPcm16(y); //Ban loc
      } else {
        pcmOut[n] = toPcm16(y);
      }
      n++;
    }
    Serial.write((const uint8_t *)pcmOut, n * channels * sizeof(int16_t));
    sent += n;
  }
  Serial.flush();
  Serial.print("\nWAVEND\n");
}

void loop() {
  if (Serial.available() && Serial.read() == 'r') recordAndSend();
}

#endif