from __future__ import annotations
import datetime
import time
import itertools
from pathlib import Path
import click

from copy import copy
import json
from contextlib import contextmanager
import sys
from typing import Any, Callable, ClassVar, Dict, Generator, Iterable, List, Optional, Tuple
from dataclasses import dataclass, field

import serial.tools.list_ports
from serial import Serial  # type: ignore

from scipy.fft import rfft, rfftfreq
import scipy.signal
import numpy as np
import plotly.graph_objects as go
import plotly.colors
from plotly.express.colors import sample_colorscale

PRUSA_VID = 0x2c99
MOTOR_PERIOD = 1024
ACCEL_MAX_SAMPLING_RATE = 1300


class Machine:

    def __init__(self, port: Serial) -> None:
        self._port = port
        self.accFreq = None

    @contextmanager
    def _preserveTimeout(self) -> Generator[None, None, None]:
        originalTimeout = self._port.timeout
        try:
            yield
        finally:
            self._port.timeout = originalTimeout

    def waitForBoot(self) -> None:
        """
        Wait for the board to boot up - that is no new info is echoed
        """
        self.command("G")

    def command(self, command: str, timeout: float = 10) -> List[str]:
        """
        Issue G-code command, waits for completion and returns a list of
        returned values (lines of response)
        """
        if not command.endswith("\n"):
            command += "\n"
        with self._preserveTimeout():
            # Clear pending data
            self._port.timeout = None
            self._port.read_all()
            # Send command
            self._port.write(command.encode("utf-8"))
            # Wait for response
            response = []
            if timeout != 0:
                self._port.timeout = timeout
            else:
                self._port.timeout = 3
            while True:
                line = self._port.readline().decode("utf-8").strip()
                if line == "":
                    if timeout != 0:
                        raise TimeoutError(
                            f"No response on command {command.strip()}")
                    else:
                        return response
                line = line.strip()
                if line.endswith("ok"):
                    if line[:-2] != "":
                        response.append(line[:-2])
                    return response
                response.append(line)

    def multiCommand(self,
                     commands: List[str],
                     timeout: float = 10) -> List[str]:
        """
        Issue multiple G-code commands at once, return line summary.
        """
        command = "\n".join(commands) + "\n"
        okCount = 0
        with self._preserveTimeout():
            # Clear pending data
            self._port.timeout = None
            self._port.read_all()
            # Send command
            self._port.write(command.encode("utf-8"))
            # Wait for response
            response = []
            if timeout != 0:
                self._port.timeout = timeout
            else:
                self._port.timeout = 3
            while True:
                line = self._port.readline().decode("utf-8").strip()
                if line == "":
                    if timeout != 0:
                        raise TimeoutError(
                            f"No response on command {command.strip()}")
                    else:
                        return response
                line = line.strip()
                if line.endswith("ok"):
                    if line[:-2] != "":
                        response.append(line[:-2])
                    okCount += 1
                    if okCount == len(commands):
                        return response
                response.append(line)


def getPrusaPort() -> Optional[str]:
    """
    Return first port that belongs to a Prusa machine
    """
    for port in serial.tools.list_ports.comports():
        if port.vid == PRUSA_VID:
            return port.device
    return None


@contextmanager
def machineConnection(port: str = getPrusaPort()) -> Generator[Machine, None,
                                                               None]:
    with Serial(port) as s:
        yield Machine(s)


@contextmanager
def enabledMachineConnection(port: str = getPrusaPort()) -> Generator[
        Machine, None, None]:
    with Serial(port) as s:
        m = Machine(s)
        m.waitForBoot()
        m.command("M17")
        m.command("M970 X1 Y1")
        yield m
        m.command("M18")


@dataclass
class PhaseCorrection:
    spectrum: List[Tuple[float, float]] = field(
        default_factory=lambda: [(0, 0) for _ in range(17)])

    def phaseShift(self) -> List[float]:
        phaseSeq = []
        for i in range(MOTOR_PERIOD):
            pha = i * 2 * np.pi / MOTOR_PERIOD
            res = 0
            for n, (mag, phase) in enumerate(self.spectrum):
                res += mag * np.sin(n * pha + phase)
            phaseSeq.append(res)
        return phaseSeq

    def currents(self) -> List[Tuple(int, int)]:
        currents = []
        for i, phasehift in enumerate(self.phaseShift()):
            pha = i * 2 * np.pi / 1024 + phasehift
            a = np.round(248 * np.sin(pha))
            b = np.round(248 * np.cos(pha))
            currents.append((a, b))
        return currents


def readLut(machine: Machine, axis: str, direction: str) -> PhaseCorrection:
    prefix = f"M971 {axis} {direction}"
    rawResponse = machine.command(prefix)

    correction = PhaseCorrection()
    for line in rawResponse:
        if not line.startswith(prefix):
            continue
        line = line.removeprefix(prefix).strip()
        idx, mag, pha = line.split(" ")
        idx = int(idx.removeprefix('I'))
        mag = float(mag.removeprefix('M'))
        pha = float(pha.removeprefix('P'))

        correction.spectrum[idx] = (mag, pha)
    return correction


def writeLut(machine: Machine, axis: str, direction: str,
             correction: PhaseCorrection) -> None:
    machine.command(f"M970 {axis}0")
    idx = 0
    for mag, pha in correction.spectrum[:17]:
        machine.command(f"M971 {axis}{direction} I{idx} M{mag:.7g} P{pha:.7g}")
        idx += 1
    machine.command(f"M970 {axis}1")


@click.command("readLut")
@click.option("--axis",
              required=True,
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--dir",
              required=True,
              type=click.Choice(["F", "B"]),
              help="Table for which direction")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
def readLutCmd(axis: str, dir: str, port: str) -> None:
    with machineConnection(port) as machine:
        machine.waitForBoot()
        currents = readLut(machine, axis, dir)
        json.dump(currents.spectrum, sys.stdout, indent=4)


@click.command("writeLut")
@click.argument("filepath",
                required=True,
                type=click.Path(file_okay=True, exists=True, dir_okay=False))
@click.option("--axis",
              required=True,
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--dir",
              required=True,
              type=click.Choice(["F", "B"]),
              help="Table for which direction")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
