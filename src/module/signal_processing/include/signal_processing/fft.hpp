#pragma once

#include <arm_math.h>
#include <cstddef>
#include <cmath>
#include <span>
#include <vector>
#include <bsod/bsod.h>

namespace sp {

class RfftFastF32 {
public:
    explicit RfftFastF32(size_t fft_len) {
        [[maybe_unused]] const arm_status status = arm_rfft_fast_init_f32(&instance, static_cast<uint16_t>(fft_len));
        debug_assert(status == ARM_MATH_SUCCESS);
    }

    void operator()(std::span<float> input, std::span<float> output) {
        debug_assert(input.size() == instance.fftLenRFFT);
        debug_assert(output.size() == instance.fftLenRFFT);
        arm_rfft_fast_f32(&instance, input.data(), output.data(), 0);
    }

private:
    arm_rfft_fast_instance_f32 instance {};
};

} // namespace sp
