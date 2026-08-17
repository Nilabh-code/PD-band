#pragma once
#include <math.h>

// Iterative radix-2 Cooley-Tukey FFT (in-place, real input).
// N must be a power of two. Output bins [k] => freq = k * fs / N.
namespace FFT {

template <int N>
void bitReverse(float* x) {
  for (int i = 1, j = 0; i < N; i++) {
    int bit = N >> 1;
    for (; j & bit; bit >>= 1) j ^= bit;
    j ^= bit;
    if (i < j) { float t = x[i]; x[i] = x[j]; x[j] = t; }
  }
}

template <int N>
void transform(float* re, float* im) {
  bitReverse<N>(re);
  bitReverse<N>(im);
  for (int len = 2; len <= N; len <<= 1) {
    float ang = -2.0f * PI / len;
    float wl_re = cosf(ang), wl_im = sinf(ang);
    for (int i = 0; i < N; i += len) {
      float w_re = 1, w_im = 0;
      for (int j = 0; j < len / 2; j++) {
        int u_i = i + j, p_i = i + j + len / 2;
        float u_re = re[u_i], u_im = im[u_i];
        float p_re = re[p_i] * w_re - im[p_i] * w_im;
        float p_im = re[p_i] * w_im + im[p_i] * w_re;
        re[u_i] = u_re + p_re; im[u_i] = u_im + p_im;
        re[p_i] = u_re - p_re; im[p_i] = u_im - p_im;
        float nw_re = w_re * wl_re - w_im * wl_im;
        w_im = w_re * wl_im + w_im * wl_re;
        w_re = nw_re;
      }
    }
  }
}

}

#define FFT_N 256

class Spectrum {
public:
  void begin(float fs_) {
    fs = fs_;
    df = fs / FFT_N;
    for (int i = 0; i < FFT_N; i++)
      win[i] = 0.54f - 0.46f * cosf(2.0f * PI * i / (FFT_N - 1));
  }

  void push(float x) {
    buf[idx] = x;
    idx = (idx + 1) % FFT_N;
  }

  // compute magnitude spectrum (g units), skip bin 0
  void analyze() {
    for (int i = 0; i < FFT_N; i++) {
      re[i] = buf[(idx + i) % FFT_N] * win[i];
      im[i] = 0;
    }
    FFT::transform<FFT_N>(re, im);
    for (int k = 0; k < FFT_N / 2; k++) {
      mag[k] = sqrtf(re[k] * re[k] + im[k] * im[k]) * 2.0f / FFT_N;
    }
    dom_amp = 0; dom_freq = 0;
    for (int k = 2; k < FFT_N / 2; k++) {  // skip DC + bin1
      if (mag[k] > dom_amp) { dom_amp = mag[k]; dom_freq = k * df; }
    }
  }

  float binFreq(int k) { return k * df; }
  float binAmp(int k) { return k >= 0 && k < FFT_N / 2 ? mag[k] : 0; }
  float dominantFreq() { return dom_freq; }
  float dominantAmp() { return dom_amp; }

private:
  float fs = 100, df = 0;
  float buf[FFT_N] = {0};
  float re[FFT_N], im[FFT_N], mag[FFT_N / 2], win[FFT_N];
  int idx = 0;
  float dom_freq = 0, dom_amp = 0;
};
