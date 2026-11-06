#ifndef _FLUXGARAGE_ROBOEYES_HPP
#define _FLUXGARAGE_ROBOEYES_HPP
#include "RoboEyes.h"

template<typename AdafruitDisplay>
void RoboEyes<AdafruitDisplay>::begin(int width, int height, byte frameRate) {
    screenWidth = width; 
    screenHeight = height; 
    canvas = new GFXcanvas16(screenWidth, screenHeight);
    eyeLheightCurrent = 1; 
    eyeRheightCurrent = 1; 
    setFramerate(frameRate); 
}

//loop cap nhat trang thai cua mat
template<typename AdafruitDisplay>
void RoboEyes<AdafruitDisplay>::update() {
    if(millis() - fpsTimer >= frameInterval) {
        drawEyes();
        fpsTimer = millis();
    }
}

template<typename AdafruitDisplay>
void RoboEyes<AdafruitDisplay>::drawEyes() {

  // 1. TÍNH TOÁN LOGIC
  if(curious){
    if(eyeLxNext<=10){eyeLheightOffset=8;}
    else if (eyeLxNext>=(getScreenConstraint_X()-10) && cyclops){eyeLheightOffset=8;}
    else{eyeLheightOffset=0;}   
    if(eyeRxNext>=screenWidth-eyeRwidthCurrent-10){eyeRheightOffset=8;}
    else{eyeRheightOffset=0;} 
  } else {
    eyeLheightOffset=0; 
    eyeRheightOffset=0; 
  }

  eyeLheightCurrent = (eyeLheightCurrent + eyeLheightNext + eyeLheightOffset)/2;
  eyeLy+= ((eyeLheightDefault-eyeLheightCurrent)/2); 
  eyeLy-= eyeLheightOffset/2;
  eyeRheightCurrent = (eyeRheightCurrent + eyeRheightNext + eyeRheightOffset)/2;
  eyeRy+= (eyeRheightDefault-eyeRheightCurrent)/2; 
  eyeRy-= eyeRheightOffset/2;

  if(eyeL_open){ if(eyeLheightCurrent <= 1 + eyeLheightOffset){eyeLheightNext = eyeLheightDefault;} }
  if(eyeR_open){ if(eyeRheightCurrent <= 1 + eyeRheightOffset){eyeRheightNext = eyeRheightDefault;} }

  eyeLwidthCurrent = (eyeLwidthCurrent + eyeLwidthNext)/2;
  eyeRwidthCurrent = (eyeRwidthCurrent + eyeRwidthNext)/2;
  spaceBetweenCurrent = (spaceBetweenCurrent + spaceBetweenNext)/2;

  eyeLx = (eyeLx + eyeLxNext)/2;
  eyeLy = (eyeLy + eyeLyNext)/2;
  eyeRxNext = eyeLxNext+eyeLwidthCurrent+spaceBetweenCurrent; 
  eyeRyNext = eyeLyNext; 
  eyeRx = (eyeRx + eyeRxNext)/2;
  eyeRy = (eyeRy + eyeRyNext)/2;

  eyeLborderRadiusCurrent = (eyeLborderRadiusCurrent + eyeLborderRadiusNext)/2;
  eyeRborderRadiusCurrent = (eyeRborderRadiusCurrent + eyeRborderRadiusNext)/2;
  
  if(autoblinker){
      if(millis() >= blinktimer){
      blink();
      blinktimer = millis()+(blinkInterval*1000)+(random(blinkIntervalVariation)*1000); 
      }
  }

  if(laugh){
    if(laughToggle){ setVFlicker(1, 5); laughAnimationTimer = millis(); laughToggle = 0; } 
    else if(millis() >= laughAnimationTimer+laughAnimationDuration){ setVFlicker(0, 0); laughToggle = 1; laugh=0; }
  }

  if(idle){
    if(millis() >= idleAnimationTimer){
      eyeLxNext = random(getScreenConstraint_X());
      eyeLyNext = random(getScreenConstraint_Y());
      idleAnimationTimer = millis()+(idleInterval*1000)+(random(idleIntervalVariation)*1000); 
    }
  }

  if(hFlicker){
    if(hFlickerAlternate) { eyeLx += hFlickerAmplitude; eyeRx += hFlickerAmplitude; } 
    else { eyeLx -= hFlickerAmplitude; eyeRx -= hFlickerAmplitude; }
    hFlickerAlternate = !hFlickerAlternate;
  }

  if(vFlicker){
    if(vFlickerAlternate) { eyeLy += vFlickerAmplitude; eyeRy += vFlickerAmplitude; } 
    else { eyeLy -= vFlickerAmplitude; eyeRy -= vFlickerAmplitude; }
    vFlickerAlternate = !vFlickerAlternate;
  }

  if(cyclops){
    eyeRwidthCurrent = 0; eyeRheightCurrent = 0; spaceBetweenCurrent = 0;
  }

  // 2. KẾT XUẤT ĐỒ HỌA (DRAW TO CANVAS)
  canvas->fillScreen(BGCOLOR); 

  if (!happy && !crying && !dizzy && !shy && !yawning && !confused && !clapping) {
    canvas->fillRoundRect(eyeLx, eyeLy, eyeLwidthCurrent, eyeLheightCurrent, eyeLborderRadiusCurrent, MAINCOLOR); 
    if (!cyclops){ canvas->fillRoundRect(eyeRx, eyeRy, eyeRwidthCurrent, eyeRheightCurrent, eyeRborderRadiusCurrent, MAINCOLOR); }
  }

  if (sweat){
    if(sweat1YPos <= sweat1YPosMax){sweat1YPos+=0.5;} 
    else {sweat1XPosInitial = random(30); sweat1YPos = 2; sweat1YPosMax = (random(10)+10); sweat1Width = 1; sweat1Height = 2;} 
    if(sweat1YPos <= sweat1YPosMax/2){sweat1Width+=0.5; sweat1Height+=0.5;} 
    else {sweat1Width-=0.1; sweat1Height-=0.5;} 
    sweat1XPos = sweat1XPosInitial-(sweat1Width/2); 
    canvas->fillRoundRect(sweat1XPos, sweat1YPos, sweat1Width, sweat1Height, sweatBorderradius, MAINCOLOR); 

    if(sweat2YPos <= sweat2YPosMax){sweat2YPos+=0.5;} 
    else {sweat2XPosInitial = random((screenWidth-60))+30; sweat2YPos = 2; sweat2YPosMax = (random(10)+10); sweat2Width = 1; sweat2Height = 2;} 
    if(sweat2YPos <= sweat2YPosMax/2){sweat2Width+=0.5; sweat2Height+=0.5;} 
    else {sweat2Width-=0.1; sweat2Height-=0.5;} 
    sweat2XPos = sweat2XPosInitial-(sweat2Width/2); 
    canvas->fillRoundRect(sweat2XPos, sweat2YPos, sweat2Width, sweat2Height, sweatBorderradius, MAINCOLOR); 

    if(sweat3YPos <= sweat3YPosMax){sweat3YPos+=0.5;} 
    else {sweat3XPosInitial = (screenWidth-30)+(random(30)); sweat3YPos = 2; sweat3YPosMax = (random(10)+10); sweat3Width = 1; sweat3Height = 2;} 
    if(sweat3YPos <= sweat3YPosMax/2){sweat3Width+=0.5; sweat3Height+=0.5;} 
    else {sweat3Width-=0.1; sweat3Height-=0.5;} 
    sweat3XPos = sweat3XPosInitial-(sweat3Width/2); 
    canvas->fillRoundRect(sweat3XPos, sweat3YPos, sweat3Width, sweat3Height, sweatBorderradius, MAINCOLOR); 
  }

  // 3. HIỆU ỨNG KHÓC (Crying Effect)
  if (crying) {
    eyeLxNext = (screenWidth - (eyeLwidthCurrent + spaceBetweenCurrent + eyeRwidthCurrent)) / 2;
    eyeLyNext = faceY();

    // --- NẤC THEO NHỊP: giật lên nhanh rồi chùng xuống, nghỉ ---
    const int sobPeriod = 900;                       // Nhịp nấc (ms)
    float sp = (millis() % sobPeriod) / (float)sobPeriod;
    int sobY = (sp < 0.15) ? -(int)(sin(sp / 0.15 * PI) * 6) : 0;

    int ey  = eyeLy + sobY;   // Mắt trái (đã cộng nhịp nấc)
    int eyR = eyeRy + sobY;   // Mắt phải

    const int eyeStroke   = 5;  // Nửa độ dày nét mắt > <  (5 -> nét dày 11px, cũ là 3 -> 7px)
    const int mouthBrush  = 5;  // Bán kính cọ vẽ miệng     (5 -> nét dày ~11px, cũ là 3)
    const int tearMaxR    = 10; // Bán kính giọt lệ to nhất (cũ là 8)

    // --- MẮT > và < (giữ như cũ, bỏ rung liên tục) ---
    for (int i = -eyeStroke; i <= eyeStroke; i++) {
      canvas->drawLine(eyeLx + 10, ey + 15 + i,
                       eyeLx + eyeLwidthCurrent - 10, ey + eyeLheightCurrent / 2 + i, MAINCOLOR);
      canvas->drawLine(eyeLx + 10, ey + eyeLheightCurrent - 15 + i,
                       eyeLx + eyeLwidthCurrent - 10, ey + eyeLheightCurrent / 2 + i, MAINCOLOR);
    }
    if (!cyclops) {
      for (int i = -eyeStroke; i <= eyeStroke; i++) {
        canvas->drawLine(eyeRx + eyeRwidthCurrent - 10, eyR + 15 + i,
                         eyeRx + 10, eyR + eyeRheightCurrent / 2 + i, MAINCOLOR);
        canvas->drawLine(eyeRx + eyeRwidthCurrent - 10, eyR + eyeRheightCurrent - 15 + i,
                         eyeRx + 10, eyR + eyeRheightCurrent / 2 + i, MAINCOLOR);
      }
    }

    // --- GIỌT LỆ: đọng ở mép mắt, to dần, rồi rơi nhanh dần ---
    // Hình giọt nước = hình tròn + tam giác nhọn phía trên
    auto teardrop = [&](int x, int y, int r) {
      canvas->fillCircle(x, y, r, MAINCOLOR);
      canvas->fillTriangle(x - r * 0.85, y - r * 0.5, x + r * 0.85, y - r * 0.5, x, y - r * 2.2, MAINCOLOR);
    };

    // offsetMs lệch pha để 2 giọt không rơi cùng lúc
    auto drawTear = [&](int x, int startY, unsigned long offsetMs) {
      const int tearCycle = 1400;                    // 1 giọt: đọng + rơi (ms)
      float p = ((millis() + offsetMs) % tearCycle) / (float)tearCycle;
      if (p < 0.3) {
        int r = 2 + (int)(p / 0.3 * (tearMaxR - 2)); // Đọng: to dần từ 2 -> tearMaxR
        teardrop(x, startY + r, r);
      } else {
        float f = (p - 0.3) / 0.7;
        int dropY = startY + tearMaxR + (int)(f * f * 120); // Rơi nhanh dần (f bình phương)
        teardrop(x, dropY, tearMaxR);
      }
    };

    int tearStartY = ey + eyeLheightCurrent - 15 + eyeStroke + 4; // Ngay dưới đuôi nét mắt
    int tearXL = eyeLx + 20;                         // Gần mép ngoài mắt trái
    int tearXR = eyeRx + eyeRwidthCurrent - 20;      // Gần mép ngoài mắt phải
    drawTear(tearXL, tearStartY, 0);
    if (!cyclops) drawTear(tearXR, tearStartY, 700);

    // --- MIỆNG VÒM ÚP ∩ RUN RUN ---
    int mouthX = eyeLx + eyeLwidthCurrent + (spaceBetweenCurrent / 2);
    int mouthY = ey + eyeLheightCurrent + 40;
    int mouthW = 50;          // Độ rộng miệng
    int mouthH = 14;          // Độ cong vòm
    float wob = millis() / 90.0;
    for (int i = 0; i <= 25; i++) {
      float u = i / 25.0;
      int px = mouthX - mouthW / 2 + (int)(u * mouthW);
      int py = mouthY - (int)(sin(u * PI) * mouthH) + (int)(sin(u * 12 + wob) * 1.5);
      canvas->fillCircle(px, py, mouthBrush, MAINCOLOR);    // Cọ vẽ bán kính 3 -> nét dày ~7px
    }
  }

  // ====================================================================
  // VUI VẺ: mắt ^^ + nhún nảy đàn hồi + miệng D có lưỡi + má hồng + lấp lánh
  // ====================================================================
  if (happy) {
    eyeLxNext = (screenWidth - (eyeLwidthCurrent + spaceBetweenCurrent + eyeRwidthCurrent)) / 2;
    eyeLyNext = faceY();

    // --- NHÚN NẢY: bật lên rồi rơi xuống, lúc chạm đất hơi bẹp (squash) ---
    const int hopPeriod = 650;                        // Thời gian 1 lần nhún (ms)
    float p = (millis() % hopPeriod) / (float)hopPeriod;
    int hopY = -(int)(sin(p * PI) * 14);              // Độ cao bật lên (px)
    float land = (p < 0.12) ? 1 - p / 0.12 : (p > 0.88) ? (p - 0.88) / 0.12 : 0;
    float sq = land * 0.18;                           // Độ bẹp lúc chạm đất
    int baseY = eyeLy + hopY + (int)(sq * 12);        // Mốc y của cả mặt

    int w = eyeLwidthCurrent;
    int h = eyeLheightCurrent;

    // --- MẮT ^^ MẢNH (quét cung, bè ra khi bẹp) ---
    float arcR = w * 0.38;        // Bán kính cung
    int eyeBrush = 7;             // Độ dày nét (bán kính cọ) -> nét ~14px
    int eyeCy = baseY + h * 3 / 8 + (int)(arcR * 0.5);
    int eyeCxL = eyeLx + w / 2;
    int eyeCxR = eyeRx + eyeRwidthCurrent / 2;
    for (int angle = 200; angle <= 340; angle += 5) {
      float rad = angle * PI / 180.0;
      int ox = (int)(arcR * cos(rad) * (1 + sq));     // Bè ngang khi bẹp
      int oy = (int)(arcR * sin(rad));
      canvas->fillCircle(eyeCxL + ox, eyeCy + oy, eyeBrush, MAINCOLOR);
      if (!cyclops) canvas->fillCircle(eyeCxR + ox, eyeCy + oy, eyeBrush, MAINCOLOR);
    }

    // --- MÁ HỒNG (dưới mắt, lệch ra ngoài) ---
    int cheekY = baseY + h * 3 / 4;
    int cheekW = 32, cheekH = 16;
    canvas->fillRoundRect(eyeCxL - 25 - cheekW / 2, cheekY - cheekH / 2, cheekW, cheekH, cheekH / 2, BLUSHCOLOR);
    if (!cyclops) {
      canvas->fillRoundRect(eyeCxR + 25 - cheekW / 2, cheekY - cheekH / 2, cheekW, cheekH, cheekH / 2, BLUSHCOLOR);
    }

    // --- MIỆNG D + LƯỠI (nửa elip vẽ bằng các đường ngang) ---
    int mouthX = eyeLx + w + spaceBetweenCurrent / 2;
    int mouthY = baseY + h * 7 / 8;                   // Mép trên miệng
    int mW = (int)(26 * (1 + sq * 0.5));              // Nửa độ rộng (bè ra khi bẹp)
    int mH = (int)(26 * (1 - sq * 1.2));              // Độ sâu (dẹt lại khi bẹp)
    for (int dy = 0; dy <= mH; dy++) {
      float f = (float)dy / mH;
      int half = (int)(mW * sqrt(1 - f * f));
      canvas->drawFastHLine(mouthX - half, mouthY + dy, half * 2 + 1, MAINCOLOR);
    }
    int tBaseY = mouthY + mH * 85 / 100;              // Đáy lưỡi
    int tW = mW * 45 / 100, tH = mH * 40 / 100;       // Kích thước lưỡi
    for (int dy = 0; dy <= tH; dy++) {                // Lưỡi = vòm nhô lên từ đáy
      float f = (float)dy / tH;
      int half = (int)(tW * sqrt(1 - f * f));
      canvas->drawFastHLine(mouthX - half, tBaseY - dy, half * 2 + 1, BLUSHCOLOR);
    }

    // --- LẤP LÁNH 4 CÁNH nhấp nháy lần lượt quanh mặt ---
    const int sparkPeriod = 1600;                     // Chu kỳ 1 lần nhấp nháy (ms)
    int sparkPos[5][3] = {                            // {x, y, lệch pha ms}
      {40,                 40,                 0},
      {screenWidth - 35,   55,                 400},
      {30,                 screenHeight - 70,  800},
      {screenWidth - 30,   screenHeight - 60,  1200},
      {screenWidth / 2,    18,                 600},
    };
    for (int k = 0; k < 5; k++) {
      float sp = ((millis() + sparkPos[k][2]) % sparkPeriod) / (float)sparkPeriod;
      if (sp >= 0.5) continue;                        // Nửa sau chu kỳ: tắt
      int s = (int)(sin(sp / 0.5 * PI) * 12);         // Lớn dần rồi nhỏ dần, tối đa 12px
      if (s < 2) continue;
      int sx = sparkPos[k][0], sy = sparkPos[k][1], in = s / 4;
      canvas->fillTriangle(sx, sy - s, sx - in, sy, sx + in, sy, MAINCOLOR); // Cánh trên
      canvas->fillTriangle(sx, sy + s, sx - in, sy, sx + in, sy, MAINCOLOR); // Cánh dưới
      canvas->fillTriangle(sx - s, sy, sx, sy - in, sx, sy + in, MAINCOLOR); // Cánh trái
      canvas->fillTriangle(sx + s, sy, sx, sy - in, sx, sy + in, MAINCOLOR); // Cánh phải
    }
  }

  // ====================================================================
  // CHÓNG MẶT: mắt xoắn ốc @_@ + cả mặt lắc lư + miệng zigzag + sao bay quanh đầu
  // ====================================================================
  if (dizzy) {
    eyeLxNext = (screenWidth - (eyeLwidthCurrent + spaceBetweenCurrent + eyeRwidthCurrent)) / 2;
    eyeLyNext = faceY();

    // --- LẮC LƯ: cả mặt trôi theo vòng tròn lệch (chỉ dịch lúc vẽ, không đổi vị trí mắt thật) ---
    float swayT = millis() / 700.0;            // Tốc độ lắc (nhỏ = nhanh)
    int dx = (int)(cos(swayT) * 10);           // Biên độ ngang
    int dy = (int)(sin(swayT) * 6);            // Biên độ dọc

    // --- MẮT XOẮN ỐC (mắt phải lật gương để 2 mắt đối xứng, cùng xoáy vào trong) ---
    int cxL = eyeLx + eyeLwidthCurrent / 2 + dx;
    int cxR = eyeRx + eyeRwidthCurrent / 2 + dx;
    int cy  = eyeLy + eyeLheightCurrent / 2 + dy;

    float rotationAngle = (millis() / 4.0) * (PI / 180.0); // Tốc độ xoáy (số chia nhỏ = nhanh)
    int   maxRadius     = 35;     // Bán kính xoắn ốc
    float numTurns      = 2.2;    // Số vòng
    int   pointsPerTurn = 60;     // Đủ dày để nét liền ở vòng ngoài
    int   brush         = 4;      // Độ dày nét (bán kính cọ)
    int   totalPoints   = (int)(numTurns * pointsPerTurn);

    for (int i = 0; i < totalPoints; i++) {
      float r = ((float)i / totalPoints) * maxRadius;
      float a = ((float)i / pointsPerTurn) * 2 * PI + rotationAngle;
      int ox = (int)(r * cos(a));
      int oy = (int)(r * sin(a));
      canvas->fillCircle(cxL + ox, cy + oy, brush, MAINCOLOR);
      if (!cyclops) {
        canvas->fillCircle(cxR - ox, cy + oy, brush, MAINCOLOR); // -ox = lật gương
      }
    }

    // --- MIỆNG ZIGZAG méo, run nhẹ ---
    int mouthX = eyeLx + eyeLwidthCurrent + spaceBetweenCurrent / 2 + dx;
    int mouthY = eyeLy + eyeLheightCurrent + 20 + dy;
    int mouthW = 60;           // Độ rộng miệng
    int zigH   = 7;            // Độ cao răng cưa
    int zigN   = 6;            // Số đoạn gấp khúc
    float wob  = millis() / 150.0;
    int prevX = 0, prevY = 0;
    for (int i = 0; i <= zigN; i++) {
      int px = mouthX - mouthW / 2 + i * mouthW / zigN;
      int py = mouthY + ((i % 2) ? -zigH : zigH) + (int)(sin(wob + i) * 2);
      if (i > 0) {
        for (int s = 0; s <= 8; s++) {         // Chấm cọ dọc đoạn thẳng -> nét dày đều
          canvas->fillCircle(prevX + (px - prevX) * s / 8, prevY + (py - prevY) * s / 8, 3, MAINCOLOR);
        }
      }
      prevX = px; prevY = py;
    }

    // --- SAO BAY QUANH ĐẦU (quỹ đạo elip, sao phía trước to hơn) ---
    int orbitCx = mouthX;                       // Giữa mặt (đã cộng lắc lư)
    int orbitCy = eyeLy - 22 + dy;              // Phía trên lông mày
    int orbitRx = 95, orbitRy = 12;             // Bán kính quỹ đạo ngang / dọc
    float orbitT = millis() / 450.0;            // Tốc độ bay

    for (int s = 0; s < 3; s++) {
      float a = orbitT + s * 2 * PI / 3;        // 3 sao cách đều nhau
      float depth = sin(a);                     // -1 = phía sau, +1 = phía trước
      int sx = orbitCx + (int)(cos(a) * orbitRx);
      int sy = orbitCy + (int)(depth * orbitRy);
      int R  = 7 + (int)(depth * 3);            // Bán kính sao: 4 (sau) -> 10 (trước)
      int ri = R * 45 / 100;                    // Bán kính lõm vào giữa các cánh

      for (int k = 0; k < 5; k++) {             // 5 cánh, mỗi cánh = 2 tam giác
        float tip = -PI / 2 + k * 2 * PI / 5;
        float inL = tip - PI / 5, inR = tip + PI / 5;
        int tx = sx + R * cos(tip),  ty = sy + R * sin(tip);
        canvas->fillTriangle(sx, sy, tx, ty, sx + ri * cos(inL), sy + ri * sin(inL), MAINCOLOR);
        canvas->fillTriangle(sx, sy, tx, ty, sx + ri * cos(inR), sy + ri * sin(inR), MAINCOLOR);
      }
    }
  }

  // ====================================================================
  // NGẠI NGÙNG: mắt > < + lắc lư chậm + má hồng /// + miệng sóng nhỏ
  // ====================================================================
  if (shy) {
    eyeLxNext = (screenWidth - (eyeLwidthCurrent + spaceBetweenCurrent + eyeRwidthCurrent)) / 2;
    eyeLyNext = faceY();

    // --- NGÚNG NGUẨY CHẬM (thay cho rung nhanh) ---
    int swayX = (int)(4 * sin(millis() / 500.0)); // Lắc ngang chậm
    int bobY  = (int)(2 * sin(millis() / 700.0)); // Nhún nhẹ lên xuống

    int w = eyeLwidthCurrent;
    int h = eyeLheightCurrent;
    int thickness = 6; // Độ dày nét > <

    // --- MẮT TRÁI ( > ) ---
    int lxL = eyeLx + swayX, lxR = eyeLx + w + swayX;
    int lyT = eyeLy + bobY, lyM = eyeLy + h / 2 + bobY, lyB = eyeLy + h + bobY;
    for (int i = -thickness; i <= thickness; i++) {
      canvas->drawLine(lxL, lyT + i, lxR, lyM + i, MAINCOLOR);
      canvas->drawLine(lxL, lyB + i, lxR, lyM + i, MAINCOLOR);
    }

    // --- MẮT PHẢI ( < ) ---
    if (!cyclops) {
      int rxL = eyeRx + swayX, rxR = eyeRx + eyeRwidthCurrent + swayX;
      int ryT = eyeRy + bobY, ryM = eyeRy + eyeRheightCurrent / 2 + bobY, ryB = eyeRy + eyeRheightCurrent + bobY;
      for (int i = -thickness; i <= thickness; i++) {
        canvas->drawLine(rxR, ryT + i, rxL, ryM + i, MAINCOLOR);
        canvas->drawLine(rxR, ryB + i, rxL, ryM + i, MAINCOLOR);
      }
    }

    // --- MÁ HỒNG + 3 VẠCH /// ---
    int blushY = eyeLy + h + 16 + bobY;   // Ngay dưới mắt
    int blushW = 44, blushH = 20;         // Kích thước má
    auto drawBlush = [&](int cx) {
      canvas->fillRoundRect(cx - blushW / 2, blushY - blushH / 2, blushW, blushH, blushH / 2, BLUSHCOLOR);
      for (int k = -1; k <= 1; k++) {     // 3 vạch chéo màu nền
        int vx = cx + k * 11;
        for (int d = -1; d <= 1; d++) {   // Nét dày 3px
          canvas->drawLine(vx - 4 + d, blushY + 6, vx + 4 + d, blushY - 6, BGCOLOR);
        }
      }
    };
    drawBlush(eyeLx + w / 2 - 10 + swayX);                          // Lệch ra ngoài một chút
    if (!cyclops) drawBlush(eyeRx + eyeRwidthCurrent / 2 + 10 + swayX);

    // --- MIỆNG SÓNG NHỎ (mím ngượng) ---
    int mouthX = eyeLx + w + spaceBetweenCurrent / 2 + swayX;
    int mouthY = eyeLy + h + 36 + bobY;
    int mouthW = 24;                      // Độ rộng miệng
    float wob = millis() / 250.0;         // Tốc độ gợn
    for (int i = 0; i <= 16; i++) {
      float u = i / 16.0;
      int px = mouthX - mouthW / 2 + (int)(u * mouthW);
      int py = mouthY + (int)(sin(u * 2 * PI + wob) * 3);
      canvas->fillCircle(px, py, 3, MAINCOLOR);
    }
  }
  
  //mặt bối rối: nhướn 1 bên mày + mắt nheo + miệng ~ + dấu ?
  static unsigned long confusedStartTime = 0;
  static bool prevConfused = false;
  if (confused && !prevConfused) {
      confusedStartTime = millis(); // Bắt đầu lượt bối rối mới
  }
  prevConfused = confused;

  if (confused) {
    eyeLxNext = (screenWidth - (eyeLwidthCurrent + spaceBetweenCurrent + eyeRwidthCurrent)) / 2;
    eyeLyNext = faceY();

    // k: 0 -> 1 trong 450ms đầu (nhướn mày dần), sau đó giữ nguyên
    unsigned long t = millis() - confusedStartTime;
    float k = min(t / 450.0, 1.0);
    k = k * k * (3 - 2 * k);   // Làm mượt đầu và cuối chuyển động

    int h = eyeLheightCurrent;

    // --- MẮT TRÁI: mở to hơn một chút ---
    int hL = h + (int)(k * 8);
    int yL = eyeLy - (hL - h) / 2;
    canvas->fillRoundRect(eyeLx, yL, eyeLwidthCurrent, hL, eyeLborderRadiusCurrent, MAINCOLOR);

    // --- MẮT PHẢI: nheo lại thành khe ---
    if (!cyclops) {
      int hR = h - (int)(k * h * 0.6);                 // Còn 40% chiều cao khi nheo hết
      int yR = eyeRy + (h - hR) / 2 + (int)(k * 6);    // Hơi trễ xuống
      int rR = min((int)eyeRborderRadiusCurrent, hR / 2);
      canvas->fillRoundRect(eyeRx, yR, eyeRwidthCurrent, hR, rR, MAINCOLOR);
    }

    // --- LÔNG MÀY (nét dày 7px) ---
    const int browStroke = 3;
    for (int i = -browStroke; i <= browStroke; i++) {
      // Mày trái: nhướn cao, xếch lên phía ngoài
      canvas->drawLine(eyeLx + 5,                    eyeLy - 14 - (int)(k * 14) + i,
                       eyeLx + eyeLwidthCurrent - 5, eyeLy - 22 - (int)(k * 14) + i, MAINCOLOR);
      // Mày phải: hạ thấp, chúc xuống phía ngoài
      if (!cyclops) {
        canvas->drawLine(eyeRx + 5,                    eyeRy - 10 + (int)(k * 4)  + i,
                         eyeRx + eyeRwidthCurrent - 5, eyeRy - 6  + (int)(k * 10) + i, MAINCOLOR);
      }
    }

    // --- MIỆNG LƯỢN SÓNG ~ (run nhẹ) ---
    int mouthX = eyeLx + eyeLwidthCurrent + spaceBetweenCurrent / 2;
    int mouthY = eyeLy + h + 32;
    int mouthW = 56;                          // Độ rộng miệng
    float mouthAmp = 4 + k * 2;               // Độ gợn sóng
    float wob = millis() / 220.0;             // Tốc độ run
    for (int i = 0; i <= 28; i++) {
      float u = i / 28.0;
      int px = mouthX - mouthW / 2 + (int)(u * mouthW);
      int py = mouthY + (int)(sin(u * PI * 3 + wob) * mouthAmp);
      canvas->fillCircle(px, py, 3, MAINCOLOR);
    }

    // --- DẤU ? ở góc trên phải: trồi lên rồi nhấp nhô ---
    if (k > 0.3) {
      float q = (k - 0.3) / 0.7;                          // 0 -> 1
      int bob = (int)(sin(millis() / 200.0) * 4);         // Nhấp nhô
      int qx = screenWidth - 45;
      int qy = 8 + (int)((1 - q) * 12) + bob;             // Trồi từ dưới lên
      canvas->setTextColor(MAINCOLOR);
      canvas->setTextSize(q > 0.6 ? 6 : 5);               // Hơi phóng to khi hiện hẳn
      canvas->setCursor(qx, qy);
      canvas->print('?');
    }
  }

  //------------
  //Mood yawning
  //------------
  static unsigned long yawnStartTime = 0;
  static bool prevYawningState = false;

  if (yawning && !prevYawningState) {
      yawnStartTime = millis();
  }
  prevYawningState = yawning; 

  if (yawning) {
    unsigned long t = millis() - yawnStartTime; 
    float progress = 0.0;

    //xu ly ngap
    if (t < 1200) {
        progress = sin((t / 1200.0) * (PI / 2)); 
    } else if (t < 1700) {
        progress = 1.0;
    } else if (t < 2900) {
        progress = cos(((t - 1700) / 1200.0) * (PI / 2));
    } else {
        progress = 0.0;
        yawning = false; //Ket thuc hieu ung ngap
    }

    //Huong nhin cua mat
    eyeLxNext = (screenWidth - (eyeLwidthCurrent + spaceBetweenCurrent + eyeRwidthCurrent)) / 2;
    const int yawnDown = 50; // Hạ cả cụm ngáp xuống (px) để lúc nhướn lên không sát biên trên
    eyeLyNext = faceY() + yawnDown - (int)(progress * 15);

    //Ve 2 mat
    int floatY = (int)(2 * sin(millis() / 250.0)); 
    int eyeUpOffset = (eyeLheightCurrent / 3) + (int)(progress * 10);
    
    int centerLx = eyeLx + (eyeLwidthCurrent / 2);
    int centerLy = eyeLy + (eyeLheightCurrent / 2) - eyeUpOffset + floatY;
    
    int centerRx = eyeRx + (eyeRwidthCurrent / 2);
    int centerRy = eyeRy + (eyeRheightCurrent / 2) - eyeUpOffset + floatY;

    float arcRadius = eyeLwidthCurrent * 0.45;
    int eyeThickness = 4; 

    //Ve mat cong
    float curveFactor = 0.15 + 0.6 * progress; 
    
    for (int angle = 190; angle <= 350; angle += 4) {
        float rad = angle * PI / 180.0;
        int offsetX = arcRadius * cos(rad);
        int offsetY = (arcRadius * sin(rad)) * curveFactor; 
        
        canvas->fillCircle(centerLx + offsetX, centerLy + offsetY, eyeThickness, MAINCOLOR);
        if (!cyclops) {
            canvas->fillCircle(centerRx + offsetX, centerRy + offsetY, eyeThickness, MAINCOLOR);
        }
    }

    //Diem thap nhat cua mat
    int eyeBottomY = centerLy + (int)(arcRadius * sin(190 * PI / 180.0) * curveFactor) + eyeThickness;

    //Ve mieng ngap
    int mouthCenterX = eyeLx + eyeLwidthCurrent + (spaceBetweenCurrent / 2);

    int gap = 8 - (int)(progress * 3); //Khoang cach mat, mieng
    int mouthTopY = eyeBottomY + gap; 
    
    int minW = 24;  int minH = 8; //Mieng ngap
    int maxW = 56;  int maxH = 66; //Ha to
    
    int curMouthW = minW + (int)((maxW - minW) * progress);
    int curMouthH = minH + (int)((maxH - minH) * progress);
    int thickness = 2 + (int)(progress * 3); 
    
    int drawMouthX = mouthCenterX - curMouthW / 2;
    int drawMouthY = mouthTopY; 
    
    canvas->fillRoundRect(drawMouthX, drawMouthY, curMouthW, curMouthH, curMouthW / 2, MAINCOLOR);
    
    int holeW = curMouthW - thickness * 2;
    int holeH = curMouthH - thickness * 2;
    
    if (holeW > 0 && holeH > 0) {
        canvas->fillRoundRect(drawMouthX + thickness, drawMouthY + thickness, holeW, holeH, holeW / 2, BGCOLOR);

        if (progress > 0.4) {
            int tongueW = holeW * 0.6;
            int tongueH = holeH * 0.3;
            canvas->fillRoundRect(mouthCenterX - tongueW / 2, drawMouthY + curMouthH - thickness - tongueH,
                                  tongueW, tongueH, tongueW / 2, MAINCOLOR);
        }
    }
  }

  // ====================================================================
  // VỖ TAY: mặt vui ^^ + hai bàn tay găng vỗ dưới cằm
  // ====================================================================
  if (clapping && !prevClapping) {
      clapStartTime = millis(); // Bắt đầu lượt vỗ tay mới
  }
  prevClapping = clapping;

  if (clapping) {
    const int clapPeriod = 400;  // Thời gian 1 nhịp vỗ (ms), nhỏ = vỗ nhanh
    const int clapCount  = 4;    // Số lần vỗ trong một lượt
    unsigned long t = millis() - clapStartTime;

    if (t >= (unsigned long)(clapPeriod * clapCount)) {
      clapping = false;          // Vỗ đủ số lần -> tự tắt
    }

    // k: 0 = hai tay mở rộng nhất, 1 = chạm nhau (mũ 0.7 cho tay "dính" lâu hơn lúc chạm)
    float phase = (t % clapPeriod) / (float)clapPeriod;
    float k = clapping ? pow(sin(phase * PI), 0.7) : 0;

    // Ép mặt về giữa và nhấc lên chừa chỗ cho tay
    eyeLxNext = (screenWidth - (eyeLwidthCurrent + spaceBetweenCurrent + eyeRwidthCurrent)) / 2;
    eyeLyNext = faceY();

    int bounceY = -(int)(k * 5);                                    // Nảy theo nhịp
    int faceX   = eyeLx + eyeLwidthCurrent + spaceBetweenCurrent / 2; // Trục giữa mặt

    // --- MIỆNG CHỮ D (vẽ trước để phần cắt không đè lên mắt) ---
    int mouthY = eyeLy + eyeLheightCurrent * 3 / 4 + bounceY;
    int mouthR = 14 + (int)(k * 4);
    canvas->fillCircle(faceX, mouthY, mouthR, MAINCOLOR);
    canvas->fillRect(faceX - mouthR - 1, mouthY - mouthR - 1, mouthR * 2 + 3, mouthR + 1, BGCOLOR); // Cắt nửa trên

    // --- MẮT VUI ^^ (quét cung giống mood happy, nét mảnh hơn) ---
    float arcR = eyeLwidthCurrent * 0.4;
    int eyeThick = 5;
    int eyeCyL = eyeLy + eyeLheightCurrent / 2 + (int)(arcR * 0.5) + bounceY;
    int eyeCyR = eyeRy + eyeRheightCurrent / 2 + (int)(arcR * 0.5) + bounceY;
    for (int angle = 200; angle <= 340; angle += 4) {
      float rad = angle * PI / 180.0;
      int ox = arcR * cos(rad);
      int oy = arcR * sin(rad);
      canvas->fillCircle(eyeLx + eyeLwidthCurrent / 2 + ox, eyeCyL + oy, eyeThick, MAINCOLOR);
      if (!cyclops) {
        canvas->fillCircle(eyeRx + eyeRwidthCurrent / 2 + ox, eyeCyR + oy, eyeThick, MAINCOLOR);
      }
    }

    // --- BÀN TAY ---
    // Hình viên thuốc (2 đầu tròn) nối 2 điểm: vẽ được ở góc nghiêng bất kỳ
    auto capsule = [&](float x1, float y1, float x2, float y2, float r) {
      float dx = x2 - x1, dy = y2 - y1;
      float len = sqrt(dx * dx + dy * dy);
      float nx = -dy / len * r, ny = dx / len * r;
      canvas->fillTriangle(x1 + nx, y1 + ny, x2 + nx, y2 + ny, x2 - nx, y2 - ny, MAINCOLOR);
      canvas->fillTriangle(x1 + nx, y1 + ny, x1 - nx, y1 - ny, x2 - nx, y2 - ny, MAINCOLOR);
      canvas->fillCircle(x1, y1, r, MAINCOLOR);
      canvas->fillCircle(x2, y2, r, MAINCOLOR);
    };

    float handScale = 1.3; // Cỡ bàn tay
    // side = +1 tay trái, -1 tay phải (lật gương). tilt > 0 = đầu ngón nghiêng vào giữa
    auto drawHand = [&](float cx, float cy, float tilt, int side) {
      float a = tilt * side, ca = cos(a), sa = sin(a);
      auto seg = [&](float lx1, float ly1, float lx2, float ly2, float r) {
        lx1 *= side * handScale; ly1 *= handScale;
        lx2 *= side * handScale; ly2 *= handScale;
        capsule(cx + lx1 * ca - ly1 * sa, cy + lx1 * sa + ly1 * ca,
                cx + lx2 * ca - ly2 * sa, cy + lx2 * sa + ly2 * ca, r * handScale);
      };
      seg(0, 12, 0, -12, 13);  // Lòng bàn tay
      seg(11, 8, 17, -3, 5);   // Ngón cái (phía trong)
    };

    int handY  = eyeLy + eyeLheightCurrent + 55 + bounceY; // Vị trí tay dưới cằm
    float gap  = 55 * (1 - k) + 15;                         // Khoảng cách từ giữa ra mỗi tay
    float tilt = 0.35 * (1 - k) + 0.1;                      // Mở ra thì nghiêng nhiều, chạm thì gần thẳng
    drawHand(faceX - gap, handY, tilt, 1);
    drawHand(faceX + gap, handY, tilt, -1);

    // --- TIA SÁNG khi hai tay chạm ---
    if (k > 0.85) {
      float sLen = (k - 0.85) / 0.15 * 12;   // Độ dài tia
      int sx = faceX, sy = handY - 36;       // Gốc tia: ngay trên đỉnh hai tay
      int sparkAngles[5] = {-90, -60, -120, -35, -145};
      for (int i = 0; i < 5; i++) {
        float rad = sparkAngles[i] * PI / 180.0;
        for (int w = -1; w <= 1; w++) {      // Nét dày 3 px
          canvas->drawLine(sx + w + 6 * cos(rad), sy + 6 * sin(rad),
                           sx + w + (6 + sLen) * cos(rad), sy + (6 + sLen) * sin(rad), MAINCOLOR);
        }
      }
    }
  }

  display->drawRGBBitmap(0, 0, canvas->getBuffer(), canvas->width(), canvas->height());
}
#endif