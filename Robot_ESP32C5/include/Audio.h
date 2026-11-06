#ifndef ROBOT_AUDIO_H
#define ROBOT_AUDIO_H
// ====================================================================
// 2 LOA qua 2 mạch MAX98357A, chung một đường I2S (chế độ phát)
// - BCLK, LRC dùng chung chân với SCK, WS của mic; DIN riêng
// - Khung I2S 2 khe: khe trái cho module trái (SD nối VIN), khe phải cho module phải
// - Âm thanh chạy trong task riêng, không làm chậm màn hình
// - Mỗi âm: nếu có file WAV trong flash (LittleFS) thì phát file, không có thì tự tạo bằng code
//   File WAV phải là: PCM 16-bit, mono, 16000 Hz
// ====================================================================
#include <Arduino.h>
#include <LittleFS.h>
#include <driver/i2s_std.h>

#define AUD_BCLK   4      // chung với MIC_SCK
#define AUD_WS     5      // chung với MIC_WS
#define AUD_DOUT   24     // DIN của cả 2 MAX98357A
#define AUD_RATE   16000
#define AUD_BLOCK  256    // 16 ms mỗi khối

enum Wave : uint8_t { W_SINE, W_TRI, W_SQUARE, W_NOISE, W_REST };

// Một nốt: quét tần số từ f0 tới f1 trong ms mili giây, có thể rung (vibrato)
struct Note { uint16_t f0, f1, ms; uint8_t wave, vol, vibHz, vibDepth; };

enum SoundId : uint8_t {
  SND_BOOT, SND_HAPPY, SND_CRY, SND_SHY, SND_CONFUSED,
  SND_DIZZY, SND_YAWN, SND_CLAP, SND_SLEEP, SND_COUNT
};

// --- Âm tạo bằng code (dùng khi không có file WAV) ---
static const Note N_BOOT[]     = {{523,523,90,W_TRI,70,0,0},{659,659,90,W_TRI,70,0,0},{784,784,90,W_TRI,70,0,0},{1047,1047,220,W_TRI,70,0,0}};
static const Note N_HAPPY[]    = {{1568,1568,70,W_SINE,80,0,0},{0,0,40,W_REST,0,0,0},{2093,2093,160,W_SINE,80,0,0}};
static const Note N_CRY[]      = {{660,330,600,W_TRI,70,6,15},{0,0,80,W_REST,0,0,0},{550,275,700,W_TRI,60,6,15}};
static const Note N_SHY[]      = {{330,300,260,W_SINE,50,0,0}};
static const Note N_CONFUSED[] = {{300,620,380,W_TRI,70,0,0}};
static const Note N_DIZZY[]    = {{500,500,900,W_TRI,60,8,150}};
static const Note N_YAWN[]     = {{420,180,1500,W_TRI,60,4,10}};
static const Note N_CLAP[]     = {{0,0,45,W_NOISE,90,0,0}};
static const Note N_SLEEP[]    = {{784,784,180,W_SINE,50,0,0},{659,659,180,W_SINE,45,0,0},{523,523,400,W_SINE,40,0,0}};

struct SoundDef { const char *wav; const Note *notes; uint8_t n; };
#define NOTES(a) a, (uint8_t)(sizeof(a) / sizeof(a[0]))
static const SoundDef SOUNDS[SND_COUNT] = {
  {"/boot.wav",  NOTES(N_BOOT)},
  {"/happy.wav", NOTES(N_HAPPY)},
  {"/cry.wav",   NOTES(N_CRY)},
  {"/shy.wav",   NOTES(N_SHY)},
  {"/huh.wav",   NOTES(N_CONFUSED)},
  {"/dizzy.wav", NOTES(N_DIZZY)},
  {"/yawn.wav",  NOTES(N_YAWN)},
  {"/clap.wav",  NOTES(N_CLAP)},
  {"/sleep.wav", NOTES(N_SLEEP)},
};

struct AudioReq { uint8_t id; float pan; };   // pan: -1 = chỉ loa trái, 0 = giữa, 1 = chỉ loa phải

static i2s_chan_handle_t audTx = NULL;
static QueueHandle_t audQ = NULL;
static volatile float audVolume = 0.4f;   // 0..1, để thấp cho an toàn loa 2 W và nguồn LM2596
static bool audFsOk = false;

// Đọc phần đầu file WAV, trả về số byte dữ liệu âm thanh (0 nếu sai định dạng)
static uint32_t wavOpenData(File &f) {
  uint8_t h[12];
  if (f.read(h, 12) != 12 || memcmp(h, "RIFF", 4) || memcmp(h + 8, "WAVE", 4)) return 0;
  bool fmtOk = false;
  uint8_t ch[8];
  while (f.read(ch, 8) == 8) {
    uint32_t len = ch[4] | ch[5] << 8 | ch[6] << 16 | (uint32_t)ch[7] << 24;
    if (!memcmp(ch, "fmt ", 4)) {
      uint8_t fm[16];
      if (len < 16 || f.read(fm, 16) != 16) return 0;
      uint16_t fmt = fm[0] | fm[1] << 8, chans = fm[2] | fm[3] << 8, bits = fm[14] | fm[15] << 8;
      uint32_t rate = fm[4] | fm[5] << 8 | fm[6] << 16 | (uint32_t)fm[7] << 24;
      fmtOk = fmt == 1 && chans == 1 && bits == 16 && rate == AUD_RATE;
      f.seek(f.position() + len - 16 + (len & 1));
    } else if (!memcmp(ch, "data", 4)) {
      return fmtOk ? len : 0;
    } else {
      f.seek(f.position() + len + (len & 1));
    }
  }
  return 0;
}

