// Copyright 2014 Google Inc. All Rights Reserved.
//
// Use of this source code is governed by a BSD-style license
// that can be found in the COPYING file in the root of the source
// tree. An additional intellectual property rights grant can be found
// in the file PATENTS. All contributing project authors may
// be found in the AUTHORS file in the root of the source tree.
// -----------------------------------------------------------------------------
//
// Utilities for processing transparent channel.
//
// Author: Skal (pascal.massimino@gmail.com)

#include "./dsp.h"

#if defined(WEBP_USE_SSE2)
#include <emmintrin.h>

//------------------------------------------------------------------------------

static int DispatchAlpha(const uint8_t* alpha, int alpha_stride,
                         int width, int height,
                         uint8_t* dst, int dst_stride) {
  // alpha_and stores an 'and' operation of all the alpha[] values. The final
  // value is not 0xff if any of the alpha[] is not equal to 0xff.
  uint32_t alpha_and = 0xff;
  int i, j;
  const __m128i zero = _mm_setzero_si128();
  const __m128i rgb_mask = _mm_set1_epi32(0xffffff00u);  // to preserve RGB
  const __m128i all_0xff = _mm_set_epi32(0, 0, ~0u, ~0u);
  __m128i all_alphas = all_0xff;

  // We must be able to access 3 extra bytes after the last written byte
  // 'dst[4 * width - 4]', because we don't know if alpha is the first or the
  // last byte of the quadruplet.
  const int limit = (width - 1) & ~7;

  for (j = 0; j < height; ++j) {
    __m128i* out = (__m128i*)dst;
    for (i = 0; i < limit; i += 8) {
      // load 8 alpha bytes
      const __m128i a0 = _mm_loadl_epi64((const __m128i*)&alpha[i]);
      const __m128i a1 = _mm_unpacklo_epi8(a0, zero);
      const __m128i a2_lo = _mm_unpacklo_epi16(a1, zero);
      const __m128i a2_hi = _mm_unpackhi_epi16(a1, zero);
      // load 8 dst pixels (32 bytes)
      const __m128i b0_lo = _mm_loadu_si128(out + 0);
      const __m128i b0_hi = _mm_loadu_si128(out + 1);
      // mask dst alpha values
      const __m128i b1_lo = _mm_and_si128(b0_lo, rgb_mask);
      const __m128i b1_hi = _mm_and_si128(b0_hi, rgb_mask);
      // combine
      const __m128i b2_lo = _mm_or_si128(b1_lo, a2_lo);
      const __m128i b2_hi = _mm_or_si128(b1_hi, a2_hi);
      // store
      _mm_storeu_si128(out + 0, b2_lo);
      _mm_storeu_si128(out + 1, b2_hi);
      // accumulate eight alpha 'and' in parallel
      all_alphas = _mm_and_si128(all_alphas, a0);
      out += 2;
    }
    for (; i < width; ++i) {
      const uint32_t alpha_value = alpha[i];
      dst[4 * i] = alpha_value;
      alpha_and &= alpha_value;
    }
    alpha += alpha_stride;
    dst += dst_stride;
  }
  // Combine the eight alpha 'and' into a 8-bit mask.
  alpha_and &= _mm_movemask_epi8(_mm_cmpeq_epi8(all_alphas, all_0xff));
  return (alpha_and != 0xff);
}

static void DispatchAlphaToGreen(const uint8_t* alpha, int alpha_stride,
                                 int width, int height,
                                 uint32_t* dst, int dst_stride) {
  int i, j;
  const __m128i zero = _mm_setzero_si128();
  const int limit = width & ~15;
  for (j = 0; j < height; ++j) {
    for (i = 0; i < limit; i += 16) {   // process 16 alpha bytes
      const __m128i a0 = _mm_loadu_si128((const __m128i*)&alpha[i]);
      const __m128i a1 = _mm_unpacklo_epi8(zero, a0);  // note the 'zero' first!
      const __m128i b1 = _mm_unpackhi_epi8(zero, a0);
      const __m128i a2_lo = _mm_unpacklo_epi16(a1, zero);
      const __m128i b2_lo = _mm_unpacklo_epi16(b1, zero);
      const __m128i a2_hi = _mm_unpackhi_epi16(a1, zero);
      const __m128i b2_hi = _mm_unpackhi_epi16(b1, zero);
      _mm_storeu_si128((__m128i*)&dst[i +  0], a2_lo);
      _mm_storeu_si128((__m128i*)&dst[i +  4], a2_hi);
      _mm_storeu_si128((__m128i*)&dst[i +  8], b2_lo);
      _mm_storeu_si128((__m128i*)&dst[i + 12], b2_hi);
    }
    for (; i < width; ++i) dst[i] = alpha[i] << 8;
    alpha += alpha_stride;
    dst += dst_stride;
  }
}

