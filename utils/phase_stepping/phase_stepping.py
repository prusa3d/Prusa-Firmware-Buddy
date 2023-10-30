from __future__ import annotations
import itertools
import time

import click

import argparse
from copy import copy
import json
from contextlib import contextmanager
import sys
from typing import ClassVar, Generator, List, Optional, Tuple
from dataclasses import dataclass, field

import serial.tools.list_ports
from serial import Serial  # type: ignore

from scipy.fft import rfft, rfftfreq
import scipy.signal
import numpy as np
import plotly.graph_objects as go
from plotly.express.colors import sample_colorscale

from optimize import nelderMead, make2DSimplex

PRUSA_VID = 0x2c99
MOTOR_PERIOD = 1024


class Machine:
    def __init__(self, port: Serial) -> None:
        self._port = port
        self.accFreq = 1400

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
        # with self._preserveTimeout():
        #     self._port.timeout = 2
        #     while True:
        #         line = self._port.readline().decode("utf-8")
        #         if line == "":
        #             return

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

    def multiCommand(self, commands: List[str],
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

    def measureAccSampligFreq(self) -> None:
        response = self.command("M975")
        self.accFreq = [
            float(x.split(":")[1]) for x in response
            if x.startswith("sample freq:")
        ][0]


def getPrusaPort() -> Optional[str]:
    """
    Return first port that belongs to a Prusa machine
    """
    for port in serial.tools.list_ports.comports():
        if port.vid == PRUSA_VID:
            return port.device
    return None


@contextmanager
def machineConnection(port: str = getPrusaPort()
                      ) -> Generator[Machine, None, None]:
    with Serial(port) as s:
        yield Machine(s)


@contextmanager
def enabledMachineConnection(port: str = getPrusaPort()
                             ) -> Generator[Machine, None, None]:
    with Serial(port) as s:
        m = Machine(s)
        m.waitForBoot()
        m.command("M17")
        m.command("M970 X Y")
        yield m
        m.command("M18")


@dataclass
class PhaseCorrection:
    spectrum: List[Tuple[float, float]] = field(
        default_factory=lambda: [(0, 0) for _ in range(32)])

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


def adjustedCorrection(corr: PhaseCorrection, n: int, mag: float,
                       pha: float) -> PhaseCorrection:
    newC = PhaseCorrection(copy(corr.spectrum))
    newC.spectrum[n] = newC.spectrum[n][0] + mag, newC.spectrum[n][1] + pha
    return newC


def projectTo(what: Tuple[float, float], dir: Tuple[float, float]) -> float:
    l = np.sqrt(dir[0] * dir[0] + dir[1] * dir[1])
    return (what[0] * dir[0] + what[1] * dir[1]) / l


@dataclass
class ResonanceMeasurement:
    # Static measurement direction so we always alternate direction between
    # measurements
    samplesPerPeriod: ClassVar[int] = 1024

    speed: float  # rps
    realSamplingFreq: float
    rawSamples: List[Tuple[float, float]]

    @staticmethod
    def measure(machine: Machine,
                axis: str,
                speed: float,
                revs: float = 1,
                ret: bool = True,
                retries: int = 5) -> ResonanceMeasurement:
        try:
            machine.multiCommand(["M400", "G92 X0 Y0"])
            samples = captureAccSamples(machine, axis, revs, speed)
            if ret:
                machine.command("G0 F10000 X0 Y0")
        except RuntimeError:
            if ret:
                machine.command("G0 F10000 X0 Y0")
            if retries <= 0:
                raise
            return ResonanceMeasurement.measure(machine, axis, speed, revs,
                                                ret, retries - 1)

        freq = 1 / samples[1][0] - samples[0][0]
        realFreq = machine.accFreq
        if realFreq is None:
            realFreq = freq
        samples = [(x, y) for idx, x, y, z in samples]
        # ResonanceMeasurement.direction *= -1
        return ResonanceMeasurement(speed, realFreq, samples)

    @property
    def signal(self) -> List[float]:
        """
        Compute signal magnitude and resample it to the frequency of motor phase
        change.
        """
        motorFreq = self.speed * 50  # 200-step motor
        measDur = len(self.rawSamples) / self.realSamplingFreq
        expectedSampleCount = int(
            np.round(measDur * motorFreq * self.samplesPerPeriod))
        magnSig = scipy.signal.detrend(
            [projectTo(s, (1, 1)) for s in self.rawSamples])
        return scipy.signal.resample(magnSig, expectedSampleCount)

    @property
    def signalPeriodChunks(self) -> List[List[float]]:
        signal = self.signal
        signal = self.signal
        chunks = [
            signal[i:i + self.samplesPerPeriod]
            for i in range(0, len(signal), self.samplesPerPeriod)
        ]
        if len(chunks[-1]) != self.samplesPerPeriod:
            chunks.pop()
        return chunks[len(chunks) // 2:]

    @property
    def avgSignalPeriod(self) -> List[float]:
        chunks = self.signalPeriodChunks
        return np.sum(chunks, axis=0) / len(chunks)

    def periodPlot(self) -> go.Figure:
        fig = go.Figure()
        chunks = self.signalPeriodChunks
        traceColors = sample_colorscale("jet",
                                        list(np.linspace(0, 1, len(chunks))))
        for i, c in enumerate(chunks):
            fig.add_trace(go.Scatter(y=c, line=dict(color=traceColors[i])))
        fig.add_trace(
            go.Scatter(y=self.avgSignalPeriod,
                       name="Average",
                       mode="lines",
                       line=dict(color="black", width=7)))
        return fig

    def autocorrPlot(self) -> go.Figure:
        signal = self.signal
        autocorr = scipy.signal.correlate(signal, signal, mode='full')
        lags = np.arange(-len(signal) + 1, len(signal)) / self.samplesPerPeriod

        peaks, _ = scipy.signal.find_peaks(autocorr)
        peaksY = [autocorr[x] for x in peaks]
        peaksX = [lags[x] for x in peaks]

        fig = go.Figure()
        fig.add_trace(
            go.Scatter(x=lags,
                       y=autocorr,
                       mode="lines",
                       line=dict(color="red")))
        fig.add_trace(go.Scatter(x=peaksX, y=peaksY, mode="markers"))
        return fig

    def extractSamplingFreq(self) -> float:
        signal = scipy.signal.detrend(
            [projectTo(s, (1, 1)) for s in self.rawSamples])
        fft = rfft(signal)
        timebins = rfftfreq(len(signal), 1 / self.realSamplingFreq)

        indexMax = np.argmax(fft)
        fMax = timebins[indexMax]
        newFeq = self.realSamplingFreq * fMax / np.round(fMax)

        return newFeq

    def periodicityPlot(self) -> go.Figure:
        signal = scipy.signal.detrend(
            [projectTo(s, (1, 1)) for s in self.rawSamples])

        fft = rfft(signal)
        timebins = rfftfreq(len(signal), 1 / self.realSamplingFreq)

        fig = go.Figure()
        fig.update_layout(
            yaxis={
                "title": "Amplitude",
                "range": [0, 8000]
            },
            xaxis={"title": "Frequency"},
        )
        fig.add_trace(go.Scatter(y=np.abs(fft), x=timebins))
        return fig

    def plotSpectrum(self, spectrum) -> go.Figure:
        fig = go.Figure()
        fig.update_layout(
            yaxis={
                "title": "Amplitude",
                "range": [0, 1000]
            },
            xaxis={"title": "Frequency"},
        )
        fig.add_trace(
            go.Bar(x=list(range(16))[1:], y=[x[0] for x in spectrum][1:16]))
        return fig

    @property
    def averageSpectrum(self) -> List[Tuple[float, float]]:
        fft = rfft(self.avgSignalPeriod)
        return zip(np.abs(fft), np.angle(fft))

    @property
    def singleChunkSpectrum(self) -> List[Tuple[float, float]]:
        fft = rfft(self.signalPeriodChunks[0])
        return zip(np.abs(fft), np.angle(fft))

    @property
    def wholeSpectrum(self) -> List[Tuple[float, float]]:
        signal = self.signal
        periods = int(len(signal) / self.samplesPerPeriod)
        signal = signal[:periods * self.samplesPerPeriod]
        fft = rfft(signal)

        chunks = [fft[i:i + periods] for i in range(0, len(fft), periods)]
        chunkedFft = [np.mean(x) for x in chunks]

        return zip(np.abs(chunkedFft), np.angle(chunkedFft))

    @property
    def chunkedSpectrum(self) -> List[Tuple[float, float]]:
        chunks = self.signalPeriodChunks
        fftChunks = list(rfft(x) for x in chunks)
        mags = np.sum([np.abs(x) for x in fftChunks], axis=0) / len(fftChunks)
        phases = np.sum([np.angle(x)
                         for x in fftChunks], axis=0) / len(fftChunks)
        return zip(mags, phases)


def valuesInRange(start: float, end: float, count: int) -> List[float]:
    return [start + (end - start) * i / (count - 1) for i in range(count)]


def readLut(machine: Machine, axis: str) -> List[Tuple[int, int]]:
    rawResponse = machine.command(f"M972 {axis}")
    currents = [(0, 0) for _ in range(MOTOR_PERIOD)]

    for line in rawResponse:
        if not line.startswith(axis):
            continue
        values = line.split(",")
        phase = int(values[1])
        a = int(values[2])
        b = int(values[3])
        currents[phase] = (a, b)

    return currents


def writeLut(machine: Machine, axis: str,
             currents: List[Tuple[int, int]]) -> None:
    assert len(currents) == MOTOR_PERIOD
    assert all((-248 <= a <= 248) and (-248 <= b <= 248) for a, b in currents)

    commands = [
        f"M973 {axis} P{phase} A{a} B{b}"
        for phase, (a, b) in enumerate(currents)
    ]
    machine.multiCommand(commands)
    return


def captureAccSamples(machine: Machine, axis: str, revs: float,
                      speed: float) -> List[Tuple[float, float, float, float]]:
    """
    Move with the print head and capture samples. Return sample in the format: <time>, <x>, <y>, <z>
    """
    rawResponse = machine.command(f"M974 {axis} R{revs} F{speed}")
    if any("Error:" in x for x in rawResponse):
        textResponse = "\n".join(rawResponse)
        raise RuntimeError(
            f"Cannot capture acc samples for {axis}: {textResponse}")
    accSamples = [
        map(float,
            x.split(",")[1:]) for x in rawResponse if x[0].isdigit()
    ]
    sampleFreq = [
        float(x.split(":")[1]) for x in rawResponse
        if x.startswith("sample freq:")
    ]
    if len(sampleFreq) == 0 or sampleFreq[0] == 0:
        rawResponse = machine.command(f"M974 {axis} R{-revs} F{speed}")
        return captureAccSamples(machine, axis, revs, speed)
    samplePeriod = 1 / sampleFreq[0]

    try:
        return [(i * samplePeriod, x, y, z)
                for i, (x, y, z) in enumerate(accSamples)]
    except:
        print(accSamples, rawResponse)
        raise


def evaluateCorrection(machine: Machine, axis: str,
                       correction: PhaseCorrection,
                       speed: float) -> List[float]:
    """
    Given correction, return "badness" on individual harmonics
    """
    MEAS_COUNT = 1
    writeLut(machine, axis, correction.currents())
    measurements = [
        ResonanceMeasurement.measure(machine, axis, speed, revs=1)
        for _ in range(MEAS_COUNT)
    ]
    magnitudes = [map(lambda x: x[0], m.averageSpectrum) for m in measurements]
    return [np.median(samples) for samples in zip(*magnitudes)]


def reacalibrate(machine: Machine,
                 axis: str,
                 initialCorrection: PhaseCorrection,
                 n: int,
                 magRange: float,
                 phaseRange: float,
                 speed=float) -> Tuple[float, float]:
    """
    Given a machine and initial correction recalibrate magnitude and phase of
    the n-th harmonics. Returns a tuple (magnitude change, phase change)
    """
    print(f"Recalibrating {n}, {magRange}, {phaseRange}")
    evaluate = lambda x: evaluateCorrection(
        machine, axis, adjustedCorrection(initialCorrection, n, x[0], x[1]),
        speed)[n]

    #initialSimplex = make2DSimplex([0, 0], 0.01)
    initialSimplex = [[0, 0], [magRange, -phaseRange], [-magRange, phaseRange]]
    res, score = nelderMead(evaluate,
                            initialSimplex,
                            noImproveThr=1,
                            noImprovCount=5)
    return res


def findPhaseMin(machine: Machine,
                 axis: str,
                 initialCorrection: PhaseCorrection,
                 n: int,
                 mag: float,
                 speed=float) -> float:
    initialSimplex = [[np.pi / 3], [2 * np.pi / 3]]
    evaluate = lambda x: evaluateCorrection(
        machine, axis, adjustedCorrection(initialCorrection, n, mag, x), speed
    )[n]

    # res, score = nelderMead(evaluate, initialSimplex, noImproveThr=np.pi/100, noImprovCount=5)
    # return res[0]

    res, fval, _, calls = scipy.optimize.brent(evaluate,
                                               brack=[0, 2 * np.pi],
                                               tol=0.01,
                                               full_output=True)
    res = res % (2 * np.pi)
    print(f"Found f({res}) = {fval} in {calls}")
    return res


def findMagMin(machine: Machine,
               axis: str,
               initialCorrection: PhaseCorrection,
               n: int,
               maxMag: float,
               speed=float) -> float:
    initialSimplex = [[maxMag / 3], [2 * maxMag / 3]]
    evaluate = lambda x: evaluateCorrection(
        machine, axis, adjustedCorrection(initialCorrection, n, x, 0), speed)[n
                                                                              ]

    # res, score = nelderMead(evaluate, initialSimplex, noImproveThr=0.001, noImprovCount=5)
    # return res[0]
    res, fval, _, calls = scipy.optimize.brent(evaluate,
                                               brack=[0, maxMag],
                                               tol=0.001,
                                               full_output=True)
    print(f"Found f({res}) = {fval} in {calls}")
    return res


@click.command("readLut")
@click.option("--axis",
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
def readLutCmd(axis: str, port: str) -> None:
    with machineConnection(port) as machine:
        machine.waitForBoot()
        currents = readLut(machine, axis)
        json.dump(currents, sys.stdout, indent=4)


@click.command("writeLut")
@click.argument("filepath",
                type=click.Path(file_okay=True, exists=True, dir_okay=False))
@click.option("--axis",
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
def writeLutCmd(axis: str, port: str, filepath: str) -> None:
    with open(filepath) as f:
        correction = json.load(f)
    currents = PhaseCorrection(correction).currents()
    with machineConnection(port) as machine:
        machine.waitForBoot()
        writeLut(machine, axis, currents)


@click.command("analyseResonance")
@click.option("--axis",
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--speed", type=float, default=1, help="Test speed")
@click.option("--revs",
              type=float,
              default=1,
              help="Number of test revolutions")
def analyseResonanceCmd(axis: str, port: str, speed: float,
                        revs: float) -> None:
    with machineConnection(port) as machine:
        machine.waitForBoot()
        machine.command("M17")
        machine.command("M970 X Y")
        samples = captureAccSamples(machine, axis, revs, speed)
        machine.command("M18")

    magn = np.array([np.sqrt(x[1] * x[1] + x[2] * x[2]) for x in samples])
    magn -= np.mean(magn)
    amplitudes = np.absolute(rfft(magn))
    timeBins = rfftfreq(len(samples), samples[1][0] - samples[0][0])

    fig = go.Figure()

    fig.add_trace(
        go.Scatter(x=timeBins, y=amplitudes, mode="lines",
                   line_shape="spline"))
    fig.show()


@click.command("analyseResonanceRange")
@click.option("--axis", type=click.Choice(["X", "Y"]))
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--speedStart", type=float, default=0.5)
@click.option("--speedStep", type=float, default=0.1)
@click.option("--speedEnd", type=float, default=5)
@click.option("--revs", type=float, default=0.5)
def analyseResonanceRangeCmd(axis: str, port: str, speedstart: float,
                             speedend: float, speedstep: float,
                             revs: float) -> None:
    HARMONICS_COUNT = 16

    fig = go.Figure()
    fig.update_layout(
        margin=dict(l=0, r=0, t=20, b=0),
        yaxis={"title": "Amplitude"},
        xaxis={"title": "Speeds"},
    )

    forwardTraces = [[] for _ in range(HARMONICS_COUNT)]
    forwardMaxTrace = []
    backwardTraces = [[] for _ in range(HARMONICS_COUNT)]
    backwardMaxTrace = []
    speeds = []

    with machineConnection(port) as machine:
        machine.waitForBoot()
        machine.command("M17")
        machine.command("M970 X Y")

        testspeed = speedstart
        while testspeed <= speedend:
            speeds.append(testspeed)

            measurements = [
                ResonanceMeasurement.measure(machine, axis, testspeed,
                                             revs if i % 2 == 0 else -revs,
                                             False) for i in range(2)
            ]
            magnitudes = [[x[0] for x in m.averageSpectrum]
                          for m in measurements]

            for n in range(HARMONICS_COUNT):
                forwardTraces[n].append(magnitudes[0][n])
                backwardTraces[n].append(magnitudes[1][n])
            forwardMaxTrace.append(np.max(magnitudes[0][1:HARMONICS_COUNT]))
            backwardMaxTrace.append(np.max(magnitudes[1][1:HARMONICS_COUNT]))

            testspeed += speedstep
        machine.command("M18")

    for name, trace in {
            "Forward max": forwardMaxTrace,
            "Backward max": backwardMaxTrace
    }.items():
        fig.add_trace(
            go.Scatter(x=speeds,
                       y=trace,
                       name=name,
                       mode="lines",
                       line=dict(color="black", width=4)))

    traceColors = sample_colorscale(
        "jet", list(np.linspace(0, 1, len(forwardTraces))))
    for n in range(HARMONICS_COUNT):
        fig.add_trace(
            go.Scatter(x=speeds,
                       y=forwardTraces[n],
                       name=f"F {n}",
                       line=dict(color=traceColors[n])))
    for n in range(HARMONICS_COUNT):
        fig.add_trace(
            go.Scatter(x=speeds,
                       y=backwardTraces[n],
                       name=f"B {n}",
                       visible='legendonly',
                       line=dict(color=traceColors[n])))

    fig.show()


@click.command("calibrate")
@click.option("--axis",
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--speed", type=float, default=1.5)
@click.option("--output",
              type=click.Path(file_okay=True, dir_okay=False),
              default=None)
def calibrateCmd(axis: str, port: str, speed: float, output: str) -> None:
    with machineConnection(port) as machine:
        machine.waitForBoot()
        machine.command("M17")
        machine.command("M970 X Y")

        correction = PhaseCorrection()
        correction.spectrum[2] = (0.05, 3.42)
        correction.spectrum[4] = (0.043, 1.14)

        mag, pha = reacalibrate(machine, axis, correction, 4, 0.05, 0.3, speed)
        correction = adjustedCorrection(correction, 4, mag, pha)

        mag, pha = reacalibrate(machine, axis, correction, 4, 0.01, 0.1,
                                speed / 2)
        correction = adjustedCorrection(correction, 4, mag, pha)

        mag, pha = reacalibrate(machine, axis, correction, 2, 0.01, 0.3, speed)
        correction = adjustedCorrection(correction, 2, mag, pha)

        mag, pha = reacalibrate(machine, axis, correction, 2, 0.01, 0.1,
                                speed / 2)
        correction = adjustedCorrection(correction, 2, mag, pha)

        mag, pha = reacalibrate(machine, axis, correction, 8, 0.05, np.pi,
                                speed)
        correction = adjustedCorrection(correction, 8, mag, pha)

        mag, pha = reacalibrate(machine, axis, correction, 8, 0.01, 0.1,
                                speed / 2)
        correction = adjustedCorrection(correction, 8, mag, pha)

        # for n in [3, 5, 6, 7, 8]:
        #     mag, pha = reacalibrate(machine, axis, correction, n, 0.1, 2 * np.pi, speed)
        #     correction = adjustedCorrection(correction, n, mag, pha)

        writeLut(machine, "X", correction.currents())

    if output is not None:
        with open(output, "w") as f:
            json.dump(correction.spectrum, f)

    print(correction.spectrum)


@click.command("calibrateIndiv")
@click.option("--axis",
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--speed", type=float, default=1.5)
@click.option("--output",
              type=click.Path(file_okay=True, dir_okay=False),
              default=None)
def calibrateIndivCmd(axis: str, port: str, speed: float, output: str) -> None:
    with machineConnection(port) as machine:
        machine.waitForBoot()
        machine.command("M17")
        machine.command("M970 X Y")

        def calib(initialCorrection: PhaseCorrection, n: int,
                  expectedMag: float) -> PhaseCorrection:
            correction = PhaseCorrection(initialCorrection.spectrum)
            pha = findPhaseMin(machine, axis, correction, n, expectedMag,
                               speed)
            print(f"Got min {n}. pha: {pha}")
            correction.spectrum[n] = (correction.spectrum[n][0], pha)
            mag = findMagMin(machine, axis, correction, n, 2 * expectedMag,
                             speed)
            print(f"Got min {n}. mag: {mag}")
            correction.spectrum[n] = (mag, correction.spectrum[n][1])
            return correction

        c = PhaseCorrection()
        c = calib(c, 2, 0.05)
        c = calib(c, 4, 0.05)

        writeLut(machine, "X", c.currents())

    print(c.spectrum)
    if output is not None:
        with open(output, "w") as f:
            json.dump(c.spectrum, f)


@click.command("analyzeSamples")
@click.option("--axis",
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--speed", type=float, default=1)
@click.option("--revs", type=float, default=0.2)
def analyzeSamplesCmd(axis: str, port: str, speed: float, revs: float):
    """
    Plot sample analysis
    """
    with enabledMachineConnection(port) as machine:
        print("Measuring accelerometer")
        # machine.measureAccSampligFreq()
        print(machine.accFreq)
        measurements = [
            ResonanceMeasurement.measure(machine, axis, speed, revs)
            for _ in range(1)
        ]

    for m in measurements:
        # m.realSamplingFreq = m.extractSamplingFreq()
        print(m.realSamplingFreq)
        m.periodPlot().show()
        m.autocorrPlot().show()
        m.plotSpectrum(m.averageSpectrum).show()
        # m.periodicityPlot().show()
        break


@click.command("analyzeConsistency")
@click.option("--axis",
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--speed", type=float, default=1)
@click.option("--revs", type=float, default=0.2)
@click.option("--n", type=int, default=20)
def analyzeConsistencyCmd(axis: str, port: str, speed: float, revs: float,
                          n: int):
    """
    Analyze consistency of measurement
    """
    with enabledMachineConnection(port) as machine:
        print("Measuring accelerometer")
        # # machine.measureAccSampligFreq()
        print(machine.accFreq)
        measurements = [
            ResonanceMeasurement.measure(machine, axis, speed, revs)
            for _ in range(n)
        ]
    fig = go.Figure()
    fig.update_layout(
        yaxis={
            "title": "Amplitude",
            "range": [0, 10000]
        },
        xaxis={"title": "Harmonic"},
    )
    for i, meas in enumerate(measurements):
        fig.add_trace(
            go.Bar(x=list(range(16))[1:],
                   y=[x[0] for x in meas.averageSpectrum][1:16],
                   name=f"{'R' if i % 2 == 1 else 'F'}{i // 2}"))
    fig.show()


@click.command("plotCorrectionValues")
@click.option("--axis",
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--speed", type=float, default=1.4)
@click.option("--n", type=int, default=2)
@click.option("--mag", type=float, default=0.02)
@click.option("--segments", type=int, default=10)
@click.option("--output", type=click.Path(), default=None)
def plotCorrectionValuesCmd(axis: str, port: str, speed: float, n: int,
                            mag: float, segments: int, output: Optional[str]):
    samples = {}
    i = 0
    with enabledMachineConnection(port) as machine:
        machine.command("M906 X450 Y650")
        for phase, mag in itertools.product(
                valuesInRange(0, 2 * np.pi, segments),
                valuesInRange(0, mag, segments)):
            i += 1
            print(
                f"Measuring {i}/{segments * segments}: {phase:.3f}, {mag:.5f}: ",
                end="")
            sys.stdout.flush()
            correction = PhaseCorrection()
            correction.spectrum[n] = (mag, phase)
            badness = evaluateCorrection(machine, axis, correction, speed)[n]
            print(badness)
            samples[(phase, mag)] = badness
    print(samples)

    unique_x = sorted(set(x for x, y in samples.keys()))
    unique_y = sorted(set(y for x, y in samples.keys()))

    x, y = np.meshgrid(unique_x, unique_y)

    # Step 3: Create a grid for z
    z = np.zeros_like(x, dtype=float)
    for i, xi in enumerate(unique_x):
        for j, yj in enumerate(unique_y):
            z[j, i] = samples.get((xi, yj), np.nan)

    fig = go.Figure()
    fig.update_layout(margin=dict(l=0, r=0, t=0, b=0),
                      yaxis={"title": "magnitude"},
                      xaxis={"title": "phase"})
    fig.add_trace(go.Surface(x=x, y=y, z=z))
    if output is not None:
        fig.write_html(output)
    fig.show()


@click.group()
def cli():
    """
    Test & calibration scripts
    """


cli.add_command(readLutCmd)
cli.add_command(writeLutCmd)
cli.add_command(analyseResonanceCmd)
cli.add_command(analyseResonanceRangeCmd)
cli.add_command(calibrateCmd)
cli.add_command(calibrateIndivCmd)
cli.add_command(analyzeSamplesCmd)
cli.add_command(analyzeConsistencyCmd)
cli.add_command(plotCorrectionValuesCmd)

if __name__ == "__main__":
    cli()
