#include <cstdio>
#ifndef __WIN32__
#include <sysexits.h>
#else
#define EX_OK 0
#define EX_USAGE 64
#define EX_OSERR 71
#endif

#include "motion.h"
using namespace modules::motion;

int main(int argc, const char *argv[]) {
    if (argc != 2 || !argv[1]) {
        fprintf(stderr, "Usage: %s <output>\n", argv[0]);
        return EX_USAGE;
    }

    // "parse" arguments
    const char *output = argv[1];
    FILE *fd = fopen(output, "w");
    if (!fd) {
        fprintf(stderr, "%s: can't open output file %s: ", argv[0], output);
        perror(NULL);
        return EX_OSERR;
    }

    // common settings
    const Axis ax_a = Idler;
    const int steps_a = 100;
    const Axis ax_b = Selector;
    const int steps_b = 80;
    const int maxFeedRate = 1000;
    const int maxJerk = 1;

    // write common parameters
    fprintf(fd, "{\"timebase\": %lu, \"quantum\": %u}\n",
        F_CPU / config::stepTimerFrequencyDivider, config::stepTimerQuantum);

    for (int ax_cnt = 0; ax_cnt != 2; ++ax_cnt) {
        for (int accel = 50; accel <= 50000; accel += accel / 2) {
            // first axis defines the nominal values
            motion.SetJerk(ax_a, maxJerk);
            motion.SetPosition(ax_a, 0);
            motion.SetAcceleration(ax_a, accel);
            motion.PlanMoveTo(ax_a, steps_a, maxFeedRate);

            fprintf(fd, "[{\"steps\": %d, \"jerk\": %d, \"accel\": %d, \"maxrate\": %d}",
                steps_a, maxJerk, accel, maxFeedRate);

            if (ax_cnt > 0) {
                // second axis finishes slightly sooner at triple acceleration to maximize the
                // aliasing effects
                int accel_3 = accel * 3;
                motion.SetJerk(ax_b, 1);
                motion.SetPosition(ax_b, 0);
                motion.SetAcceleration(ax_b, accel_3);
                motion.PlanMoveTo(ax_b, steps_b, maxFeedRate);

                fprintf(fd, ", {\"steps\": %d, \"jerk\": %d, \"accel\": %d, \"maxrate\": %d}",
                    steps_b, maxJerk, accel_3, maxFeedRate);
            }

            fprintf(fd, "]\n");

            // initial state
            unsigned long ts = 0;
            st_timer_t next = 0;
            fprintf(fd, "%lu %u %d %d\n", ts, next, motion.CurPosition(ax_a), motion.CurPosition(ax_b));

            // step and output time, interval and positions
            do {
                next = motion.Step();
                pos_t pos_idler = motion.CurPosition(ax_a);
                pos_t pos_selector = motion.CurPosition(ax_b);

                fprintf(fd, "%lu %u %d %d\n", ts, next, pos_idler, pos_selector);

                ts += next;
            } while (next);
            fprintf(fd, "\n");
        }
    }

    return EX_OK;
}
