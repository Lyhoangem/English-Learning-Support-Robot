#ifndef ROBOT_SERVO_CTL_H
#define ROBOT_SERVO_CTL_H
// ====================================================================
// 3 SERVO MG90S điều khiển trực tiếp bằng PWM của ESP32 (LEDC), không cần driver
// - Xung 50 Hz, độ phân giải 14 bit (mỗi bước khoảng 1,2 µs)
// - Chuyển động mượt: giới hạn tốc độ và gia tốc, chạy trong một task riêng 50 lần/giây
//   nên không bị giật khi màn hình đang vẽ
// - Hiệu chỉnh góc 0, giới hạn min/max, đảo chiều; lưu vào flash (NVS)
// Góc logic tính bằng độ so với điểm 0 đã hiệu chỉnh:
//   đầu: 0 = nhìn thẳng, dương = quay sang phải; tay: 0 = nghỉ sát thân, dương = vung ra trước
// ====================================================================
#include <Arduino.h>
#include <Preferences.h>

#define SRV_HEAD   0
#define SRV_ARM_L  1
#define SRV_ARM_R  2
#define SRV_COUNT  3
static const uint8_t SRV_PIN[SRV_COUNT] = {8, 9, 0};   // đầu, tay trái, tay phải

#define SRV_FREQ        50
#define SRV_RES_BITS    14
#define SRV_US_PER_DEG  11.11f   // 2000 µs cho 180° (500–2500 µs)
#define SRV_US_ABS_MIN  500      // Giới hạn tuyệt đối khi chỉnh tay
#define SRV_US_ABS_MAX  2500

struct ServoCal {
  int16_t zeroUs;   // Độ rộng xung ứng với góc logic 0
  int16_t minUs;    // Không bao giờ xuất xung ngắn hơn (chặn kẹt cơ khí)
  int16_t maxUs;    // Không bao giờ xuất xung dài hơn
  int8_t  dir;      // +1 hoặc -1 (servo lắp ngược chiều)
};

struct ServoMotion {
  float pos, vel, target;   // độ, độ/giây
  float maxVel, maxAcc;     // độ/giây, độ/giây²
  bool  on;                 // false = không xuất xung (servo thả lỏng, ít tốn điện)
};

// ====================================================================
// HIỆU CHỈNH SERVO: sửa số ở đây rồi nạp lại code
// ====================================================================
// Bước 1: đặt SRV_MOUNT_MODE = 1, nạp code. Cả 3 servo đứng ở góc 0 (xung 1500 µs).
//         Lắp càng: đầu nhìn thẳng, hai tay nghỉ sát thân.
//         Sau đó robot lần lượt cho đầu, tay trái, tay phải xoay +20° rồi về (vòng lặp 9 giây):
//           đầu phải quay sang PHẢI robot, tay phải vung ra TRƯỚC. Servo nào xoay ngược thì đổi dir thành -1.
// Bước 2: điền min/max theo bảng (1° ≈ 11 µs):
//           đầu, dir  1 hoặc -1 : min 1000, max 2000   (quay ±45°)
//           tay, dir  1         : min 1450, max 2450   (từ sát thân tới vung ra trước 85°)
//           tay, dir -1         : min  550, max 1550
//         Tay hoặc đầu chạm thân ở góc nào thì thu hẹp min/max lại ở phía đó.
// Bước 3: đặt SRV_MOUNT_MODE = 0, nạp lại code: robot chạy theo kịch bản.
#define SRV_MOUNT_MODE    0   // 1 = chế độ lắp càng và kiểm tra chiều quay
#define SRV_USE_SAVED_CAL 0   // 1 = dùng số đã lưu bằng lệnh "w" qua Serial thay cho bảng dưới

//                          zeroUs minUs maxUs dir
static ServoCal srvCal[SRV_COUNT] = {
  /* đầu      (GPIO8) */ {1500,  1000, 2000,  1},
  /* tay trái (GPIO9) */ {1500,  1450, 2450,  1},   // mặc định an toàn ±45°, sửa sau khi kiểm tra chiều
  /* tay phải (GPIO0) */ {1500,  550, 1550,  -1},
};
static ServoMotion srv[SRV_COUNT];
static volatile int16_t srvRawUs[SRV_COUNT] = {0, 0, 0};  // > 0: đang chỉnh tay, bỏ qua làm mượt

static inline uint32_t srvDuty(int us) { return (uint32_t)us * ((1UL << SRV_RES_BITS) - 1) / 20000UL; }

static inline int srvAngleToUs(int i, float deg) {
  int us = srvCal[i].zeroUs + (int)lroundf(srvCal[i].dir * deg * SRV_US_PER_DEG);
  return constrain(us, (int)srvCal[i].minUs, (int)srvCal[i].maxUs);
}

// Một bước làm mượt: tiến về đích với tốc độ tối đa maxVel, tăng/giảm tốc theo maxAcc,
// tự giảm tốc khi gần tới đích để dừng êm
static void srvStep(ServoMotion &s, float dt) {
  float err = s.target - s.pos;
  float h = s.maxAcc * dt * 0.5f;   // hiệu chỉnh cho bước rời rạc: về tới đích với vận tốc gần 0
  float vDes = copysignf(fminf(s.maxVel, sqrtf(2.0f * s.maxAcc * fabsf(err) + h * h) - h), err);
  float dv = constrain(vDes - s.vel, -s.maxAcc * dt, s.maxAcc * dt);
  s.vel += dv;
  s.pos += s.vel * dt;
  float after = s.target - s.pos;
  if ((after > 0) != (err > 0) || (fabsf(after) < 0.3f && fabsf(s.vel) < 5.0f)) {
    s.pos = s.target;   // đã vượt qua hoặc đã tới đích: dừng hẳn
    s.vel = 0;
  }
}

