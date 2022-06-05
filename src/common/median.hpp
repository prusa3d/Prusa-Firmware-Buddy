/**
 * @file
 */
#include <stdint.h>

/// Returns index of median of 3 numbers
int median_3_i32(int32_t *nums) {
    // Compare each three number to find middle number
    if (nums[0] > nums[1]) {
        if (nums[1] > nums[2])
            return 1;
        if (nums[0] > nums[2])
            return 2;
        return 0;
    } else {
        if (nums[0] > nums[2])
            return 0;
        if (nums[1] > nums[2])
            return 2;
        return 1;
    }
}
