#ifndef MIC_FILTER_H
#define MIC_FILTER_H
#include <math.h>

struct Biquad {
  float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
  float z1 = 0, z2 = 0;
  bool on = false;

  //fc: Tan so cat
  //fs: Tan so lay mau
  //q: He so chat luong
  void setHighpass(float fs, float fc, float q = 0.7071f) { design(0, fs, fc, q); } //Thong cao
  void setLowpass (float fs, float fc, float q = 0.7071f) { design(1, fs, fc, q); } //Thong thap
  void setNotch (float fs, float fc, float q = 10.0f) { design(2, fs, fc, q); } //Chan hep

  void design(int type, float fs, float fc, float q) {
    if (fc <= 0 || fc >= fs * 0.5f) { on = false; return; }
    float w = 2.0f * (float)M_PI * fc / fs;
    float c = cosf(w);
    float alpha = sinf(w) / (2.0f * q);
    float a0 = 1.0f + alpha;
    float nb0, nb1, nb2;
    switch (type) {
      case 0: //Thong cao
      nb0 = (1 + c) / 2;
      nb1 = -(1 + c);
      nb2 = (1 + c) / 2;
      break;
      case 1: //Thong thap
      nb0 = (1 - c) / 2;
      nb1 =  (1 - c);
      nb2 = (1 - c) / 2;
      break;
      default: //Chan hep
      nb0 = 1;
      nb1 = -2 * c;
      nb2 = 1;
      break;
    }
    b0 = nb0 / a0;
    b1 = nb1 / a0;
    b2 = nb2 / a0;
    a1 = -2 * c / a0;
    a2 = (1 - alpha) / a0;
    z1 = z2 = 0;
    on = true;
  }

  //Loc 1 mau, tra ve gia tri da loc
  inline float process(float x) {
    if (!on) return x;
    float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
  }
};

#endif