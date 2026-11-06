#ifndef ROBOT_BEHAVIOR_H
#define ROBOT_BEHAVIOR_H
// ====================================================================
// KỊCH BẢN ĐẦU + TAY + ÂM THANH theo từng biểu cảm của mắt
// Mỗi servo chỉ xoay quanh trục của nó:
//   đầu: quay trái/phải. 0 = nhìn thẳng, số dương = quay sang phải robot
//   tay: vung quanh vai. 0 = nghỉ sát thân, số dương = vung ra phía trước (tối đa khoảng 90°)
// Các nhịp lấy đúng từ RoboEyes.hpp để chuyển động khớp với hình:
//   vui: nhún 650 ms · khóc: nấc 900 ms · chóng mặt: lắc 700 ms · ngại: 500 ms
//   bối rối: nhướn mày 450 ms · ngáp: 1200 + 500 + 1200 ms · vỗ tay: 4 nhịp × 400 ms
// File này dùng SystemState, robot và stateStartTime khai báo trong main.cpp,
// nên phải #include sau các khai báo đó.
// ====================================================================
#include "ServoCtl.h"
#include "Audio.h"

#define ARM_REST 5.0f   // Tay nghỉ hơi tách khỏi thân, không tì vào giới hạn cơ khí

static const float POSE_REST[SRV_COUNT]  = {0, ARM_REST, ARM_REST};
static const float POSE_SLEEP[SRV_COUNT] = {15, 0, 0};

static inline void head(float deg)            { servoSet(SRV_HEAD, deg); }
static inline void arms(float l, float r)     { servoSet(SRV_ARM_L, l); servoSet(SRV_ARM_R, r); }
static inline float smooth01(float k)         { k = constrain(k, 0.0f, 1.0f); return k * k * (3 - 2 * k); }

static bool behSleepReleased = false;
static int  behLastClapHit = -1;
static unsigned long behNextLook = 0, behLookEnd = 0;
static float behLook = 0;

// Gọi một lần khi đổi trạng thái: phát âm, đặt tốc độ servo
inline void behaviorEnter(SystemState s, SystemState from) {
  if (from == STATE_SLEEP && s != STATE_SLEEP) servoPowerUp(POSE_SLEEP);   // thức dậy: bật lại servo từng con
  behSleepReleased = false;
  behLastClapHit = -1;
  for (int i = 0; i < SRV_COUNT; i++) servoSetSpeed(i, 200, 1200);   // tốc độ thường, nhẹ cho nguồn
  switch (s) {
    case STATE_BOOT:     audioPlay(SND_BOOT);      break;
    case STATE_HAPPY:    audioPlay(SND_HAPPY);     break;
    case STATE_CRYING:   audioPlay(SND_CRY);       break;
    case STATE_SHY:      audioPlay(SND_SHY, 0.6f); break;   // tiếng lệch sang phía quay mặt đi
    case STATE_CONFUSED: audioPlay(SND_CONFUSED);  break;
    case STATE_DIZZY:    audioPlay(SND_DIZZY);     break;
    case STATE_YAWNING:  audioPlay(SND_YAWN);      break;
    case STATE_SLEEP:    audioPlay(SND_SLEEP);     break;
    case STATE_CLAPPING:
      servoSetSpeed(SRV_ARM_L, 280, 2500);          // vỗ tay cần nhanh hơn
      servoSetSpeed(SRV_ARM_R, 280, 2500);
      break;
    default: break;
  }
}