def writeLutCmd(axis: str, dir: str, port: str, filepath: str) -> None:
    with open(filepath) as f:
        correction = json.load(f)
    with machineConnection(port) as machine:
        machine.waitForBoot()
        writeLut(machine, axis, dir, PhaseCorrection(correction))


def store_or_show_plot(show, output, name, plot):
    if output is not None:
        Path(output).mkdir(parents=True, exist_ok=True)
        plot.write_html(output / f"{name}.html")
    if show:
        plot.show()


@dataclass
class SweepMeasurement:
    samples: List[float]
    sampling_freq: float
    movement_ok: bool
    accel_error: int

    start_marker: float
    end_marker: float
    signal_start: float
    signal_end: float

    @staticmethod
    def from_raw_command(response: List[str]):
        samples = []
        sampling_freq = 0.0
        movement_ok = False
        accel_error = 0
        start_marker = 0.0
        end_marker = 0.0
        signal_start = 0.0
        signal_end = 0.0

        for line in response:
            if line.startswith("sampling_freq:"):
                sampling_freq = float(line.split(":")[1])
            elif line.startswith("movement_ok:"):
                movement_ok = line.split(":")[1].strip().lower() == "true"
            elif line.startswith("accel_error:"):
                accel_error = int(line.split(":")[1])
            elif line.startswith("start_marker:"):
                start_marker = float(line.split(":")[1])
            elif line.startswith("end_marker:"):
                end_marker = float(line.split(":")[1])
            elif line.startswith("signal_start:"):
                signal_start = float(line.split(":")[1])
            elif line.startswith("signal_end:"):
                signal_end = float(line.split(":")[1])
            elif line[0].isdigit():
                samples.append(float(line.split(",")[1]))

        return SweepMeasurement(
            samples=samples,
            sampling_freq=sampling_freq,
            movement_ok=movement_ok,
            accel_error=accel_error,
            start_marker=start_marker,
            end_marker=end_marker,
            signal_start=signal_start,
            signal_end=signal_end,
        )

    def is_ok(self):
        return self.movement_ok and self.accel_error == 0 and self.sampling_freq > 0

    def get_signal(self):
        start_marker_idx, end_marker_idx = locate_markers(self)
        start_marker_time = start_marker_idx / self.sampling_freq
        end_marker_time = end_marker_idx / self.sampling_freq

        signal_start_idx = int(
            (self.signal_start - self.start_marker) * self.sampling_freq)
        signal_end_idx = int(
            (self.signal_end - self.start_marker) * self.sampling_freq)

        return self.samples[start_marker_idx +
                            signal_start_idx:start_marker_idx + signal_end_idx]


def compute_energy(samples: List[float], start_idx: int, end_idx: int,
                   win: int) -> list[float]:
    # This isn't pythonic, but it mimics the firmware implementation
    energy = [sum(x * x for x in samples[start_idx:start_idx + win])]
    for i in range(start_idx + 1, end_idx):
        energy.append(energy[-1] + samples[i] * samples[i] -
                      samples[i - win] * samples[i - win])
    return energy


def locate_markers(measurement: SweepMeasurement,
                   search_win=0.1) -> Tuple[int, int]:
    ENERGY_WIN = 0.005  # s
    energy_win = int(ENERGY_WIN * measurement.sampling_freq)

    markers_offset = int((measurement.end_marker - measurement.start_marker) *
                         measurement.sampling_freq)
    start1_idx = int((measurement.start_marker - search_win / 2) *
                     measurement.sampling_freq)
    end1_idx = int((measurement.start_marker + search_win / 2) *
                   measurement.sampling_freq)

    energy1 = compute_energy(measurement.samples, start1_idx, end1_idx,
                             energy_win)
    energy2 = compute_energy(measurement.samples, start1_idx + markers_offset,
                             end1_idx + markers_offset, energy_win)
    combined_energy = [e1 + e2 for e1, e2 in zip(energy1, energy2)]

    mean = np.mean(combined_energy)
    first_peak = np.argmax(combined_energy > mean)

    if False:  # Override to quickly visualize the results as a debugging mean
        fig = go.Figure()
        fig.update_layout(
            yaxis=dict(title="Energy"),
            yaxis2=dict(title="Signal", overlaying="y", side="right"),
        )

        fig.add_trace(go.Scatter(y=combined_energy, name="Combined energy"))
        fig.add_trace(go.Scatter(y=energy1, name="Energy 1"))
        fig.add_trace(go.Scatter(y=energy2, name="Energy 2"))

        fig.add_trace(
            go.Scatter(y=np.diff(combined_energy),
                       name="Diff combined energy"))

        fig.add_trace(
            go.Scatter(y=measurement.samples[start1_idx:end1_idx],
                       name="Signal 1",
                       yaxis="y2"))
        fig.add_trace(
            go.Scatter(
                y=measurement.samples[start1_idx + markers_offset:end1_idx +
                                      markers_offset],
                name="Signal 2",
                yaxis="y2"))

        fig.add_vline(x=first_peak,
                      line_dash="dash",
                      line_color="red",
                      annotation_text="Max peak")
        fig.add_hline(y=mean,
                      line_dash="dash",
                      line_color="green",
                      annotation_text="Mean")
        fig.show()

    return start1_idx + first_peak, start1_idx + first_peak + markers_offset


def locate_signal(measurement: SweepMeasurement,
                  search_win=0.1) -> Tuple[int, int]:
    start_marker_idx, end_marker_idx = locate_markers(measurement)
    signal_start = int((measurement.signal_start - measurement.start_marker) *
                       measurement.sampling_freq)
    signal_end = int((measurement.signal_end - measurement.start_marker) *
                     measurement.sampling_freq)
    return start_marker_idx + signal_start, start_marker_idx + signal_end


