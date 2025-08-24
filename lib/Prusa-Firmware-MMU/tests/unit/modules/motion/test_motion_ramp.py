#!/usr/bin/env python3
import numpy as np
import argparse
import json

import pandas as pd
pd.options.mode.chained_assignment = None


def load_data(data):
    runs = []

    # read all sets
    lines = open(data).readlines()
    info = json.loads(lines[0])

    i = 1
    while i < len(lines):
        # first line in each set is a json description
        run_info = json.loads(lines[i])

        run_data = []
        for j in range(i + 1, len(lines)):
            # read until an empty line (data terminator)
            line = lines[j].rstrip()
            if len(line) == 0:
                break

            # parse the line
            tokens = list(map(int, line.split(' ')))
            run_data.append(tokens)

        runs.append([run_info, run_data])
        i = j + 1

    return info, runs


def check_axis(info, ax_info, data, fine_check):
    tb = info['timebase']

    # remove duplicate positions (meaning another axis was moved, not the current)
    data = data[data['pos'].diff() != 0].reset_index()

    # recalculate intervals just for this axis
    data['int'] = data['ts'].diff()

    # check start/ending position
    assert (data['pos'].iat[0] == 0)
    assert (data['pos'].iat[-1] == ax_info['steps'])

    # check first null timestamp/interval/position values
    assert ((data['ts'][0:2] == 0).all())
    assert ((data['int'][0:2].dropna() == 0).all())

    # ensure timestamps and positions are monotonically increasing
    assert ((data['ts'].diff()[2:] > 0).all())
    assert ((data['pos'].diff()[1:] > 0).all())

    # convert timestamps to seconds
    data['ts_s'] = data['ts'] / tb
    data['int_s'] = data['int'] / tb

    if fine_check:
        maxdev = 0.1  # 10% rate deviation tolerance
    else:
        maxdev = 0.2  # higher tolerance due to quantization

    # initial samples to skip
    ramp_smp_skip = 3

    if fine_check:
        # exact rate
        data['rate'] = 1 / data['int_s']

    else:
        # reconstruct the rate from 5 samples
        data['rate'] = 1 / data.rolling(5)['int_s'].median()
        data['rate'].bfill(inplace=True)

    # ensure we never _exceed_ max feedrate
    assert ((data['rate'][2:] <= ax_info['maxrate']).all())

    # recalculate independently the acceleration parameters
    acc_dist = (ax_info['maxrate']**2 -
                ax_info['jerk']**2) / (2 * ax_info['accel'])
    if acc_dist * 2 > ax_info['steps']:
        # no cruising, calculate intersection (equal start/end speed)
        acc_dist = ax_info['steps'] / 2
        maxrate = np.sqrt(2 * ax_info['accel'] * acc_dist + ax_info['jerk']**2)
    else:
        # cruising possible, get distance
        c_dist = ax_info['steps'] - 2 * acc_dist
        maxrate = ax_info['maxrate']

        # check cruising speed
        cruise_data = data[(data['pos'] > acc_dist + 2)
                           & (data['pos'] < acc_dist + 2 + c_dist)]
        assert ((cruise_data['rate'] - maxrate).abs().max() < 1)

    # checking acceleration segments require a decent number of samples for good results
    if acc_dist < 20:
        return

    # TODO: minrate is currently hardcoded in the FW as a function of the timer type (we
    # can't represent infinitely-long intervals, to the slowest speed itself is limited).
    # We recover the minrate here directly from the trace, but perhaps we shouldn't
    startrate = data['rate'].iat[ramp_smp_skip]
    endrate = data['rate'].iat[-1]

    # check acceleration segment
    acc_data = data[(data['pos'] < acc_dist)][ramp_smp_skip:]
    acc_data['ts_s'] -= acc_data['ts_s'].iat[0]
    acc_time = acc_data['ts_s'].iat[-1]
    acc_data['exp_rate'] = startrate + acc_data['ts_s'] * ax_info['accel']
    assert ((acc_data['exp_rate'] - acc_data['rate']).abs().max() <
            maxrate * maxdev)

    # check acceleration rate
    acc_acc = (acc_data['rate'].iat[-1] - acc_data['rate'].iat[0]) / acc_time
    assert (abs(acc_acc - ax_info['accel']) / ax_info['accel'] < maxdev)

    # deceleration segment
    dec_data = data[(data['pos'] >
                     (data['pos'].iat[-1] - acc_dist))][ramp_smp_skip:]
    dec_data['ts_s'] -= dec_data['ts_s'].iat[0]
    dec_time = dec_data['ts_s'].iat[-1]
    dec_data['exp_rate'] = dec_data['rate'].iat[
        0] - dec_data['ts_s'] * ax_info['accel']

    # check deceleration rate
    dec_acc = (dec_data['rate'].iat[0] - dec_data['rate'].iat[-1]) / dec_time
    assert (abs(dec_acc - ax_info['accel']) / ax_info['accel'] < maxdev)


def check_run(info, run):
    # unpack the axis data
    ax_info, data = run
    ax_count = len(ax_info)

    # first and last interval should always be zero
    assert (data[0][1] == 0)
    assert (data[-1][1] == 0)

    # ensure no interval is shorter than the specified quantum
    for i in range(1, len(data) - 2):
        interval = data[i][1]
        assert (interval >= info['quantum'])

    # split axis information
    ax_data = []
    for ax in range(ax_count):
        ax_info[ax]['name'] = ax
        tmp = []
        for i in range(len(data)):
            row = data[i]
            tmp.append([row[0], row[1], row[2 + ax]])

        ax_data.append(pd.DataFrame(tmp, columns=['ts', 'int', 'pos']))

    # check each axis independently, but only perform fine-grained checks when a single
    # axis is run due to stepperTimerQuantum introducing discretization noise
    fine_check = ax_count == 1
    for ax in range(ax_count):
        check_axis(info, ax_info[ax], ax_data[ax], fine_check)


def main():
    # parse arguments
    ap = argparse.ArgumentParser()
    ap.add_argument('data')
    args = ap.parse_args()

    # load data runs
    info, runs = load_data(args.data)

    # test each set
    for run in runs:
        check_run(info, run)


if __name__ == '__main__':
    exit(main())
