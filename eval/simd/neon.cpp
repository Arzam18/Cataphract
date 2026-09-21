#if !defined(__AVX512F__) && !defined(__AVX2__) && \
    (defined(__ARM_NEON) || defined(__ARM_NEON__))

#include <arm_neon.h>

#include "../arch.hpp"
#include "simd.hpp"

namespace NNUE
{
    int32_t forward(const Network& __restrict network,
                    int16_t* __restrict stm,
                    int16_t* __restrict nstm,
                    const uint8_t bucket)
    {
        const int16x8_t vec_zero = vdupq_n_s16(0);
        const int16x8_t vec_QA   = vdupq_n_s16(QA);

        const int16_t* __restrict us_weights   = &network.output_weights[bucket][0];
        const int16_t* __restrict them_weights = &network.output_weights[bucket][HL_SIZE];

        int32x4_t acc_lo = vdupq_n_s32(0);
        int32x4_t acc_hi = vdupq_n_s32(0);

        static constexpr int iters = HL_SIZE / 8;

        for (int i = 0; i < iters; i++)
        {
            const int16x8_t us        = vld1q_s16(stm          + i * 8);
            const int16x8_t them      = vld1q_s16(nstm         + i * 8);
            const int16x8_t us_w      = vld1q_s16(us_weights   + i * 8);
            const int16x8_t them_w    = vld1q_s16(them_weights + i * 8);

            const int16x8_t us_c   = vminq_s16(vmaxq_s16(us,   vec_zero), vec_QA);
            const int16x8_t them_c = vminq_s16(vmaxq_s16(them, vec_zero), vec_QA);

            // Mirror AVX2: mullo_epi16(W, C) then madd_epi16(that, C).
            // This accumulates sum_i (W[i] * C[i] * C[i]) as int32.
            const int16x8_t us_p16   = vmulq_s16(us_w,   us_c);
            const int16x8_t them_p16 = vmulq_s16(them_w, them_c);

            const int32x4_t us_lo   = vmull_s16(vget_low_s16(us_p16),   vget_low_s16(us_c));
            const int32x4_t us_hi   = vmull_s16(vget_high_s16(us_p16),  vget_high_s16(us_c));
            const int32x4_t them_lo = vmull_s16(vget_low_s16(them_p16), vget_low_s16(them_c));
            const int32x4_t them_hi = vmull_s16(vget_high_s16(them_p16),vget_high_s16(them_c));

            acc_lo = vaddq_s32(acc_lo, vaddq_s32(us_lo, them_lo));
            acc_hi = vaddq_s32(acc_hi, vaddq_s32(us_hi, them_hi));
        }

        const int32x4_t total  = vaddq_s32(acc_lo, acc_hi);
        const int32x2_t lo_hi  = vadd_s32(vget_low_s32(total), vget_high_s32(total));
        return vget_lane_s32(lo_hi, 0) + vget_lane_s32(lo_hi, 1);
    }
}

#endif
