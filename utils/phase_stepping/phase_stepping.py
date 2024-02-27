from __future__ import annotations
import itertools
import time

import click

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

from optimize import intervalSearch, nelderMead, make2DSimplex, goldenSearch

PRUSA_VID = 0x2c99
MOTOR_PERIOD = 1024


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
        ][-1]


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
            samples, freq = captureAccSamples(machine, axis, revs, speed)
            if ret:
                machine.command("G0 F10000 X0 Y0")
        except RuntimeError:
            if ret:
                machine.command("G0 F10000 X0 Y0")
            if retries <= 0:
                raise
            return ResonanceMeasurement.measure(machine, axis, speed, revs,
                                                ret, retries - 1)

        realFreq = machine.accFreq
        if realFreq is None:
            realFreq = freq
        samples = [(x, y) for x, y, z in samples]
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

    def rawWholePlot(self) -> go.Figure:
        fig = go.Figure()

        trace = [(i / self.realSamplingFreq, projectTo(s, (1, 1)))
                 for i, s in enumerate(self.rawSamples)]
        fig.add_trace(
            go.Scatter(x=[x[0] for x in trace], y=[x[1] for x in trace]))

        return fig

    def rawPeriodPlot(self) -> go.Figure:
        fig = go.Figure()

        trace = [(i / self.realSamplingFreq, projectTo(s, (1, 1)))
                 for i, s in enumerate(self.rawSamples)]
        period = 1 / (self.speed * 50)
        chunks = []
        i = 0
        while i < len(trace):
            l = []
            limit = (1 + len(chunks)) * period
            while i < len(trace) and trace[i][0] < limit:
                l.append((trace[i][0] + period - limit, trace[i][1]))
                i += 1
            if len(l) > 0:
                chunks.append(l)

        traceColors = sample_colorscale("jet",
                                        list(np.linspace(0, 1, len(chunks))))
        for i, c in enumerate(chunks):
            fig.add_trace(
                go.Scatter(x=[x[0] for x in c],
                           y=[x[1] for x in c],
                           line=dict(color=traceColors[i])))
        return fig

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
                "range": [0, 1500]
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

    @property
    def directSpectrum(self) -> List[Tuple[float, float]]:
        motorPeriodDuration = 1 / (self.speed * 50)  # 200-step motor
        samplingDuration = len(self.rawSamples) / self.realSamplingFreq
        motorPeriodCount = int(samplingDuration / motorPeriodDuration)
        relevantSamplesCount = int(motorPeriodCount * motorPeriodDuration /
                                   samplingDuration * len(self.rawSamples))

        signal = [projectTo(s, (1, 1)) for s in self.rawSamples]

        fft = rfft(signal[:relevantSamplesCount])

        mags = np.abs(fft)
        phas = np.angle(fft)

        spectrum = [(0, 0)]
        while True:
            idx = len(spectrum) * motorPeriodCount
            if idx >= len(fft):
                break
            spectrum.append(((mags[idx - 1] + mags[idx] + mags[idx + 1]) / 3,
                             (phas[idx - 1] + phas[idx] + phas[idx + 1]) / 3))
        return spectrum


def valuesInRange(start: float, end: float, count: int) -> List[float]:
    return [start + (end - start) * i / (count - 1) for i in range(count)]


def readLut(machine: Machine, axis: str, direction: str) -> PhaseCorrection:
    rawResponse = machine.command(f"M972 {axis} {direction}")

    correction = PhaseCorrection()
    for line in rawResponse:
        if not line.startswith(axis):
            continue
        values = line.split(",")
        dir = values[1].strip()
        if dir != direction:
            continue
        n = int(values[2])
        mag = float(values[3])
        pha = float(values[4])

        correction.spectrum[n] = (mag, pha)
    return correction


def writeLut(machine: Machine, axis: str, direction: str,
             correction: PhaseCorrection) -> None:
    tableStr = " ".join(
        [f"{mag:.7g},{pha:.7g}" for mag, pha in correction.spectrum[:16]])
    command = f"M973 {axis}{direction} {tableStr}"
    if len(command) > 256:
        raise RuntimeError("Too long LUT command")
    machine.command(command)
    return