static int ExtractAlpha(const uint8_t* argb, int argb_stride,
                        int width, int height,
                        uint8_t* alpha, int alpha_stride) {
  // alpha_and stores an 'and' operation of all the alpha[] values. The final
  // value is not 0xff if any of the alpha[] is not equal to 0xff.
  uint32_t alpha_and = 0xff;
  int i, j;
  const __m128i a_mask = _mm_set1_epi32(0xffu);  // to preserve alpha
  const __m128i all_0xff = _mm_set_epi32(0, 0, ~0u, ~0u);
  __m128i all_alphas = all_0xff;

  // We must be able to access 3 extra bytes after the last written byte
  // 'src[4 * width - 4]', because we don't know if alpha is the first or the
  // last byte of the quadruplet.
  const int limit = (width - 1) & ~7;

  for (j = 0; j < height; ++j) {
    const __m128i* src = (const __m128i*)argb;
    for (i = 0; i < limit; i += 8) {
      // load 32 argb bytes
      const __m128i a0 = _mm_loadu_si128(src + 0);
      const __m128i a1 = _mm_loadu_si128(src + 1);
      const __m128i b0 = _mm_and_si128(a0, a_mask);
      const __m128i b1 = _mm_and_si128(a1, a_mask);
      const __m128i c0 = _mm_packs_epi32(b0, b1);
      const __m128i d0 = _mm_packus_epi16(c0, c0);
      // store
      _mm_storel_epi64((__m128i*)&alpha[i], d0);
      // accumulate eight alpha 'and' in parallel
      all_alphas = _mm_and_si128(all_alphas, d0);
      src += 2;
    }
    for (; i < width; ++i) {
      const uint32_t alpha_value = argb[4 * i];
      alpha[i] = alpha_value;
      alpha_and &= alpha_value;
    }
    argb += argb_stride;
    alpha += alpha_stride;
  }
  // Combine the eight alpha 'and' into a 8-bit mask.
  alpha_and &= _mm_movemask_epi8(_mm_cmpeq_epi8(all_alphas, all_0xff));
  return (alpha_and == 0xff);
}

//------------------------------------------------------------------------------
// Non-dither premultiplied modes

#define MULTIPLIER(a)   ((a) * 0x8081)
#define PREMULTIPLY(x, m) (((x) * (m)) >> 23)

// We can't use a 'const int' for the SHUFFLE value, because it has to be an
// immediate in the _mm_shufflexx_epi16() instruction. We really need a macro.
// We use: v / 255 = (v + 1 + (v >> 8)) >> 8, where v = alpha * {r,g,b} is
// a 16bit value.
#define APPLY_ALPHA(RGBX, SHUFFLE) do {                           \
  const __m128i argb0 = _mm_loadl_epi64((const __m128i*)&(RGBX)); \
  const __m128i argb1 = _mm_unpacklo_epi8(argb0, zero);           \
  const __m128i alpha0 = _mm_or_si128(argb1, kMask);              \
  const __m128i alpha1 = _mm_shufflelo_epi16(alpha0, SHUFFLE);    \
  const __m128i alpha2 = _mm_shufflehi_epi16(alpha1, SHUFFLE);    \
  /* alpha2 = [ff a0 a0 a0][ff a1 a1 a1] */                       \
  const __m128i A0 = _mm_mullo_epi16(alpha2, argb1);              \
  const __m128i A1 = _mm_srli_epi16(A0, 8);                       \
  const __m128i A2 = _mm_add_epi16(A1, A0);                       \
  const __m128i A3 = _mm_add_epi16(A2, one);                      \
  const __m128i A4 = _mm_srli_epi16(A3, 8);                       \
  const __m128i A5 = _mm_packus_epi16(A4, zero);                  \
  _mm_storel_epi64((__m128i*)&(RGBX), A5);                        \
} while (0)

static void ApplyAlphaMultiply_SSE2(uint8_t* rgba, int alpha_first,
                                    int w, int h, int stride) {
  const __m128i zero = _mm_setzero_si128();
  const __m128i one = _mm_set1_epi16(1);
  const __m128i kMask = _mm_set_epi16(0, 0xff, 0xff, 0, 0, 0xff, 0xff, 0);
  const int kSpan = 2;
  while (h-- > 0) {
    uint32_t* const rgbx = (uint32_t*)rgba;
    int i;
    if (!alpha_first) {
      for (i = 0; i + kSpan <= w; i += kSpan) {
        APPLY_ALPHA(rgbx[i], _MM_SHUFFLE(2, 3, 3, 3));
      }
    } else {
      for (i = 0; i + kSpan <= w; i += kSpan) {
        APPLY_ALPHA(rgbx[i], _MM_SHUFFLE(0, 0, 0, 1));
      }
    }
    // Finish with left-overs.
    for (; i < w; ++i) {
      uint8_t* const rgb = rgba + (alpha_first ? 1 : 0);
      const uint8_t* const alpha = rgba + (alpha_first ? 0 : 3);
      const uint32_t a = alpha[4 * i];
      if (a != 0xff) {
        const uint32_t mult = MULTIPLIER(a);
        rgb[4 * i + 0] = PREMULTIPLY(rgb[4 * i + 0], mult);
        rgb[4 * i + 1] = PREMULTIPLY(rgb[4 * i + 1], mult);
        rgb[4 * i + 2] = PREMULTIPLY(rgb[4 * i + 2], mult);
      }
    }
    rgba += stride;
  }
}
#undef MULTIPLIER
#undef PREMULTIPLY

