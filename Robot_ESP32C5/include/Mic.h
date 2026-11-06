#ifndef ROBOEYES_MIC_H
#define ROBOEYES_MIC_H
#include <Arduino.h>
#include <driver/i2s_std.h>
#include <driver/gpio.h>

#define MIC_SCK 4 //SCK (BCLK)
#define MIC_WS 5 //WS (LRCLK)
#define MIC_SD 23 //SD (DATA)

#define MIC_CHANNEL 0
#define MIC_RATE 16000 // Tan so lay mau (Hz)
#define MIC_FRAMES 256 

static i2s_chan_handle_t micRx = NULL;
static int32_t micBuf[MIC_FRAMES * 2];
static float micDc[2] = {0, 0}; 
static bool micDcReady = false;
static float micLevelCh[2] = {0, 0};
static float micPeakCh[2] = {0, 0};

//Khoi tao I2S
inline bool micBegin() {
  i2s_chan_config_t chanCfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  chanCfg.dma_desc_num  = 8;
  chanCfg.dma_frame_num = MIC_FRAMES;
  if (i2s_new_channel(&chanCfg, NULL, &micRx) != ESP_OK) return false;

  i2s_std_config_t stdCfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(MIC_RATE),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = (gpio_num_t)MIC_SCK,
      .ws = (gpio_num_t)MIC_WS,
      .dout = I2S_GPIO_UNUSED,
      .din = (gpio_num_t)MIC_SD,
      .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
    },
  };

  if (i2s_channel_init_std_mode(micRx, &stdCfg) != ESP_OK) return false;

  gpio_set_pull_mode((gpio_num_t)MIC_SD, GPIO_PULLDOWN_ONLY);

  if (i2s_channel_enable(micRx) != ESP_OK) return false;
  return true;
}

inline bool micUpdate() {
  if (micRx == NULL) return false;
  size_t bytesRead = 0;
  i2s_channel_read(micRx, micBuf, sizeof(micBuf), &bytesRead, 0);
  int frames = bytesRead / (2 * sizeof(int32_t));
  if (frames == 0) return false;

  if (!micDcReady) { micDc[0] = micBuf[0] >> 8; micDc[1] = micBuf[1] >> 8; micDcReady = true; }

  for (int ch = 0; ch < 2; ch++) {
    double sumSq = 0;
    float maxAbs = 0;
    for (int i = 0; i < frames; i++) {
      float s = micBuf[i * 2 + ch] >> 8;
      micDc[ch] += (s - micDc[ch]) * 0.001f;
      float v = s - micDc[ch];
      sumSq += (double)v * v;
      if (fabsf(v) > maxAbs) maxAbs = fabsf(v);
    }
    micLevelCh[ch] = 20.0f * log10f(sqrt(sumSq / frames) + 1.0f);
    micPeakCh[ch] = 20.0f * log10f(maxAbs + 1.0f);
  }
  return true;
}

inline float micLevel() { return micLevelCh[MIC_CHANNEL]; }
inline float micPeak() { return micPeakCh[MIC_CHANNEL]; }
inline float micLevelOf(int ch) { return micLevelCh[ch]; }
inline float micPeakOf(int ch) { return micPeakCh[ch]; }

#endif