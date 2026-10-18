"""Sanity checks on a .pzrec take (Experiment 1 groundwork). Standard library only.

    python tools/analyze_take.py <take.pzrec>

Checks: matrix layout (where translation lives), bone rigidity across frames, which axis is up,
left/right sides, character height, frame timing and distance travelled.
"""

import json
import math
import os
import statistics
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pzstream import FRAME, HELLO, PZREC_MAGIC, SKELETON, parse_frame, read_messages  # noqa: E402


def load(path):
    with open(path, "rb") as f:
        data = f.read()
    if not data.startswith(PZREC_MAGIC):
        sys.exit("not a .pzrec v1 file")
    pos = len(PZREC_MAGIC)

    def read_exact(n):
        nonlocal pos
        if pos + n > len(data):
            raise EOFError
        pos += n
        return data[pos - n:pos]

    hello, skeleton, frames = None, None, []
    try:
        for msg_type, payload in read_messages(read_exact):
            if msg_type == HELLO:
                hello = json.loads(payload)
            elif msg_type == SKELETON:
                skeleton = json.loads(payload)
            elif msg_type == FRAME:
                frames.append(parse_frame(payload))
    except EOFError:
        pass
    return hello, skeleton, frames


def translation(m, i):
    base = i * 16
    return (m[base + 3], m[base + 7], m[base + 11])


def dist(a, b):
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))


def main():
    hello, skeleton, frames = load(sys.argv[1])
    names = [b["name"] for b in skeleton["bones"]]
    parents = [b["parent"] for b in skeleton["bones"]]
    idx = {n: i for i, n in enumerate(names)}
    print("game %s, %d bones, %d frames" % (hello["game"], len(names), len(frames)))

    # 1. Matrix layout: translation should be in m03/m13/m23 and m30..m32 should stay ~0.
    col = statistics.mean(abs(f["matrices"][i * 16 + k]) for f in frames[::50] for i in range(len(names)) for k in (3, 7, 11))
    row = statistics.mean(abs(f["matrices"][i * 16 + k]) for f in frames[::50] for i in range(len(names)) for k in (12, 13, 14))
    print("\n[layout] mean |m03,m13,m23| = %.4f   mean |m30,m31,m32| = %.6f   -> translation in %s"
          % (col, row, "m03/m13/m23 (as decompiled)" if col > 100 * max(row, 1e-9) else "UNEXPECTED"))

    # 2. Rigidity: parent-child distance should be constant (no stretch) except twist/helper bones.
    print("\n[rigidity] bone length variation across all frames (max-min)/mean:")
    worst = []
    for i, p in enumerate(parents):
        if p < 0:
            continue
        lengths = [dist(translation(f["matrices"], i), translation(f["matrices"], p)) for f in frames]
        mean = statistics.mean(lengths)
        if mean < 1e-4:
            continue
        worst.append(((max(lengths) - min(lengths)) / mean, names[i], mean))
    worst.sort(reverse=True)
    for spread, name, mean in worst[:5]:
        print("  %-22s length %.4f  spread %.2f%%" % (name, mean, spread * 100))
    rigid = sum(1 for s, _, _ in worst if s < 0.01)
    print("  %d of %d bones vary by less than 1%%" % (rigid, len(worst)))

    # 3. Up axis and height, measured on the first frame (model space).
    f0 = frames[0]["matrices"]
    head = translation(f0, idx["Bip01_Head"])
    feet = [translation(f0, idx[n]) for n in ("Bip01_L_Toe0", "Bip01_R_Toe0", "Bip01_L_Foot", "Bip01_R_Foot") if n in idx]
    up = max(range(3), key=lambda a: abs(head[a] - min(p[a] for p in feet)))
    height = head[up] - min(p[up] for p in feet)
    print("\n[axes] up axis in model space: %s; head above lowest foot joint: %.4f model units"
          % ("XYZ"[up], height))
    print("        = %.2f tiles (x1.5)  -> if the head joint sits ~1.65 m up, 1 model unit ~= %.2f m"
          % (height * 1.5, 1.65 / height))

    # 4. Sides: which way does the left hand sit relative to the right, in model and in PZ world space?
    lh, rh = translation(f0, idx["Bip01_L_Hand"]), translation(f0, idx["Bip01_R_Hand"])
    lc, rc = translation(f0, idx["Bip01_L_Clavicle"]), translation(f0, idx["Bip01_R_Clavicle"])
    print("\n[sides] model x: L_Clavicle %.3f  R_Clavicle %.3f  (L_Hand %.3f, R_Hand %.3f)"
          % (lc[0], rc[0], lh[0], rh[0]))

    # 5. Timing.
    dts = [(b["monoNanos"] - a["monoNanos"]) / 1e6 for a, b in zip(frames, frames[1:])]
    print("\n[timing] frame interval ms: mean %.2f  median %.2f  p99 %.2f  max %.2f"
          % (statistics.mean(dts), statistics.median(dts), sorted(dts)[int(len(dts) * 0.99)], max(dts)))
    anim = [f["animDelta"] for f in frames]
    print("         animDelta: mean %.4f  min %.4f  max %.4f" % (statistics.mean(anim), min(anim), max(anim)))

    # 6. Movement.
    path = sum(dist(a["pos"], b["pos"]) for a, b in zip(frames, frames[1:]))
    moving = sum(1 for a, b in zip(frames, frames[1:]) if dist(a["pos"], b["pos"]) > 1e-4)
    print("\n[movement] path length %.2f tiles, moving in %d of %d frames, levels %s"
          % (path, moving, len(frames) - 1, sorted({round(f["pos"][2], 2) for f in frames})))


if __name__ == "__main__":
    main()