// -----------------------------------------------------------------------------
// Apply alpha value to rows

static void MultARGBRow_SSE2(uint32_t* const ptr, int width, int inverse) {
  int x = 0;
  if (!inverse) {
    const int kSpan = 2;
    const __m128i zero = _mm_setzero_si128();
    const __m128i k128 = _mm_set1_epi16(128);
    const __m128i kMask = _mm_set_epi16(0, 0xff, 0, 0, 0, 0xff, 0, 0);
    for (x = 0; x + kSpan <= width; x += kSpan) {
      // To compute 'result = (int)(a * x / 255. + .5)', we use:
      //   tmp = a * v + 128, result = (tmp + (tmp >> 8)) >> 8
      const __m128i A0 = _mm_loadl_epi64((const __m128i*)&ptr[x]);
      const __m128i A1 = _mm_unpacklo_epi8(A0, zero);
      const __m128i A2 = _mm_or_si128(A1, kMask);
      const __m128i A3 = _mm_shufflelo_epi16(A2, _MM_SHUFFLE(2, 3, 3, 3));
      const __m128i A4 = _mm_shufflehi_epi16(A3, _MM_SHUFFLE(2, 3, 3, 3));
      // here, A4 = [ff a0 a0 a0][ff a1 a1 a1]
      const __m128i A5 = _mm_mullo_epi16(A4, A1);
      const __m128i A6 = _mm_add_epi16(A5, k128);
      const __m128i A7 = _mm_srli_epi16(A6, 8);
      const __m128i A8 = _mm_add_epi16(A7, A6);
      const __m128i A9 = _mm_srli_epi16(A8, 8);
      const __m128i A10 = _mm_packus_epi16(A9, zero);
      _mm_storel_epi64((__m128i*)&ptr[x], A10);
    }
  }
  width -= x;
  if (width > 0) WebPMultARGBRowC(ptr + x, width, inverse);
}

static void MultRow_SSE2(uint8_t* const ptr, const uint8_t* const alpha,
                         int width, int inverse) {
  int x = 0;
  if (!inverse) {
    const int kSpan = 8;
    const __m128i zero = _mm_setzero_si128();
    const __m128i kRound = _mm_set1_epi16(1 << 7);
    const int w2 = width & ~(kSpan - 1);
    for (x = 0; x < w2; x += kSpan) {
      const __m128i v0 = _mm_loadl_epi64((__m128i*)&ptr[x]);
      const __m128i v1 = _mm_unpacklo_epi8(v0, zero);
      const __m128i alpha0 = _mm_loadl_epi64((const __m128i*)&alpha[x]);
      const __m128i alpha1 = _mm_unpacklo_epi8(alpha0, zero);
      const __m128i alpha2 = _mm_unpacklo_epi8(alpha0, alpha0);
      const __m128i v2 = _mm_mulhi_epu16(v1, alpha2);
      const __m128i v3 = _mm_mullo_epi16(v1, alpha1);
      const __m128i v4 = _mm_adds_epu16(v2, v3);
      const __m128i v5 = _mm_adds_epu16(v4, kRound);
      const __m128i v6 = _mm_srli_epi16(v5, 8);
      const __m128i v7 = _mm_packus_epi16(v6, zero);
      _mm_storel_epi64((__m128i*)&ptr[x], v7);
    }
  }
  width -= x;
  if (width > 0) WebPMultRowC(ptr + x, alpha + x, width, inverse);
}

//------------------------------------------------------------------------------
// Entry point

extern void WebPInitAlphaProcessingSSE2(void);

WEBP_TSAN_IGNORE_FUNCTION void WebPInitAlphaProcessingSSE2(void) {
  WebPMultARGBRow = MultARGBRow_SSE2;
  WebPMultRow = MultRow_SSE2;
  WebPApplyAlphaMultiply = ApplyAlphaMultiply_SSE2;
  WebPDispatchAlpha = DispatchAlpha;
  WebPDispatchAlphaToGreen = DispatchAlphaToGreen;
  WebPExtractAlpha = ExtractAlpha;
}

#else  // !WEBP_USE_SSE2

WEBP_DSP_INIT_STUB(WebPInitAlphaProcessingSSE2)

#endif  // WEBP_USE_SSE2
