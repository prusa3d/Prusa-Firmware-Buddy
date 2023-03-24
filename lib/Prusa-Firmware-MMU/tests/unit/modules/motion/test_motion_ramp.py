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
    data = data[data['pos'].diff() != 0]

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

    maxdev_fine = 20  # absolute maximum deviation
    maxdev_acc = 0.05  # 5% acceleration deviation
    maxdev_coarse = 0.1  # 10% speed deviation
    ramp_smp_skip = 3  # skip initial null values

    if fine_check:
        # exact rate
        data['rate'] = 1 / data['int_s']

    else:
        # reconstruct the rate from 3 samples
        data['rate'] = 1 / data.rolling(3)['int_s'].median()
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
        cruise_maxdev = 1 if fine_check else maxrate * maxdev_coarse
        assert ((cruise_data['rate'] - maxrate).abs().max() < cruise_maxdev)

    # checking acceleration segments require a decent number of samples for good results
    if acc_dist < 10:
        return

    # TODO: minrate is currently hardcoded in the FW as a function of the timer type (we
    # can't represent infinitely-long intervals, to the slowest speed itself is limited).
    # We recover the minrate here directly from the trace, but perhaps we shouldn't
    startrate = data['rate'].iat[ramp_smp_skip]
    endrate = data['rate'].iat[-1]

    # check acceleration segment (coarse)
    acc_data = data[(data['pos'] < acc_dist)][ramp_smp_skip:]
    acc_data['ts_s'] -= acc_data['ts_s'].iat[0]
    acc_time = acc_data['ts_s'].iat[-1]
    acc_data['exp_rate'] = startrate + acc_data['ts_s'] \
        / acc_time * (maxrate - startrate)
    assert ((acc_data['exp_rate'] - acc_data['rate']).abs().max() <
            maxrate * maxdev_coarse)

    # acceleration (fine)
    acc_data['exp_fine'] = acc_data['rate'].iat[0] + acc_data['ts_s'] \
        / acc_time * (acc_data['rate'].iat[-1] - startrate)
    if fine_check:
        assert ((acc_data['exp_fine'] - acc_data['rate']).abs().max() <
                maxdev_fine)

    # check effective acceleration rate
    acc_vel = (acc_data['rate'].iat[-1] - acc_data['rate'].iat[0]) / acc_time
    if fine_check:
        assert (abs(acc_vel - ax_info['accel']) / ax_info['accel'] <
                maxdev_acc)

    # deceleration (coarse)
    dec_data = data[(data['pos'] > (data['pos'].iat[-1] - acc_dist))][2:]
    dec_data['ts_s'] -= dec_data['ts_s'].iat[0]
    dec_time = dec_data['ts_s'].iat[-1]
    dec_data['exp_rate'] = maxrate - dec_data['ts_s'] \
        / dec_time * (maxrate - endrate)
    assert ((dec_data['exp_rate'] - dec_data['rate']).abs().max() <
            maxrate * maxdev_coarse)

    # deceleration (fine)
    dec_data['exp_fine'] = dec_data['rate'].iat[0] - dec_data['ts_s'] \
        / dec_time * (dec_data['rate'].iat[0] - endrate)
    if fine_check:
        assert ((dec_data['exp_fine'] - dec_data['rate']).abs().max() <
                maxdev_fine)

    # check effective deceleration rate
    dec_vel = (dec_data['rate'].iat[0] - dec_data['rate'].iat[-1]) / dec_time
    if fine_check:
        # TODO: deceleration rate is not as accurate as acceleration!
        assert (abs(dec_vel - ax_info['accel']) / ax_info['accel'] < 0.15)


def check_run(info, run):
    # unpack the axis data
    ax_info, data = run
    ax_count = len(ax_info)

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
