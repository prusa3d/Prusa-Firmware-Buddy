#include "common.hpp"

#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include "sfl/segmented_devector.hpp"
#include "sfl/segmented_vector.hpp"

#include <deque>
#include <vector>

template <typename Vector>
void erase_random(ankerl::nanobench::Bench& bench, int num_elements)
{
    const std::string title(name_of_type<Vector>());

    ankerl::nanobench::Rng rng;

    const int num_elements_to_erase = num_elements / 2;

    bench.batch(num_elements_to_erase).unit("erase").run
    (
        title,
        [&]
        {
            Vector vec;

            for (int i = 0; i < num_elements; ++i)
            {
                vec.emplace_back(int(rng()));
            }

            // Randomly erase half of elements
            for (int i = 0; i < num_elements_to_erase; ++i)
            {
                vec.erase(vec.begin() + rng.bounded(num_elements_to_erase));
            }

            ankerl::nanobench::doNotOptimizeAway(vec.empty());
        }
    );
}

int main()
{
    constexpr int num_elements = 100'000;

    ankerl::nanobench::Bench bench;
    bench.title("erase random (" + std::to_string(num_elements) + " elements)");
    bench.performanceCounters(false);
    bench.warmup(3);
    bench.epochs(10);

    erase_random<std::deque<int>>(bench, num_elements);
    erase_random<sfl::segmented_devector<int, 1024>>(bench, num_elements);
    erase_random<sfl::segmented_vector<int, 1024>>(bench, num_elements);
    erase_random<std::vector<int>>(bench, num_elements);
}
