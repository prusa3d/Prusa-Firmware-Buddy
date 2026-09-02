#pragma once

// Cholesky factorization and solve for small dense symmetric positive-definite
// systems, as produced by least-squares normal equations.
//
// Matrices are row-major with an explicit row stride, so a caller can keep a
// fixed-capacity buffer and solve a smaller leading n x n block out of it. Only
// the lower triangle is ever read or written: callers that accumulate just that
// half need no mirror, which matters when the buffer is kilobytes on a small
// task stack. Factoring is separate from solving so one factorization can serve
// several right-hand sides.

#include <cmath>
#include <cstddef>

namespace sp {

// A = L*L^T in place, L in the lower triangle of A (the lower triangle is
// always overwritten). Returns false if A is not positive definite (or a
// non-finite value appears); the partial factor left behind is unusable.
inline bool cholesky_factor(float *a, size_t n, size_t stride) {
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j <= i; ++j) {
            float sum = a[i * stride + j];
            for (size_t k = 0; k < j; ++k) {
                sum -= a[i * stride + k] * a[j * stride + k];
            }

            if (i == j) {
                if (!(sum > 0.0f) || !std::isfinite(sum)) {
                    return false;
                }
                a[i * stride + j] = std::sqrt(sum);
            } else {
                if (!std::isfinite(sum)) {
                    return false;
                }
                a[i * stride + j] = sum / a[j * stride + j];
            }
        }
    }
    return true;
}

// Solve L*L^T*x = b for the factor L produced by cholesky_factor. `b` may be a
// different type than the matrix (normal equations often accumulate the
// right-hand side in double to avoid cancellation). Returns false on a
// non-finite intermediate, in which case x is left partially overwritten.
//
// x needs n elements. x == b is allowed (step i reads b[i] before writing
// x[i]); partial overlap is not. No scratch buffer is needed: the forward
// substitution leaves y in x, and the back substitution consumes y[i] before
// overwriting x[i].
template <typename T>
inline bool cholesky_solve(const float *l, const T *b, size_t n, size_t stride, float *x) {
    for (size_t i = 0; i < n; ++i) {
        float sum = static_cast<float>(b[i]);
        for (size_t k = 0; k < i; ++k) {
            sum -= l[i * stride + k] * x[k];
        }
        x[i] = sum / l[i * stride + i];
        if (!std::isfinite(x[i])) {
            return false;
        }
    }

    // Back substitution with L^T, read through the lower triangle:
    // L^T(i,k) == l[k*stride + i], so no transpose is materialized.
    for (size_t ii = n; ii-- > 0;) {
        float sum = x[ii];
        for (size_t k = ii + 1; k < n; ++k) {
            sum -= l[k * stride + ii] * x[k];
        }
        x[ii] = sum / l[ii * stride + ii];
        if (!std::isfinite(x[ii])) {
            return false;
        }
    }
    return true;
}

} // namespace sp
