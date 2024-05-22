from collections import OrderedDict
import sys
from typing import Callable, Tuple
import numpy as np


def make2DSimplex(origin, distance):
    numPoints = 3
    angle = 2 * np.pi / numPoints

    points = []
    for i in range(numPoints):
        x = origin[0] + distance * np.cos(i * angle)
        y = origin[1] + distance * np.sin(i * angle)
        points.append((x, y))
    return points


def nelderMead(f,
               initialSimplex,
               noImproveThr=10e-6,
               noImprovCount=10,
               maxIter=100,
               alpha=1.,
               gamma=2.,
               rho=-0.5,
               sigma=0.5):
    dim = len(initialSimplex[0])

    print("Initial measurement")

    target = [(np.array(p), f(p)) for p in initialSimplex]
    prevBestScore = None
    noImprov = 0

    for iteration in range(maxIter):
        target.sort(key=lambda x: x[1])
        simplex = [t[0] for t in target]
        bestScore = target[0][1]
        secondWorstScore = target[-2][1]
        print(f"Iteration {iteration} - {bestScore}")
        if prevBestScore is not None:
            print(f"{prevBestScore - bestScore}")

        print(f"  {simplex}, {bestScore}")

        if prevBestScore is not None and prevBestScore - bestScore < noImproveThr:
            noImprov += 1
        else:
            noImprov = 0

        if noImprov >= noImprovCount:
            return target[0]

        if prevBestScore is None or bestScore <= prevBestScore:
            prevBestScore = bestScore
        else:
            assert False

        centroid = np.sum(simplex[:-1], axis=0) / dim

        # Reflect...
        reflection = centroid + alpha * (centroid - simplex[-1])
        reflectionScore = f(reflection)
        print(f"  reflection: {reflection}: {reflectionScore}")

        if bestScore <= reflectionScore < secondWorstScore:
            target[-1] = (reflection, reflectionScore)
            continue

        if reflectionScore < bestScore:
            # ... expand
            expansion = centroid + gamma * (centroid - simplex[-1])
            expansionScore = f(expansion)
            print(f"  expansion: {expansion}: {expansionScore}")
            if expansionScore < reflectionScore:
                target[-1] = (expansion, expansionScore)
                continue
            else:
                target[-1] = (reflection, reflectionScore)
                continue

        # Contract
        contraction = centroid + rho * (centroid - simplex[-1])
        contractionScore = f(contraction)
        print(f"  contraction: {contraction}: {contractionScore}")
        if contractionScore < bestScore:
            target[-1] = (contraction, contractionScore)
            continue

        # Shrink
        print("  shrink")
        for i in range(1, dim + 1):
            p = simplex[0] + sigma * (simplex[i] - simplex[0])
            target[i] = (p, f(p))
            print(f"    {simplex[i]} -> {target[i][0]} {target[i][1]}")

    target.sort(key=lambda x: x[1])
    return target[0]


def goldenSearch(func: Callable[[float], float],
                 brack: Tuple[float, float],
                 tol=1e-5,
                 maxiter: int = 1000):
    phi = (1 + 5**0.5) / 2.0
    invPhi = 1 / phi

    # Initial bracketing
    a, b = brack
    c = b - invPhi * (b - a)
    d = a + invPhi * (b - a)

    fc, fd = func(c), func(d)
    print(f"  Initial bracket ({c}, {d}) = ({fc}, {fd})")

    numIterations = 0
    while abs(b - a) > tol and numIterations < maxiter:
        if fc < fd:
            b, d, fd = d, c, fc
            c = b - invPhi * (b - a)
            fc = func(c)
        else:
            a, c, fc = c, d, fd
            d = a + invPhi * (b - a)
            fd = func(d)
        print(f"  Iteration bracket ({c}, {d}) = ({fc}, {fd})")
        numIterations += 1

    if fc < fd:
        return c, fc, numIterations
    else:
        return d, fd, numIterations


def intervalSearch(func: Callable[[float], float],
                   brack: Tuple[float, float],
                   tol=1e-5,
                   maxiter: int = 1000,
                   n: int = 4):
    a, b = brack
    print(f"  Initial interval ({a}, {b})")
    resArg, resVal = None, None
    funcCalls = 0
    arg = np.linspace(a, b, n)
    dInt = arg[1] - arg[0]

    while dInt > tol:
        print(f"  Eval {arg} = ", end="")
        sys.stdout.flush()

        fx = [func(x) for x in arg]
        funcCalls += len(fx)

        print(f" {fx}")

        i = np.argmin(fx)
        resArg = arg[i]
        resVal = fx[i]

        a, b = arg[i] - 4 * dInt / 7, arg[i] + 4 * dInt / 7
        print(f"  Interval interval ({a}, {b})")
        arg = np.linspace(a, b, n)
        dInt = arg[1] - arg[0]

    return resArg, resVal, funcCalls
