from .phase_stepping import PhaseCorrection
import numpy as np


def test_phase_correction():
    c1 = PhaseCorrection()
    c1.spectrum[2] = (1, 0)

    c2 = PhaseCorrection()
    c2.spectrum[2] = (1, 2 * np.pi)

    c3 = PhaseCorrection()
    c3.spectrum[2] = (1, np.pi)

    assert c1.currents() == c2.currents()
    assert c1.currents() != c3.currents()
