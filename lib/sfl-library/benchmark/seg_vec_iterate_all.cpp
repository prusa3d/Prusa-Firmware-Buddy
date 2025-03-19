#include "common.hpp"

#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include "sfl/segmented_devector.hpp"
#include "sfl/segmented_vector.hpp"

#include <deque>
#include <vector>

template <typename Vector>
void iterate(ankerl::nanobench::Bench& bench, int num_elements)
{
    const std::string title(name_of_type<Vector>());

    ankerl::nanobench::Rng rng;

    Vector vec;

    for (int i = 0; i < num_elements; ++i)
    {
        vec.emplace_back(int(rng()));
    }

    bench.warmup(10).batch(num_elements).unit("iteration").run
    (
        title,
        [&]
        {
            int sum = 0;

            for (auto it = vec.begin(), end = vec.end(); it != end; ++it)
            {
                sum += *it;
            }

            ankerl::nanobench::doNotOptimizeAway(sum);
        }
    );
}

int main()
{
    constexpr int num_elements = 100'000'000;

    ankerl::nanobench::Bench bench;
    bench.title("iterate all (" + std::to_string(num_elements) + " elements)");
    bench.performanceCounters(false);
    bench.warmup(3);
    bench.epochs(10);

    iterate<std::deque<int>>(bench, num_elements);
    iterate<sfl::segmented_devector<int, 1024>>(bench, num_elements);
    iterate<sfl::segmented_vector<int, 1024>>(bench, num_elements);
    iterate<std::vector<int>>(bench, num_elements);
}