// Gọi liên tục trong loop(): t = số ms từ lúc vào trạng thái
inline void behaviorUpdate(SystemState s, unsigned long t) {
  unsigned long now = millis();
#if SRV_MOUNT_MODE
  // Chế độ lắp càng (xem ServoCtl.h): giữ góc 0 trong 4,5 giây, rồi lần lượt đầu, tay trái, tay phải xoay +20°
  unsigned long c = now % 9000;
  head(c >= 4500 && c < 6000 ? 20 : 0);
  arms(c >= 6000 && c < 7500 ? 20 : 0, c >= 7500 ? 20 : 0);
  return;
#endif
  switch (s) {
    case STATE_BOOT: {             // nhìn trái, nhìn phải rồi nhìn thẳng; hai tay vung ra trước chào
      head(t < 500 ? -25 : t < 1100 ? 25 : 0);
      float up = t < 700 ? smooth01(t / 700.0f) : (t < 1300 ? 1 : 1 - smooth01((t - 1300) / 700.0f));
      arms(ARM_REST + 70 * up, ARM_REST + 70 * up);
      break;
    }
    case STATE_DEFAULT: {          // tay đung đưa nhẹ, thỉnh thoảng quay đầu liếc sang một bên
      float sway = 3 * sinf(2 * PI * now / 4000.0f);
      arms(ARM_REST + sway, ARM_REST - sway);
      if (now >= behNextLook) {
        behLook = random(-25, 26);
        behLookEnd = now + 1200;
        behNextLook = now + random(4000, 8000);
      }
      head(now < behLookEnd ? behLook : 0);
      break;
    }
    case STATE_HAPPY: {            // lắc đầu qua lại theo nhịp nhún, hai tay vung so le
      float w = sinf(2 * PI * (now % 650) / 650.0f);
      head(10 * w);
      arms(45 + 30 * w, 45 - 30 * w);
      break;
    }
    case STATE_CRYING: {           // lắc đầu chậm, giật theo nhịp nấc; hai tay khép ra phía trước
      float sp = (now % 900) / 900.0f;
      float sob = sp < 0.15f ? sinf(sp / 0.15f * PI) : 0;
      head(12 * sinf(2 * PI * now / 1800.0f));
      arms(75 + 6 * sob, 75 + 6 * sob);
      break;
    }
    case STATE_SHY: {              // quay mặt đi, lắc chậm; tay phải đưa ra trước, tay trái nép sát
      head(30 + 6 * sinf(now / 500.0f));
      arms(ARM_REST, 60 + 5 * sinf(now / 500.0f));
      break;
    }
    case STATE_CONFUSED: {         // quay đầu sang một bên theo mày nhướn, tay phải đưa ra ngoe nguẩy
      float k = smooth01(t / 450.0f);
      head(-20 * k);
      float wiggle = k > 0.6f ? 12 * sinf(2 * PI * t / 280.0f) : 0;
      arms(ARM_REST, 55 * k + wiggle);
      break;
    }
    case STATE_DIZZY: {            // lắc đầu cùng nhịp mặt trên màn hình, tay đung đưa lỏng
      float a = now / 700.0f;
      head(25 * cosf(a));
      arms(35 + 25 * sinf(a + 0.5f), 35 + 25 * sinf(a + 2.0f));
      break;
    }
    case STATE_YAWNING: {          // hai tay vung ra trước theo độ há miệng, đầu hơi ngoảnh đi
      float p = t < 1200 ? sinf(t / 1200.0f * PI / 2) : t < 1700 ? 1.0f
              : t < 2900 ? cosf((t - 1700) / 1200.0f * PI / 2) : 0.0f;
      head(-10 * p);
      arms(ARM_REST + 80 * p, ARM_REST + 80 * p);
      break;
    }
    case STATE_CLAPPING: {         // vỗ đúng nhịp tay trên màn hình, mỗi lần chạm kêu "bộp"
      if (!robot.clapping || robot.clapStartTime < stateStartTime) { arms(40, 40); head(0); break; }
      unsigned long ct = now - robot.clapStartTime;
      float k = powf(sinf((ct % 400) / 400.0f * PI), 0.7f);
      arms(40 + 45 * k, 40 + 45 * k);
      int hit = ct / 400;
      head(hit % 2 ? 6 * k : -6 * k);   // mỗi lần vỗ lắc đầu sang một phía
      if (ct % 400 >= 200 && hit != behLastClapHit) { behLastClapHit = hit; audioPlay(SND_CLAP); }
      break;
    }
    case STATE_SLEEP: {            // ngoảnh đầu sang một bên, hạ tay; 1,5 giây sau thả lỏng servo cho đỡ tốn pin
      head(POSE_SLEEP[SRV_HEAD]);
      arms(0, 0);
      if (t > 1500 && !behSleepReleased) { servoReleaseAll(); behSleepReleased = true; }
      break;
    }
  }
}

#endif