def captureAccSamples(machine: Machine, axis: str, revs: float, speed: float
                      ) -> Tuple[List[Tuple[float, float, float, int]], float]:
    """
    Move with the print head and capture samples. Return sample in the format: <x>, <y>, <z>, <time_ticks>
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
    freq = [
        float(x.split(":")[1]) for x in rawResponse
        if x.startswith("sample freq:")
    ][0]

    return [(x, y, z) for x, y, z in accSamples], freq


def evaluateCorrection(machine: Machine, axis: str, direction: str,
                       correction: PhaseCorrection,
                       speed: float) -> List[float]:
    """
    Given correction, return "badness" of individual harmonics
    """
    assert axis in "XY"
    assert direction in "FB"

    MEAS_COUNT = 1
    REVS = 0.5
    writeLut(machine, axis, direction, correction)
    measurements = [
        ResonanceMeasurement.measure(machine,
                                     axis,
                                     speed,
                                     revs=REVS if direction == "B" else -REVS)
        for _ in range(MEAS_COUNT)
    ]
    magnitudes = [map(lambda x: x[0], m.directSpectrum) for m in measurements]
    return [np.median(samples) for samples in zip(*magnitudes)]


def reacalibrate(machine: Machine,
                 axis: str,
                 direction: str,
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
        machine, axis, direction,
        adjustedCorrection(initialCorrection, n, x[0], x[1]), speed)[n]

    #initialSimplex = make2DSimplex([0, 0], 0.01)
    initialSimplex = [[0, 0], [magRange, -phaseRange], [-magRange, phaseRange]]
    res, score = nelderMead(evaluate,
                            initialSimplex,
                            noImproveThr=1,
                            noImprovCount=5)
    return res


def findPhaseMin(machine: Machine,
                 axis: str,
                 direction: str,
                 initialCorrection: PhaseCorrection,
                 n: int,
                 mag: float,
                 speed: float,
                 guess: Optional[float] = None) -> float:
    evaluate = lambda x: evaluateCorrection(
        machine, axis, direction,
        adjustedCorrection(initialCorrection, n, mag, x), speed)[n]
    print(f"Finding pha {n}")
    if guess is None:
        brack = [0, 2 * np.pi]
    else:
        GUESS_WINDOW = 0.5
        brack = [guess - GUESS_WINDOW, guess + GUESS_WINDOW]

    res, fval, calls = goldenSearch(evaluate, brack=brack, tol=0.005)
    res = res % (2 * np.pi)
    print(f"Found f({res}) = {fval} in {calls}")
    return res


def findMagMin(machine: Machine,
               axis: str,
               direction: str,
               initialCorrection: PhaseCorrection,
               n: int,
               maxMag: float,
               speed: float,
               guess: Optional[float] = None) -> float:
    evaluate = lambda x: evaluateCorrection(
        machine, axis, direction, adjustedCorrection(initialCorrection, n, x, 0
                                                     ), speed)[n]
    print(f"Finding mag {n}")
    if guess is None:
        brack = [0, maxMag]
    else:
        GUESS_WINDOW = 0.1
        brack = [guess - GUESS_WINDOW, guess + GUESS_WINDOW]

    res, fval, calls = goldenSearch(evaluate, brack=brack, tol=0.0002)
    print(f"Found f({res}) = {fval} in {calls}")
    return res


@click.command("readLut")
@click.option("--axis",
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--dir",
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
                type=click.Path(file_okay=True, exists=True, dir_okay=False))
@click.option("--axis",
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--dir",
              type=click.Choice(["F", "B"]),
              help="Table for which direction")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
def writeLutCmd(axis: str, dir: str, port: str, filepath: str) -> None:
    with open(filepath) as f:
        correction = json.load(f)
    with machineConnection(port) as machine:
        machine.waitForBoot()
        writeLut(machine, axis, dir, PhaseCorrection(correction))


@click.command("analyseResonance")
@click.option("--axis", type=click.Choice(["X", "Y"]))
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--speedStart", type=float, default=0.5)
@click.option("--speedStep", type=float, default=0.1)
@click.option("--speedEnd", type=float, default=5)
@click.option("--revs", type=float, default=0.5)
def analyseResonanceCmd(axis: str, port: str, speedstart: float,
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


@click.command("calibrateIndiv")
@click.option("--axis",
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--dir",
              type=click.Choice(["F", "B"]),
              help="Table for which direction")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--speed", type=float, default=1.4)
@click.option("--output",
              type=click.Path(file_okay=True, dir_okay=False),
              default=None)
def calibrateIndivCmd(axis: str, dir: str, port: str, speed: float,
                      output: str) -> None:
    with machineConnection(port) as machine:
        machine.waitForBoot()
        machine.command("M17")
        machine.command("M970 X Y")

        def calib(initialCorrection: PhaseCorrection, n: int,
                  expectedMag: float) -> PhaseCorrection:
            baseline = evaluateCorrection(machine, axis, dir,
                                          initialCorrection, speed)[n]
            print(f"Baseline for {n}: {baseline}")
            correction = PhaseCorrection(initialCorrection.spectrum)
            pha = findPhaseMin(machine, axis, dir, correction, n, expectedMag,
                               speed)
            print(f"Got min {n}. pha: {pha}")
            correction.spectrum[n] = (correction.spectrum[n][0], pha)
            mag = findMagMin(machine, axis, dir, correction, n,
                             2 * expectedMag, speed)
            print(f"Got min {n}. mag: {mag}")
            correction.spectrum[n] = (mag, correction.spectrum[n][1])

            # print("Refinment")
            # correction.spectrum[n] = (correction.spectrum[n][0], 0)
            # pha = findPhaseMin(machine, axis, dir, correction, n, mag,
            #                    speed, pha)
            # print(f"Got ref min {n}. pha: {pha}")
            # correction.spectrum[n] = (0, pha)
            # mag = findMagMin(machine, axis, dir, correction, n,
            #                  2 * expectedMag, speed, mag)
            # print(f"Got ref min {n}. mag: {mag}")
            # correction.spectrum[n] = (mag, correction.spectrum[n][1])
            return correction

        c = PhaseCorrection()
        c = calib(c, 2, 0.028)
        c = calib(c, 4, 0.022)
        # c = calib(c, 8, 0.005)
        # c = calib(c, 3, 0.005)

        writeLut(machine, "X", dir, c)

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
        measurement = ResonanceMeasurement.measure(machine, axis, speed, revs)

    # measurement.periodPlot().show()
    # measurement.autocorrPlot().show()
    # measurement.plotSpectrum(measurement.averageSpectrum).show()
    measurement.plotSpectrum(measurement.directSpectrum).show()
    measurement.rawPeriodPlot().show()
    measurement.rawWholePlot().show()
    # measurement.periodicityPlot().show()


@click.command("analyzeMachineSamples")
@click.option("--axis",
              type=click.Choice(["X", "Y"]),
              help="Axis for reading LUT")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--speed", type=float, default=1)
@click.option("--revs", type=float, default=0.2)
def analyzeMachineSamplesCmd(axis: str, port: str, speed: float, revs: float):
    """
    Plot sample analysis performed by the machine
    """
    with enabledMachineConnection(port) as machine:
        machine.multiCommand(["M400", "G92 X0 Y0"])
        rawResponse = machine.command(f"M976 {axis} R{revs} F{speed}")
        machine.command("G0 F10000 X0 Y0")

    print(rawResponse)
    dataPoints = [float(x.split(":")[1]) for x in rawResponse]

    fig = go.Figure()
    fig.update_layout(
        yaxis={
            "title": "Amplitude",
            # "range": [0, 1500]
        },
        xaxis={"title": "Frequency"},
    )
    fig.add_trace(go.Bar(x=list(range(len(dataPoints)))[1:], y=dataPoints[1:]))
    fig.show()


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
    Analyze consistency of measurement by repeated measurement and comparing the
    results.
    """
    with enabledMachineConnection(port) as machine:
        measurements = [
            ResonanceMeasurement.measure(machine, axis, speed, revs)
            for _ in range(n)
        ]
    fig = go.Figure()
    fig.update_layout(
        yaxis={
            "title": "Amplitude",
            "range": [0, 3000]
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
@click.option("--dir",
              type=click.Choice(["F", "B"]),
              default="F",
              help="Table for which direction")
@click.option("--port", type=str, default=getPrusaPort(), help="Machine port")
@click.option("--speed", type=float, default=1.4)
@click.option("--n", type=int, default=2)
@click.option("--mag", type=float, default=0.02)
@click.option("--segments", type=int, default=10)
@click.option("--output", type=click.Path(), default=None)
def plotCorrectionValuesCmd(axis: str, dir: str, port: str, speed: float,
                            n: int, mag: float, segments: int,
                            output: Optional[str]):
    samples = {}
    i = 0
    with enabledMachineConnection(port) as machine:
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
            badness = evaluateCorrection(machine, axis, dir, correction,
                                         speed)[n]
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
cli.add_command(calibrateIndivCmd)
cli.add_command(analyzeSamplesCmd)
cli.add_command(analyzeMachineSamplesCmd)
cli.add_command(analyzeConsistencyCmd)
cli.add_command(plotCorrectionValuesCmd)

if __name__ == "__main__":
    cli()
