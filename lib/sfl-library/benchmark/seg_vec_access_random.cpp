#include "common.hpp"

#define ANKERL_NANOBENCH_IMPLEMENT
#include "nanobench.h"

#include "sfl/segmented_devector.hpp"
#include "sfl/segmented_vector.hpp"

#include <deque>
#include <vector>

template <typename Vector>
void access_random_array(ankerl::nanobench::Bench& bench, int num_elements)
{
    const std::string title(name_of_type<Vector>());

    ankerl::nanobench::Rng rng;

    Vector vec;

    for (int i = 0; i < num_elements; ++i)
    {
        vec.emplace_back(int(rng()));
    }

    bench.batch(num_elements).unit("operator[]").run
    (
        title,
        [&]
        {
            int sum = 0;

            for (int i = 0; i < num_elements; ++i)
            {
                sum += vec[rng.bounded(num_elements)];
            }

            ankerl::nanobench::doNotOptimizeAway(sum);
        }
    );

}

template <typename Vector>
void access_random_at(ankerl::nanobench::Bench& bench, int num_elements)
{
    const std::string title(name_of_type<Vector>());

    ankerl::nanobench::Rng rng;

    Vector vec;

    for (int i = 0; i < num_elements; ++i)
    {
        vec.emplace_back(int(rng()));
    }

    bench.batch(num_elements).unit("at()").run
    (
        title,
        [&]
        {
            int sum = 0;

            for (int i = 0; i < num_elements; ++i)
            {
                sum += vec.at(rng.bounded(num_elements));
            }

            ankerl::nanobench::doNotOptimizeAway(sum);
        }
    );

}

template <typename Vector>
void access_random_nth(ankerl::nanobench::Bench& bench, int num_elements)
{
    const std::string title(name_of_type<Vector>());

    ankerl::nanobench::Rng rng;

    Vector vec;

    for (int i = 0; i < num_elements; ++i)
    {
        vec.emplace_back(int(rng()));
    }

    bench.batch(num_elements).unit("nth()").run
    (
        title,
        [&]
        {
            int sum = 0;

            for (int i = 0; i < num_elements; ++i)
            {
                sum += *vec.nth(rng.bounded(num_elements));
            }

            ankerl::nanobench::doNotOptimizeAway(sum);
        }
    );

}

int main()
{
    constexpr int num_elements = 10'000'000;

    ankerl::nanobench::Bench bench;
    bench.performanceCounters(false);
    bench.warmup(3);
    bench.epochs(10);

    bench.title("operator[] (" + std::to_string(num_elements) + " elements)");

    access_random_array<std::deque<int>>(bench, num_elements);
    access_random_array<sfl::segmented_devector<int, 1024>>(bench, num_elements);
    access_random_array<sfl::segmented_vector<int, 1024>>(bench, num_elements);
    access_random_array<std::vector<int>>(bench, num_elements);

    bench.title("at() (" + std::to_string(num_elements) + " elements)");

    access_random_at<std::deque<int>>(bench, num_elements);
    access_random_at<sfl::segmented_devector<int, 1024>>(bench, num_elements);
    access_random_at<sfl::segmented_vector<int, 1024>>(bench, num_elements);
    access_random_at<std::vector<int>>(bench, num_elements);

    bench.title("nth() (" + std::to_string(num_elements) + " elements)");

    access_random_nth<sfl::segmented_devector<int, 1024>>(bench, num_elements);
    access_random_nth<sfl::segmented_vector<int, 1024>>(bench, num_elements);

}