static void servoTask(void *) {
  TickType_t last = xTaskGetTickCount();
  for (;;) {
    for (int i = 0; i < SRV_COUNT; i++) {
      if (srvRawUs[i] > 0) { ledcWrite(SRV_PIN[i], srvDuty(srvRawUs[i])); continue; }
      if (!srv[i].on) continue;
      srvStep(srv[i], 0.02f);
      ledcWrite(SRV_PIN[i], srvDuty(srvAngleToUs(i, srv[i].pos)));
    }
    vTaskDelayUntil(&last, pdMS_TO_TICKS(20));
  }
}

// --- Hiệu chỉnh (lưu flash) ---
inline void servoLoadCal() {
  Preferences p;
  if (!p.begin("srvcal", true)) return;
  if (p.getBytesLength("cal") == sizeof(srvCal)) p.getBytes("cal", srvCal, sizeof(srvCal));
  p.end();
}

inline bool servoSaveCal() {
  Preferences p;
  if (!p.begin("srvcal", false)) return false;
  bool ok = p.putBytes("cal", srvCal, sizeof(srvCal)) == sizeof(srvCal);
  p.end();
  return ok;
}

inline void servoPrintCal() {
  static const char *names[SRV_COUNT] = {"dau", "tay trai", "tay phai"};
  for (int i = 0; i < SRV_COUNT; i++)
    Serial.printf("Servo %d (%s, GPIO%d): zero=%d min=%d max=%d dir=%d\n", i, names[i], SRV_PIN[i],
                  srvCal[i].zeroUs, srvCal[i].minUs, srvCal[i].maxUs, srvCal[i].dir);
}

// --- Khởi tạo ---
// Chỉ cấu hình PWM, chưa xuất xung (servo đứng yên, không giật lúc bật nguồn)
inline bool servoBegin() {
  if (SRV_USE_SAVED_CAL) servoLoadCal();
  bool ok = true;
  for (int i = 0; i < SRV_COUNT; i++) {
    srv[i] = {0, 0, 0, 200, 1200, false};
    ok &= ledcAttach(SRV_PIN[i], SRV_FREQ, SRV_RES_BITS);
    ledcWrite(SRV_PIN[i], 0);
  }
  return ok && xTaskCreate(servoTask, "servo", 3072, NULL, 4, NULL) == pdPASS;
}

// Bật từng servo một, cách nhau 250 ms: tránh 3 servo cùng kéo dòng khởi động làm sụt nguồn LM2596
inline void servoPowerUp(const float pose[SRV_COUNT]) {
  for (int i = 0; i < SRV_COUNT; i++) {
    srv[i].pos = srv[i].target = pose[i];
    srv[i].vel = 0;
    srv[i].on = true;
    delay(250);
  }
}

inline void servoReleaseAll() {
  for (int i = 0; i < SRV_COUNT; i++) { srv[i].on = false; ledcWrite(SRV_PIN[i], 0); }
}

inline void servoSet(int i, float deg) { if (i >= 0 && i < SRV_COUNT) srv[i].target = deg; }
inline void servoSetSpeed(int i, float maxVel, float maxAcc) {
  if (i >= 0 && i < SRV_COUNT) { srv[i].maxVel = maxVel; srv[i].maxAcc = maxAcc; }
}
inline bool servoAllOn() { return srv[0].on && srv[1].on && srv[2].on; }

// --- Chỉnh tay qua Serial ---
enum ServoCalField { CAL_ZERO, CAL_MIN, CAL_MAX };

inline bool servoRaw(int i, int us) {
  if (i < 0 || i >= SRV_COUNT) return false;
  srvRawUs[i] = constrain(us, SRV_US_ABS_MIN, SRV_US_ABS_MAX);
  return true;
}

inline bool servoCalFromRaw(int i, ServoCalField f) {
  if (i < 0 || i >= SRV_COUNT || srvRawUs[i] <= 0) return false;
  int16_t us = srvRawUs[i];
  if (f == CAL_ZERO) srvCal[i].zeroUs = us;
  else if (f == CAL_MIN) srvCal[i].minUs = us;
  else srvCal[i].maxUs = us;
  if (srvCal[i].minUs > srvCal[i].maxUs) { int16_t t = srvCal[i].minUs; srvCal[i].minUs = srvCal[i].maxUs; srvCal[i].maxUs = t; }
  return true;
}

inline bool servoFlip(int i) {
  if (i < 0 || i >= SRV_COUNT) return false;
  srvCal[i].dir = -srvCal[i].dir;
  return true;
}

// Thoát chế độ chỉnh tay: servo quay lại chạy theo kịch bản, xuất phát từ vị trí đang đứng
inline void servoRawOffAll() {
  for (int i = 0; i < SRV_COUNT; i++) {
    if (srvRawUs[i] > 0) {
      srv[i].pos = srvCal[i].dir * (srvRawUs[i] - srvCal[i].zeroUs) / SRV_US_PER_DEG;
      srv[i].vel = 0;
    }
    srvRawUs[i] = 0;
  }
}

#endif