// Trạng thái của âm đang phát
struct AudioPlayer {
  enum { IDLE, SYNTH, WAV } mode = IDLE;
  const Note *notes = nullptr; uint8_t n = 0, idx = 0;
  uint32_t pos = 0, len = 0;
  float phase = 0, gl = 0.707f, gr = 0.707f;
  uint32_t rng = 0x12345678;
  File file; uint32_t wavLeft = 0;

  void start(const AudioReq &r) {
    if (file) file.close();
    float a = (constrain(r.pan, -1.0f, 1.0f) + 1.0f) * (float)PI / 4.0f;   // chia đều công suất cho 2 loa
    gl = cosf(a); gr = sinf(a);
    const SoundDef &d = SOUNDS[r.id < SND_COUNT ? r.id : 0];
    if (audFsOk && d.wav && LittleFS.exists(d.wav)) {
      file = LittleFS.open(d.wav, "r");
      if (file && (wavLeft = wavOpenData(file)) > 0) { mode = WAV; return; }
      Serial.printf("Am thanh: %s khong dung dinh dang (can PCM 16-bit, mono, 16000 Hz), dung am tao bang code\n", d.wav);
      if (file) file.close();
    }
    notes = d.notes; n = d.n; idx = 0; pos = 0; phase = 0;
    len = (uint32_t)notes[0].ms * AUD_RATE / 1000;
    mode = SYNTH;
  }

  float synthNext() {
    while (pos >= len) {
      if (++idx >= n) { mode = IDLE; return 0; }
      pos = 0; len = (uint32_t)notes[idx].ms * AUD_RATE / 1000;
    }
    const Note &nt = notes[idx];
    float t = (float)pos / len;
    float f = nt.f0 + (nt.f1 - nt.f0) * t;
    if (nt.vibHz) f += nt.vibDepth * sinf(2.0f * PI * nt.vibHz * pos / AUD_RATE);
    phase += f / AUD_RATE;
    if (phase >= 1.0f) phase -= 1.0f;
    float v;
    switch (nt.wave) {
      case W_SINE:   v = sinf(2.0f * PI * phase); break;
      case W_TRI:    v = 4.0f * fabsf(phase - 0.5f) - 1.0f; break;
      case W_SQUARE: v = phase < 0.5f ? 0.6f : -0.6f; break;
      case W_NOISE:  rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
                     v = (int32_t)rng / 2147483648.0f * (1.0f - t); break;   // tắt dần: tiếng "bộp"
      default:       v = 0;
    }
    uint32_t edge = AUD_RATE / 200;   // 5 ms vào/ra để không có tiếng "tách"
    float env = pos < edge ? (float)pos / edge : (len - pos < edge ? (float)(len - pos) / edge : 1.0f);
    pos++;
    return v * env * nt.vol / 100.0f;
  }

  // Điền một khối mẫu stereo 32-bit (16 bit cao là âm thanh)
  void render(int32_t *buf, int frames) {
    static int16_t pcm[AUD_BLOCK];
    int got = 0;
    if (mode == WAV) {
      int want = min((uint32_t)frames * 2, wavLeft);
      got = file.read((uint8_t *)pcm, want) / 2;
      wavLeft -= got * 2;
      if (got < frames) { file.close(); mode = IDLE; }
    }
    float vol = audVolume;
    for (int i = 0; i < frames; i++) {
      float s = 0;
      if (mode == SYNTH) s = synthNext();
      else if (i < got)  s = pcm[i] / 32768.0f;
      s *= vol;
      buf[i * 2]     = (int32_t)(s * gl * 32767.0f) << 16;
      buf[i * 2 + 1] = (int32_t)(s * gr * 32767.0f) << 16;
    }
  }
};

static void audioTask(void *) {
  static int32_t buf[AUD_BLOCK * 2];
  static AudioPlayer player;
  AudioReq r;
  for (;;) {
    if (xQueueReceive(audQ, &r, 0) == pdTRUE) player.start(r);
    player.render(buf, AUD_BLOCK);   // không có âm thì ghi số 0: loa im, clock vẫn chạy
    size_t written;
    i2s_channel_write(audTx, buf, sizeof(buf), &written, portMAX_DELAY);
  }
}

inline bool audioBegin() {
  i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  cc.dma_desc_num = 6;
  cc.dma_frame_num = AUD_BLOCK;
  if (i2s_new_channel(&cc, &audTx, NULL) != ESP_OK) return false;
  i2s_std_config_t sc = {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(AUD_RATE),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = (gpio_num_t)AUD_BCLK,
      .ws   = (gpio_num_t)AUD_WS,
      .dout = (gpio_num_t)AUD_DOUT,
      .din  = I2S_GPIO_UNUSED,
      .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
    },
  };
  if (i2s_channel_init_std_mode(audTx, &sc) != ESP_OK) return false;
  if (i2s_channel_enable(audTx) != ESP_OK) return false;
  audFsOk = LittleFS.begin(true);   // false: không tự xóa flash nếu chưa có hệ thống file
  if (!audFsOk) Serial.println("Am thanh: chua co file WAV trong flash, dung am tao bang code");
  audQ = xQueueCreate(4, sizeof(AudioReq));
  return audQ && xTaskCreate(audioTask, "audio", 4096, NULL, 5, NULL) == pdPASS;
}

inline void audioPlay(SoundId id, float pan = 0) {
  AudioReq r = {(uint8_t)id, pan};
  if (audQ) xQueueSend(audQ, &r, 0);   // âm mới thay âm đang phát
}

inline void audioSetVolume(float v) { audVolume = constrain(v, 0.0f, 1.0f); }

#endif