def compute_spectrogram(samples,
                        start_idx,
                        end_idx,
                        sampling_freq,
                        window_size_s,
                        step_size=1):
    """
    Compute a spectrogram of the signal. Returns time_bins, freq_bins,
    spectrogram.
    """
    win_size = int(window_size_s * sampling_freq)
    spectrogram = []
    for i in range(start_idx, end_idx, step_size):
        windowed_signal = samples[i - win_size // 2:i + win_size // 2]
        windowed_signal *= np.hanning(len(windowed_signal))
        fft = rfft(windowed_signal)
        spectrogram.append(np.abs(fft))
    spectrogram.pop()

    time_bins = [
        i / sampling_freq for i in range(start_idx, end_idx, step_size)
    ]
    freq_bins = rfftfreq(win_size, 1 / sampling_freq)
    return time_bins, freq_bins, np.array(spectrogram)


def dft_sweep_motor_harmonic(samples: np.ndarray, sampling_freq: float,
                             idx_start: int, idx_end: int, speed: float,
                             motor_steps: int, harmonic: int, window_size: int,
                             step_size: int):
    """
    Compute a single bin DFT sweep at an integral multiple of motor frequency.
    The window size and step size are given in motor periods.np.round(
    """
    motor_period_duration = 1 / (speed * motor_steps / 4)
    analysis_freq = speed * motor_steps / 4 * harmonic

    signal_duration = (idx_end - idx_start) / sampling_freq
    window_duration = window_size * motor_period_duration
    half_window_idx = int(window_duration * sampling_freq / 2)

    total_steps = int(signal_duration / motor_period_duration / step_size)

    sin_corr = []
    cos_corr = []
    for i, s in enumerate(samples):
        t = i / sampling_freq
        sin_corr.append(np.sin(2 * np.pi * analysis_freq * t) * s)
        cos_corr.append(np.cos(2 * np.pi * analysis_freq * t) * s)

    bins = []
    res = []
    for i in range(0, total_steps):
        center_time = i * motor_period_duration * step_size
        center_idx = idx_start + int(center_time * sampling_freq)

        sin_win = sin_corr[center_idx - half_window_idx:center_idx +
                           half_window_idx]
        cos_win = cos_corr[center_idx - half_window_idx:center_idx +
                           half_window_idx]

        sin_corr_sum = np.sum(sin_win)
        cos_corr_sum = np.sum(cos_win)
        bins.append(center_time)
        res.append(
            np.sqrt(sin_corr_sum * sin_corr_sum + cos_corr_sum * cos_corr_sum)
            / len(sin_win))

    return bins, np.asarray(res)


def find_n_valleys(y, n, hysteresis=0.1, max_iter=10, stepdown=0.9):
    """
    Identifies the n most prominent valleys by iteratively adjusting threshold
    until n valleys are found.
    """

    y = np.asarray(y)
    min_y, max_y = np.min(y), np.max(y)
    best_threshold = np.mean(y)
    regions = []

    for i in range(max_iter):
        lower_threshold = best_threshold
        upper_threshold = best_threshold + hysteresis * (max_y - min_y)

        num_regions = 0
        in_region = False
        regions = []
        start = None

        for i in range(len(y)):
            if y[i] <= lower_threshold and not in_region:
                num_regions += 1
                in_region = True
                start = i
            elif y[i] > upper_threshold and in_region:
                in_region = False
                regions.append((start, i))
        if num_regions == n:
            break

        best_threshold = min_y + (best_threshold - min_y) * stepdown

    valleys = []
    for start, end in regions:
        assert start < end
        valleys.append(start + np.argmin(y[start:end]))

    return valleys


def find_peaks(signal: Iterable[float],
               min_prominence: float = 0.2) -> List[Tuple[int, float, float]]:
    """
    Find all peaks in a signal. Signal is a 1D iterable containing the signal.

    Returns:
        List of tuples (peak_index, peak_value, prominence), sorted by
        prominence.
    """
    signal = np.asarray(signal)
    assert len(signal) > 0, "Signal must not be empty."

    n = len(signal)
    peaks = []

    peak_indices = [
        i for i in range(1, n - 1)
        if signal[i] > signal[i - 1] and signal[i] > signal[i + 1]
    ]

    # If no local maxima found, return global max
    if not peak_indices:
        max_idx = np.argmax(signal)
        return [(max_idx, signal[max_idx], 1)]

    # Compute left and right minimums
    left_min = np.zeros(n)
    right_min = np.zeros(n)

    left_min[0] = signal[0]
    for i in range(1, n):
        left_min[i] = min(left_min[i - 1], signal[i])

    right_min[-1] = signal[-1]
    for i in range(n - 2, -1, -1):
        right_min[i] = min(right_min[i + 1], signal[i])

    # Step 3: Compute prominence for each peak
    signal_max = max(signal)
    for i in peak_indices:
        surrounding_min = min(left_min[i - 1], right_min[i + 1])
        prominence = (signal[i] - surrounding_min) / signal_max
        if prominence >= min_prominence:
            peaks.append((i, signal[i], prominence))

    return sorted(peaks, key=lambda x: -x[2])


def harmonic_peaks_fit(harmonic_positions):
    """
    Computes a mean fit for hamonic positions. The positions are given as tuples
    (harmonic, position).

    Return a tuple (estimated_positions, sum_squared_error)
    """
    C_values = [h * p for h, p in harmonic_positions]
    C_est = np.mean(C_values)
    estimated_positions = [(h, C_est / h) for h, _ in harmonic_positions]
    sum_squared_error = sum(
        (p - p_est)**2
        for (_, p), (_, p_est) in zip(harmonic_positions, estimated_positions))

    return estimated_positions, sum_squared_error


def detect_harmonic_peaks(signals: List[Tuple[int, Iterable[float]]],
                          pos_function: Callable[[int], float],
                          min_prominence: float = 0.2):
    """
    Detect peaks in multiple signals that follow harmonic relationships. The
    signals are specified as a list of tuples (harmonic, signal). The position
    function is used to convert indices to positions. Return the best detected
    peaks and the best estimated positions.
    """

    peaks = [(h, [(pos_function(i), s, p)
                  for i, s, p in find_peaks(signal, min_prominence)])
             for h, signal in signals]

    # Try all peaks as anchors for harmonic detection
    best_badness = np.inf
    best_positions = None
    best_estimation = None
    for h_anchor, h_peaks in peaks:
        for anchor_pos, _, _ in h_peaks:
            nominal_pos = anchor_pos * h_anchor

            # Selected the closes peak for each harmonic
            selected_peaks = {}
            for h, h_peaks in peaks:
                best_dist = np.inf
                best_peak = None
                for pos, _, _ in h_peaks:
                    dist = abs(pos * h - nominal_pos)
                    if dist < best_dist:
                        best_dist = dist
                        best_peak = pos
                selected_peaks[h] = best_peak

            # Compute badness
            estimated_pos, badness = harmonic_peaks_fit(
                list(selected_peaks.items()))
            if badness < best_badness:
                best_badness = badness
                best_positions = selected_peaks
                best_estimation = estimated_pos

    best_positions = [(h, p) for h, p in best_positions.items()]
    return best_positions, best_estimation


def moving_average(signal, window_size):
    """
    Compute a moving average with boundary correction.
    """
    n = len(signal)
    smoothed = np.zeros(n)

    for i in range(n):
        left = max(0, i - window_size // 2)
        right = min(n, i + window_size // 2 + 1)
        smoothed[i] = np.mean(signal[left:right])

    return smoothed


def analyze_speed_sweep(samples, sampling_freq, start_idx, end_idx,
                        start_speed, top_speed, motor_steps, harmonic,
                        window_size):
    """
    Analyze a speed sweep measurement that goes from start_speed to top_speed
    and then back again to start_speed. The window_size is given in seconds
    """
    sweep_duration = (end_idx - start_idx) / sampling_freq
    ramp_duration = sweep_duration / 2
    one_period_revs = 1 / (motor_steps / 4)

    start_freq = harmonic * start_speed / one_period_revs
    top_freq = harmonic * top_speed / one_period_revs
    speed_accel = (top_speed - start_speed) / ramp_duration
    freq_accel = (top_freq - start_freq) / ramp_duration
    freq_accel_start_t = start_idx / sampling_freq
    freq_accel_top_t = (end_idx + start_idx) / 2 / sampling_freq

    sin_cor = []
    cos_cor = []
    for i, s in enumerate(samples):
        # The argument of sin and cos is in radians and consists of three parts:
        # - the constant velocity part
        # - the acceleration part
        # - the deceleration part
        t = i / sampling_freq
        if t < freq_accel_start_t:
            freq = start_freq
            arg = 2 * np.pi * start_freq * t
        elif t < freq_accel_top_t:
            freq = start_freq + freq_accel * (t - freq_accel_start_t)
            arg = 2 * np.pi * start_freq * t + np.pi * freq_accel * (
                t - freq_accel_start_t)**2
        else:
            freq = top_freq - freq_accel * (t - freq_accel_top_t)
            t_rel = t - freq_accel_top_t
            arg = 2 * np.pi * top_freq * t_rel - np.pi * freq_accel * t_rel**2
        if freq < sampling_freq / 2:
            sin_cor.append(np.sin(arg) * s)
            cos_cor.append(np.cos(arg) * s)
        else:
            sin_cor.append(0)
            cos_cor.append(0)

    retval = []
    for r in [(start_idx, (start_idx + end_idx) // 2),
              ((start_idx + end_idx) // 2, end_idx)]:
        res = []
        speed_bins = []
        for i in range(*r):
            win_half = int(window_size * sampling_freq / 2)
            sin_w = np.sum(sin_cor[i - win_half:i +
                                   win_half])  # * np.hanning(win_half * 2))
            cos_w = np.sum(cos_cor[i - win_half:i +
                                   win_half])  # * np.hanning(win_half * 2))

            t = i / sampling_freq
            if t < freq_accel_start_t:
                speed = start_speed
            elif t < freq_accel_top_t:
                speed = start_speed + speed_accel * (t - freq_accel_start_t)
            else:
                speed = top_speed - speed_accel * (t - freq_accel_top_t)

            res.append((sin_w * sin_w + cos_w * cos_w) / speed)
            speed_bins.append(speed)

        if r[1] == end_idx:
            speed_bins = speed_bins[::-1]
            res = res[::-1]
        retval.append((np.asarray(speed_bins), res))

    return tuple(retval)


def plot_raw_measurement(measurement: SweepMeasurement, name=None):
    fig = go.Figure()

    if name is not None:
        fig.update_layout(title=name)

    times = [
        i / measurement.sampling_freq for i in range(len(measurement.samples))
    ]
    fig.add_trace(go.Scatter(x=times, y=measurement.samples))

    start_maker_idx, end_marker_idx = locate_markers(measurement)
    start_marker_time = start_maker_idx / measurement.sampling_freq
    end_marker_time = end_marker_idx / measurement.sampling_freq

    signal_start_time = start_marker_time + measurement.signal_start - measurement.start_marker
    signal_end_time = start_marker_time + measurement.signal_end - measurement.start_marker

    fig.add_vline(x=start_marker_time,
                  line_dash="dash",
                  line_color="red",
                  annotation_text="Start marker")
    fig.add_vline(x=end_marker_time,
                  line_dash="dash",
                  line_color="red",
                  annotation_text="End marker")
    fig.add_vline(x=signal_start_time,
                  line_dash="dash",
                  line_color="green",
                  annotation_text="Signal start")
    fig.add_vline(x=signal_end_time,
                  line_dash="dash",
                  line_color="green",
                  annotation_text="Signal end")

    return fig


def plot_spectrogram(spectrogram, time_bins, freq_bins, name=None):
    fig = go.Figure(data=go.Heatmap(
        z=spectrogram.T, x=time_bins, y=freq_bins, colorscale='Jet'))
    fig.update_layout(title=name)
    return fig


def plot_accelerated_spectrogram(spectrogram,
                                 time_bins,
                                 freq_bins,
                                 fundamental_start,
                                 fundamental_end,
                                 name=None):
    spectrogram = np.asarray(spectrogram)
    fig = go.Figure(data=go.Heatmap(
        z=spectrogram.T, x=time_bins, y=freq_bins, colorscale='Jet'))
    fig.update_layout(title=name)

    # The speed is linearly increasing, so the fundamental frequency is also
    # linearly increasing. Draw the fundamental line + harmonics
    for i in range(1, 9):
        max_y = freq_bins[-1]

        origin_y = fundamental_start * i
        origin_x = time_bins[0]

        end_y = fundamental_end * i
        end_x = time_bins[len(time_bins) // 2]
        if end_y > max_y:
            end_y = max_y
            end_x = end_x - (fundamental_end - max_y / i) / (
                fundamental_end - fundamental_start) * (end_x - time_bins[0])

        fig.add_trace(
            go.Scatter(x=[origin_x, end_x],
                       y=[origin_y, end_y],
                       mode="lines",
                       line_shape="linear",
                       line=dict(color="black", width=1)))
    return fig


@click.command("analyzeParamSweep")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--motor-steps", type=int, default=200)
@click.option("--axis", type=click.Choice(["X", "Y"]))
@click.option("--speed", type=float, default=1)
@click.option("--revs", type=float, default=2)
@click.option("--n", type=int, default=2)
@click.option("--mag-start", type=float, default=0)
@click.option("--mag-end", type=float, default=0.0)
@click.option("--pha-start", type=float, default=0)
@click.option("--pha-end", type=float, default=0)
@click.option("--output",
              type=click.Path(dir_okay=True, file_okay=False),
              default=None,
              help="Output directory for data")
@click.option("--show", is_flag=True)
@click.option("--as-numpy/--as-machine",
              default=False,
              help="Use numpy or the direct machine implementation?")
def analyzeParamSweep(port, motor_steps, axis, speed, revs, n, mag_start,
                      mag_end, pha_start, pha_end, output, show, as_numpy):
    """
    Analyze parameter sweep
    """

    if output is None and not show:
        print("No output specified, nothing to do. Specify --output or --show")
        return

    present_plot = lambda name, plot: store_or_show_plot(
        show, output, name, plot)

    with enabledMachineConnection(port=port) as machine:
        time.sleep(0.2)
        machine.command("G92 X0 Y0")
        while True:
            raw_output = machine.command(
                f"M973 {axis} R{revs:.10f} F{speed:.10f} H{n} A{pha_start:.10f} B{pha_end:.10f} C{mag_start:.10f} D{mag_end:.10f}"
            )
            sweep_measurement_f = SweepMeasurement.from_raw_command(raw_output)
            machine.command("G0 F10000 X0 Y0")
            if sweep_measurement_f.is_ok():
                break
            print("Retrying")

        pha_start_b, pha_end_b = pha_end, pha_start
        mag_start_b, mag_end_b = mag_end, mag_start

        while True:
            raw_output = machine.command(
                f"M973 {axis} R{revs:.10f} F{speed:.10f} H{n} A{pha_start_b:.10f} B{pha_end_b:.10f} C{mag_start_b:.10f} D{mag_end_b:.10f}"
            )
            sweep_measurement_b = SweepMeasurement.from_raw_command(raw_output)
            machine.command("G0 F10000 X0 Y0")
            if sweep_measurement_b.is_ok():
                break
            print("Retrying")

    present_plot(
        "raw_f",
        plot_raw_measurement(sweep_measurement_f,
                             name="Forward sweep raw data"))
    present_plot(
        "raw_b",
        plot_raw_measurement(sweep_measurement_b,
                             name="Backward sweep raw data"))

    signal_start_f, signal_end_f = locate_signal(sweep_measurement_f)
    signal_start_b, signal_end_b = locate_signal(sweep_measurement_b)

    # Spectrogram
    time_bins_f, freq_bins_f, spectrogram_f = compute_spectrogram(
        sweep_measurement_f.samples, signal_start_f, signal_end_f,
        sweep_measurement_f.sampling_freq, 0.2)
    time_bins_b, freq_bins_b, spectrogram_b = compute_spectrogram(
        sweep_measurement_b.samples, signal_start_b, signal_end_b,
        sweep_measurement_b.sampling_freq, 0.2)

    present_plot(
        "spectrogram_f",
        plot_spectrogram(spectrogram_f,
                         time_bins_f,
                         freq_bins_f,
                         name="Forward sweep spectrogram"))
    present_plot(
        "spectrogram_b",
        plot_spectrogram(spectrogram_b,
                         time_bins_b,
                         freq_bins_b,
                         name="Backward sweep spectrogram"))

    # Analysis
    if as_numpy:
        analysis_freq = motor_steps / 4 * n
        analysis_f_freq_idx = np.argmin(np.abs(freq_bins_f - analysis_freq))
        analysis_b_freq_idx = np.argmin(np.abs(freq_bins_b - analysis_freq))

        f_response = [
            spectrum[analysis_f_freq_idx] for spectrum in spectrogram_f
        ]
        b_response = [
            spectrum[analysis_b_freq_idx] for spectrum in spectrogram_b
        ]
        b_response.reverse(
        )  # The sweep runs from end to start, but we want to compare it to the forward sweep
    else:
        time_bins_f, f_response = dft_sweep_motor_harmonic(
            sweep_measurement_f.samples, sweep_measurement_f.sampling_freq,
            signal_start_f, signal_end_f, speed, motor_steps, n, 10, 1)
        time_bins_b, b_response = dft_sweep_motor_harmonic(
            sweep_measurement_b.samples, sweep_measurement_b.sampling_freq,
            signal_start_b, signal_end_b, speed, motor_steps, n, 10, 1)
        b_response = b_response[::-1]

    mag_bins = np.linspace(mag_start, mag_end, len(time_bins_f))
    pha_bins = np.linspace(pha_start, pha_end, len(time_bins_b))

    trim_len = min(len(f_response), len(b_response))
    f_response = f_response[:trim_len]
    b_response = b_response[:trim_len]
    time_bins_f = time_bins_f[:trim_len]
    mag_bins = mag_bins[:trim_len]
    pha_bins = pha_bins[:trim_len]

    f_response, b_response = f_response + b_response, f_response + b_response

    # Plot an amplitude of analysis frequency for each window
    fig = go.Figure()
    fig.update_layout(
        title="Resonance analysis based on parameter sweep",
        yaxis_title="Magnitude",
        yaxis=dict(
            domain=[0.1, 1
                    ]  # Shrinks the plot area upwards to create space below
        ),
        xaxis=dict(
            title="Time (s)",
            domain=[0, 1],
            showgrid=True,  # Show grid only for main x-axis
            zeroline=True,
        ),
        xaxis2=dict(title="Mag",
                    overlaying="x",
                    side="bottom",
                    anchor="free",
                    position=0.05,
                    tickformat=".4f",
                    showgrid=False,
                    nticks=20),
        xaxis3=dict(title="Pha",
                    overlaying="x",
                    side="bottom",
                    anchor="free",
                    position=0.00,
                    tickformat=".2f",
                    showgrid=False,
                    nticks=20),
    )

    for name, response in [("Forward", f_response), ("Backward", b_response)]:
        fig.add_trace(
            go.Scatter(x=time_bins_f,
                       y=response,
                       name=name + " response to time"))
        fig.add_trace(
            go.Scatter(x=mag_bins,
                       y=response,
                       name=name + " response to mag",
                       xaxis="x2"))
        fig.add_trace(
            go.Scatter(x=pha_bins,
                       y=response,
                       name=name + " response to pha",
                       xaxis="x3"))

    if pha_start != pha_end:
        num_valleys = (pha_end - pha_start) // (2 * np.pi)
        to_pha_space = lambda x: (x / len(time_bins_f) *
                                  (pha_end - pha_start) + pha_start)

        f_valleys_idx = find_n_valleys(f_response, num_valleys)
        b_valleys_idx = find_n_valleys(b_response, num_valleys)

        for v_idx in f_valleys_idx:
            fig.add_trace(
                go.Scatter(x=[to_pha_space(v_idx)],
                           y=[f_response[v_idx]],
                           mode="markers",
                           marker=dict(color="blue"),
                           xaxis="x3",
                           showlegend=False))
        for v_idx in b_valleys_idx:
            fig.add_trace(
                go.Scatter(x=[to_pha_space(v_idx)],
                           y=[b_response[v_idx]],
                           mode="markers",
                           marker=dict(color="red"),
                           xaxis="x3",
                           showlegend=False))

        for v in [(a + b) // 2 for a, b in zip(f_valleys_idx, b_valleys_idx)]:
            fig.add_vline(x=time_bins_f[v],
                          line_dash="dash",
                          line_color="green",
                          annotation_text="Identified pha")

        for v in [(to_pha_space(a) + to_pha_space(b)) / 2
                  for a, b in zip(f_valleys_idx, b_valleys_idx)]:
            print(f"Candidate pha: {v % (2 * np.pi)}")

    present_plot("analysis", fig)


@click.command("findOptimalMagnitude")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--motor-steps", type=int, default=200)
@click.option("--axis", type=click.Choice(["X", "Y"]))
@click.option("--speed", type=float, default=1)
@click.option("--revs", type=float, default=1)
@click.option("--n", type=int, default=2)
@click.option("--mag-start", type=float, default=0.001)
@click.option("--mag-step", type=float, default=2)
@click.option("--pha-start", type=float, default=-2)
@click.option("--pha-end", type=float, default=14)
@click.option("--output",
              type=click.Path(dir_okay=True, file_okay=False),
              default=None,
              help="Output directory for data")
@click.option("--show", is_flag=True)
def findOptimalMagnitude(port, motor_steps, axis, speed, revs, n, mag_start,
                         mag_step, pha_start, pha_end, output, show):
    """
    Perform several parameter sweeps to locate optimal magnitude
    """
    if output is None and not show:
        print("No output specified, nothing to do. Specify --output or --show")
        return

    present_plot = lambda name, plot: store_or_show_plot(
        show, output, name, plot)

    fig = go.Figure()
    fig.update_layout(title="Optimal magnitude sweep",
                      yaxis_title="Magnitude",
                      xaxis_title="Phase")

    last_minimum = np.inf
    mag = mag_start
    gone_worse_count = 0
    with enabledMachineConnection(port=port) as machine:
        time.sleep(0.2)
        machine.command("G92 X0 Y0")

        for _ in range(20):
            print(f"Testing magnitude {mag:.5f}: ", end="")
            raw_output = machine.command(
                f"M973 {axis} R{revs:.10f} F{speed:.10f} H{n} A{pha_start:.10f} B{pha_end:.10f} C{mag:.10f} D{mag:.10f}"
            )
            sweep_measurement = SweepMeasurement.from_raw_command(raw_output)
            machine.command("G0 F10000 X0 Y0")

            if not sweep_measurement.is_ok():
                print("Movement error")
                continue

            signal_start_f, signal_end_f = locate_signal(sweep_measurement)
            time_bins_f, f_response = dft_sweep_motor_harmonic(
                sweep_measurement.samples, sweep_measurement.sampling_freq,
                signal_start_f, signal_end_f, speed, motor_steps, n, 10, 1)

            f_response = moving_average(f_response, len(f_response) // 20)

            fig.add_trace(
                go.Scatter(x=np.linspace(pha_start, pha_end, len(f_response)),
                           y=f_response,
                           name=f"Mag {mag:.5f}"))

            minimum = np.min(f_response)
            print(minimum)
            if minimum < last_minimum:
                gone_worse_count = 0
                last_minimum = minimum
            else:
                gone_worse_count += 1
                if gone_worse_count >= 2:
                    break

            mag *= mag_step

    present_plot("optimal_mag", fig)


@click.command("analyzeSpeedSweep")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--motor-steps", type=int, default=200)
@click.option("--axis", type=click.Choice(["X", "Y"]))
@click.option("--revs", type=float, default=5)
@click.option("--speed-start", type=float, default=0.5)
@click.option("--speed-end", type=float, default=3)
@click.option("--output",
              type=click.Path(dir_okay=True, file_okay=False),
              default=None,
              help="Output directory for data")
@click.option("--show", is_flag=True)
def analyzeSpeedSweep(port, motor_steps, axis, revs, speed_start, speed_end,
                      output, show):
    """
    Analyze speed sweep
    """

    if output is None and not show:
        print("No output specified, nothing to do. Specify --output or --show")
        return

    present_plot = lambda name, plot: store_or_show_plot(
        show, output, name, plot)

    with enabledMachineConnection(port=port) as machine:
        machine.command("M17")
        time.sleep(0.2)
        machine.command("G92 X0 Y0")
        raw_output = machine.command(
            f"M974 {axis} R{revs:.10f} A{speed_start:.10f} B{speed_end:.10f}")
        sweep_measurement_f = SweepMeasurement.from_raw_command(raw_output)

        raw_output = machine.command(
            f"M974 {axis} R{-revs:.10f} A{speed_start:.10f} B{speed_end:.10f}")
        machine.command("G0 F10000 X0 Y0")
        sweep_measurement_b = SweepMeasurement.from_raw_command(raw_output)

    present_plot(
        "raw_f",
        plot_raw_measurement(sweep_measurement_f,
                             name="Forward sweep raw data"))
    present_plot(
        "raw_b",
        plot_raw_measurement(sweep_measurement_b,
                             name="Backward sweep raw data"))

    for dir, meas in {
            "f": sweep_measurement_f,
            "b": sweep_measurement_b
    }.items():
        signal_start, signal_end = locate_signal(meas)
        time_bins, freq_bins, spectrogram = compute_spectrogram(
            meas.samples, signal_start, signal_end, meas.sampling_freq, 0.2)
        present_plot(
            f"spectrogram_{dir}",
            plot_accelerated_spectrogram(spectrogram,
                                         time_bins,
                                         freq_bins,
                                         speed_start,
                                         speed_end,
                                         name=f"{dir} sweep spectrogram"))

    harmonic_analysis = {}
    for h in [1, 2, 3, 4]:
        signal_start, signal_end = locate_signal(sweep_measurement_f)
        (speeds_f_up,
         profile_f_up), (speeds_f_down, profile_f_down) = analyze_speed_sweep(
             sweep_measurement_f.samples, sweep_measurement_f.sampling_freq,
             signal_start, signal_end, speed_start, speed_end, motor_steps, h,
             0.05)

        signal_start, signal_end = locate_signal(sweep_measurement_b)
        (speeds_b_up,
         profile_b_up), (speeds_b_down, profile_b_down) = analyze_speed_sweep(
             sweep_measurement_b.samples, sweep_measurement_b.sampling_freq,
             signal_start, signal_end, speed_start, speed_end, motor_steps, h,
             0.05)

        trim_len = min(len(speeds_f_up), len(speeds_b_up), len(speeds_f_down),
                       len(speeds_b_down))
        speeds_f_up = speeds_f_up[:trim_len]
        speeds_b_up = speeds_b_up[:trim_len]
        speeds_f_down = speeds_f_down[:trim_len]
        speeds_b_down = speeds_b_down[:trim_len]

        profile_f_up = profile_f_up[:trim_len]
        profile_b_up = profile_b_up[:trim_len]
        profile_f_down = profile_f_down[:trim_len]
        profile_b_down = profile_b_down[:trim_len]

        harmonic_analysis[h] = {
            "f_up": (speeds_f_up, profile_f_up),
            "f_down": (speeds_f_down, profile_f_down),
            "b_up": (speeds_b_up, profile_b_up),
            "b_down": (speeds_b_down, profile_b_down),
        }

    fig = go.Figure()
    fig.update_layout(title="Speed sweep analysis",
                      yaxis_title="Magnitude",
                      xaxis_title="Speed (mm/s)")
    colors = plotly.colors.DEFAULT_PLOTLY_COLORS
    for i, (h, data) in enumerate(harmonic_analysis.items()):
        color = colors[i]
        for name, (speeds, profile) in data.items():
            fig.add_trace(
                go.Scatter(x=speeds,
                           y=profile,
                           name=f"H{h} {name}",
                           mode="lines",
                           line=dict(color=color, width=1),
                           visible="legendonly"))

        combination = np.add(data["f_up"][1], data["f_down"][1])
        combination = np.add(combination, data["b_up"][1])
        combination = np.add(combination, data["b_down"][1])
        fig.add_trace(
            go.Scatter(x=data["f_up"][0],
                       y=combination,
                       name=f"H{h}",
                       mode="lines",
                       line=dict(color=color, width=2)))

    signals = []
    for h, data in harmonic_analysis.items():
        s = np.add(data["f_up"][1], data["f_down"][1])
        s = np.add(s, data["b_up"][1])
        s = np.add(s, data["b_down"][1])
        # It is cheaper to compute the moving average on the combined signal
        # than to perform hanning windowing
        s = moving_average(s, len(s) // 50)
        signals.append((h, s))

    speeds = harmonic_analysis[1]["f_up"][0]
    best_found, best_estimate = detect_harmonic_peaks(signals,
                                                      lambda idx: speeds[idx])

    print("Best found", best_found)
    print("Best estimate", best_estimate)

    for h, pos in best_found:
        fig.add_vline(x=pos,
                      line_dash="dash",
                      line_color=colors[h - 1],
                      annotation_text=f"Best found H{h}")
    for h, pos in best_estimate:
        fig.add_vline(x=pos,
                      line_dash="solid",
                      line_color=colors[h - 1],
                      annotation_text=f"Best estimate H{h}")

    present_plot("speed_sweep_analysis", fig)


@click.command("debugCalibration")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--axis", type=click.Choice(["X", "Y"]))
@click.option("--show", is_flag=True)
@click.option("--output",
              type=click.Path(dir_okay=True, file_okay=False),
              default=None,
              help="Output directory for data")
def debugCalibration(port, axis, show, output):
    """
    Debug calibration
    """

    if output is None and not show:
        print("No output specified, nothing to do. Specify --output or --show")
        return

    present_plot = lambda name, plot: store_or_show_plot(
        show, output, name, plot)

    def handle_raw_singal(obj):
        fig = go.Figure()
        tims = [
            i / obj["annotation"]["sampling_freq"]
            for i in range(len(obj["signal"]))
        ]
        fig.add_trace(go.Scatter(x=tims, y=obj["signal"]))

        start_marker, end_marker = obj["signal_bounds"]
        start_marker /= obj["annotation"]["sampling_freq"]
        end_marker /= obj["annotation"]["sampling_freq"]

        fig.add_vline(x=start_marker,
                      line_dash="dash",
                      line_color="red",
                      annotation_text="Signal start")
        fig.add_vline(x=end_marker,
                      line_dash="dash",
                      line_color="red",
                      annotation_text="Signal end")

        present_plot(obj["name"], fig)

    harmonic_data = {}

    def handle_dft_speed_sweep_result(obj):
        nonlocal harmonic_data
        harmonic = obj["harmonic"]
        name = obj["name"]

        if harmonic not in harmonic_data:
            harmonic_data[harmonic] = {}
        harmonic_data[harmonic][name] = obj

    smoothed_harmonic_data = {}

    def handle_smoothed_harmonic(obj):
        nonlocal smoothed_harmonic_data
        smoothed_harmonic_data[obj["harmonic"]] = obj

    detected_peaks = None

    def handle_detected_peaks(obj):
        nonlocal detected_peaks
        detected_peaks = obj

    mag_search_traces = {}

    def handle_mag_search(obj):
        harmonic = obj["harmonic"]
        if harmonic not in mag_search_traces:
            mag_search_traces[harmonic] = []
        mag_search_traces[harmonic].append(obj)

    param_search_traces = {}

    def handle_param_search(obj):
        harmonic = obj["harmonic"]
        if harmonic not in param_search_traces:
            param_search_traces[harmonic] = []
        param_search_traces[harmonic].append(obj)

    with enabledMachineConnection(port=port) as machine:
        raw_output = machine.command(f"M972 {axis}", timeout=60)

    for line in raw_output:
        if line.startswith("# raw_signal"):
            handle_raw_singal(json.loads(line.split(" ", maxsplit=2)[2]))
        if line.startswith("# dft_speed_sweep_result"):
            handle_dft_speed_sweep_result(
                json.loads(line.split(" ", maxsplit=2)[2]))
        if line.startswith("# smoothed_speed_sweep"):
            handle_smoothed_harmonic(json.loads(
                line.split(" ", maxsplit=2)[2]))
        if line.startswith("# harmonic_peaks"):
            handle_detected_peaks(json.loads(line.split(" ", maxsplit=2)[2]))
        if line.startswith("# magnitude_search"):
            handle_mag_search(json.loads(line.split(" ", maxsplit=2)[2]))
        if line.startswith("# param_search"):
            handle_param_search(json.loads(line.split(" ", maxsplit=2)[2]))

    # Plot all harmonic data
    fig = go.Figure()
    fig.update_layout(title="Speed sweep analysis",
                      yaxis_title="Magnitude",
                      xaxis_title="Speed (rev/s)")
    colors = plotly.colors.DEFAULT_PLOTLY_COLORS
    for h, data in harmonic_data.items():
        color = colors[h - 1]
        combination = None
        for name, obj in data.items():
            speeds = np.linspace(obj["start_x"], obj["end_x"],
                                 len(obj["signal"]))
            signal = np.asarray(obj["signal"])
            if combination is None:
                combination = signal
            else:
                combination = np.add(combination, signal)
            fig.add_trace(
                go.Scatter(x=speeds,
                           y=signal,
                           name=f"H{h} {name}",
                           mode="lines",
                           line=dict(color=color, width=1),
                           visible="legendonly"))

        fig.add_trace(
            go.Scatter(x=speeds,
                       y=combination,
                       name=f"H{h}",
                       mode="lines",
                       line=dict(color=color, width=2)))

    for peak in detected_peaks["peaks"]:
        harmonic = peak["harmonic"]
        meas_pos = peak["measured_position"]
        est_pos = peak["estimated_position"]
        fig.add_vline(x=meas_pos,
                      line_dash="dash",
                      line_color=colors[harmonic - 1],
                      annotation_text=f"Measured H{harmonic}")
        fig.add_vline(x=est_pos,
                      line_dash="solid",
                      line_color=colors[harmonic - 1],
                      annotation_text=f"Estimated H{harmonic}")

    present_plot("speed_sweep_analysis", fig)

    # Plot each magnitude search
    for h, traces in mag_search_traces.items():
        fig = go.Figure()
        fig.update_layout(title=f"Magnitude search H{h}",
                          yaxis_title="Magnitude",
                          xaxis_title="Magnitude")
        for trace in traces:
            fig.add_trace(
                go.Scatter(y=trace["response"],
                           name=f"Mag {trace['magnitude']}",
                           mode="lines"))
        present_plot(f"mag_search_H{h}", fig)

    # Plot each phase search
    for h, traces in param_search_traces.items():
        for dir in [1, -1]:
            dirname = "forward" if dir == 1 else "backward"
            fig = go.Figure()
            fig.update_layout(title=f"Phase search H{h} - {dirname}",
                              yaxis_title="Magnitude",
                              xaxis_title="Phase")
            for trace in [
                    x for x in traces
                    if x["move_dir"] == dir and x["mag_start"] == x["mag_end"]
            ]:
                x = np.linspace(trace["pha_start"], trace["pha_end"],
                                len(trace["response"]))
                fig.add_trace(
                    go.Scatter(
                        x=x,
                        y=trace["response"],
                        name=f"Pha {trace['pha_start']} → {trace['pha_end']}",
                        mode="lines"))
            present_plot(f"pha_search_H{h}_{dirname}", fig)

    # Plot each mag search
    for h, traces in param_search_traces.items():
        for dir in [1, -1]:
            dirname = "forward" if dir == 1 else "backward"
            fig = go.Figure()
            fig.update_layout(title=f"Magnitude search H{h} - {dirname}",
                              yaxis_title="Response",
                              xaxis_title="Magnitude")
            for trace in [
                    x for x in traces
                    if x["move_dir"] == dir and x["pha_start"] == x["pha_end"]
            ]:
                x = np.linspace(trace["mag_start"], trace["mag_end"],
                                len(trace["response"]))
                fig.add_trace(
                    go.Scatter(
                        x=x,
                        y=trace["response"],
                        name=f"Mag {trace['mag_start']} → {trace['mag_end']}",
                        mode="lines"))
            present_plot(f"mag_search_H{h}_{dirname}", fig)


@click.group()
def cli():
    """
    Test & calibration scripts
    """


cli.add_command(readLutCmd)
cli.add_command(writeLutCmd)
cli.add_command(analyzeParamSweep)
cli.add_command(findOptimalMagnitude)
cli.add_command(analyzeSpeedSweep)
cli.add_command(debugCalibration)

if __name__ == "__main__":
    cli()
