#include <signal_processing/cholesky.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>

using Catch::Approx;
using namespace sp;

namespace {

// Fill the lower triangle of a leading n x n block; the rest of the buffer is
// left as poison so a stride mistake shows up as a wrong answer.
template <size_t Stride, size_t N>
std::array<float, Stride * Stride> lower_triangle(const std::array<std::array<float, N>, N> &m) {
    std::array<float, Stride * Stride> a;
    a.fill(-999.0f);
    for (size_t i = 0; i < N; ++i) {
        for (size_t j = 0; j <= i; ++j) {
            a[i * Stride + j] = m[i][j];
        }
    }
    return a;
}

} // namespace

TEST_CASE("cholesky solves a known SPD system", "[cholesky]") {
    // A = [[4,2,1],[2,5,3],[1,3,6]], b chosen so x = [1,2,3].
    // A*x = [4+4+3, 2+10+9, 1+6+18] = [11, 21, 25]
    constexpr size_t kStride = 6; // deliberately larger than n
    auto a = lower_triangle<kStride, 3>({ { { { 4, 0, 0 } }, { { 2, 5, 0 } }, { { 1, 3, 6 } } } });
    const std::array<float, 3> b { 11, 21, 25 };
    std::array<float, 3> x {};

    REQUIRE(cholesky_factor(a.data(), 3, kStride));
    REQUIRE(cholesky_solve(a.data(), b.data(), 3, kStride, x.data()));

    CHECK(x[0] == Approx(1.0f).margin(1e-5));
    CHECK(x[1] == Approx(2.0f).margin(1e-5));
    CHECK(x[2] == Approx(3.0f).margin(1e-5));
}

TEST_CASE("cholesky reuses one factor for several right-hand sides", "[cholesky]") {
    // The lock-in accumulator solves the same normal matrix against X^T u and
    // X^T y, so a factor has to survive more than one solve.
    constexpr size_t kStride = 4;
    auto a = lower_triangle<kStride, 2>({ { { { 2, 0 } }, { { 1, 3 } } } });
    REQUIRE(cholesky_factor(a.data(), 2, kStride));

    // A = [[2,1],[1,3]]; A*[1,1] = [3,4]; A*[2,-1] = [3,-1].
    const std::array<float, 2> b1 { 3, 4 };
    const std::array<float, 2> b2 { 3, -1 };
    std::array<float, 2> x {};

    REQUIRE(cholesky_solve(a.data(), b1.data(), 2, kStride, x.data()));
    CHECK(x[0] == Approx(1.0f).margin(1e-5));
    CHECK(x[1] == Approx(1.0f).margin(1e-5));

    REQUIRE(cholesky_solve(a.data(), b2.data(), 2, kStride, x.data()));
    CHECK(x[0] == Approx(2.0f).margin(1e-5));
    CHECK(x[1] == Approx(-1.0f).margin(1e-5));
}

TEST_CASE("cholesky accepts a double right-hand side", "[cholesky]") {
    // The lock-in accumulates X^T y in double to keep the residual free of
    // catastrophic cancellation.
    constexpr size_t kStride = 3;
    auto a = lower_triangle<kStride, 2>({ { { { 2, 0 } }, { { 1, 3 } } } });
    REQUIRE(cholesky_factor(a.data(), 2, kStride));

    const std::array<double, 2> b { 3.0, 4.0 };
    std::array<float, 2> x {};
    REQUIRE(cholesky_solve(a.data(), b.data(), 2, kStride, x.data()));
    CHECK(x[0] == Approx(1.0f).margin(1e-5));
    CHECK(x[1] == Approx(1.0f).margin(1e-5));
}

TEST_CASE("cholesky works with stride == n and with x aliasing b", "[cholesky]") {
    constexpr size_t kStride = 3; // dense layout: stride equals n
    auto a = lower_triangle<kStride, 3>({ { { { 4, 0, 0 } }, { { 2, 5, 0 } }, { { 1, 3, 6 } } } });
    REQUIRE(cholesky_factor(a.data(), 3, kStride));

    SECTION("separate right-hand side") {
        const std::array<float, 3> b { 11, 21, 25 };
        std::array<float, 3> x {};
        REQUIRE(cholesky_solve(a.data(), b.data(), 3, kStride, x.data()));
        CHECK(x[0] == Approx(1.0f).margin(1e-5));
        CHECK(x[1] == Approx(2.0f).margin(1e-5));
        CHECK(x[2] == Approx(3.0f).margin(1e-5));
    }

    SECTION("x == b solves in place") {
        std::array<float, 3> bx { 11, 21, 25 };
        REQUIRE(cholesky_solve(a.data(), bx.data(), 3, kStride, bx.data()));
        CHECK(bx[0] == Approx(1.0f).margin(1e-5));
        CHECK(bx[1] == Approx(2.0f).margin(1e-5));
        CHECK(bx[2] == Approx(3.0f).margin(1e-5));
    }
}

TEST_CASE("cholesky rejects matrices that are not positive definite", "[cholesky]") {
    constexpr size_t kStride = 3;

    SECTION("negative pivot") {
        auto a = lower_triangle<kStride, 2>({ { { { -1, 0 } }, { { 0, 1 } } } });
        CHECK_FALSE(cholesky_factor(a.data(), 2, kStride));
    }

    SECTION("singular (zero pivot)") {
        auto a = lower_triangle<kStride, 2>({ { { { 0, 0 } }, { { 0, 1 } } } });
        CHECK_FALSE(cholesky_factor(a.data(), 2, kStride));
    }

    SECTION("indefinite: positive diagonal but det < 0") {
        // [[1,2],[2,1]] has eigenvalues 3 and -1.
        auto a = lower_triangle<kStride, 2>({ { { { 1, 0 } }, { { 2, 1 } } } });
        CHECK_FALSE(cholesky_factor(a.data(), 2, kStride));
    }
}

TEST_CASE("cholesky handles the degenerate 1x1 and n==0 cases", "[cholesky]") {
    constexpr size_t kStride = 2;
    auto a = lower_triangle<kStride, 1>({ { { { 9 } } } });
    REQUIRE(cholesky_factor(a.data(), 1, kStride));
    CHECK(a[0] == Approx(3.0f).margin(1e-6));

    const std::array<float, 1> b { 18 };
    std::array<float, 1> x {};
    REQUIRE(cholesky_solve(a.data(), b.data(), 1, kStride, x.data()));
    CHECK(x[0] == Approx(2.0f).margin(1e-5));

    // n == 0 is vacuously fine and must not read anything.
    CHECK(cholesky_factor(a.data(), 0, kStride));
    CHECK(cholesky_solve(a.data(), b.data(), 0, kStride, x.data()));
}
