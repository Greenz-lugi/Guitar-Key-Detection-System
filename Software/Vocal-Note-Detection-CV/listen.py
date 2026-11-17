"""
Live mic analyzer: guitar vs voice, solfege for voice, chords for guitar, key estimate.

pip install numpy sounddevice
python listen.py
"""
import sys
from collections import deque

import numpy as np

SR = 44100
BUF = 16384          # ~0.37 s window for chroma (guitar chords)
YIN_N = 4096         # ~0.09 s window for pitch tracking
STEP = 0.08         # seconds between updates
HIST = 5             # frames used for guitar/voice decision (~0.4 s)

GATE = 0.003         # RMS below this = silence (raise if noisy room)
CLARITY_POLY = 0.80  # YIN clarity below this => polyphonic (chord)
STD_CENTS = 6    # pitch wobble below this => guitar-like
DECAY_DB_S = -2.0    # amplitude slope below this => plucked/decaying
CHORD_MIN = 0.7   # min cosine score to accept a chord

NOTES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
SOLFEGE = ['Do', 'Di', 'Re', 'Ri', 'Mi', 'Fa', 'Fi', 'Sol', 'Si', 'La', 'Li', 'Ti']
MOVABLE_DO = True    # True: Do = tonic of detected key. False: Do = C

CHORDS = {'': [0, 4, 7], 'm': [0, 3, 7], '7': [0, 4, 7, 10]}   # add more here
TEMPLATES = []
for name, iv in CHORDS.items():
    for root in range(12):
        v = np.zeros(12)
        v[[(root + i) % 12 for i in iv]] = 1
        TEMPLATES.append((NOTES[root] + name, root, v / np.linalg.norm(v)))

KS_MAJ = np.array([6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88])
KS_MIN = np.array([6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17])


def yin(x, sr, fmin=70, fmax=1000, thr=0.15):
    """Returns (f0 or None, clarity 0..1)."""
    n = len(x)
    tau_max = int(sr / fmin) + 1
    tau_min = max(2, int(sr / fmax))
    W = n - tau_max
    N = 1 << (n + W).bit_length()
    corr = np.fft.irfft(np.fft.rfft(x, N) * np.conj(np.fft.rfft(x[:W], N)), N)[:tau_max]
    cs = np.concatenate(([0.0], np.cumsum(x ** 2)))
    d = cs[W] + (cs[W:W + tau_max] - cs[:tau_max]) - 2 * corr
    d[0] = 0
    cm = np.ones(tau_max)
    cm[1:] = d[1:] * np.arange(1, tau_max) / np.maximum(np.cumsum(d[1:]), 1e-12)
    t = None
    for i in range(tau_min, tau_max - 1):
        if cm[i] < thr:
            while i + 1 < tau_max - 1 and cm[i + 1] < cm[i]:
                i += 1
            t = i
            break
    if t is None:
        t = tau_min + int(np.argmin(cm[tau_min:tau_max - 1]))
        return None, float(1 - cm[t])
    a, b, c = cm[t - 1], cm[t], cm[t + 1]
    den = a - 2 * b + c
    off = 0.5 * (a - c) / den if den else 0
    return sr / (t + off), float(1 - b)


def chroma(x, sr):
    w = x * np.hanning(len(x))
    S = np.abs(np.fft.rfft(w, 2 * len(x)))
    f = np.fft.rfftfreq(2 * len(x), 1 / sr)
    m = (f >= 80) & (f <= 2000)
    S, f = S[m], f[m]
    midi = 69 + 12 * np.log2(f / 440)
    near = np.abs(midi - np.round(midi)) < 0.35
    pc = np.round(midi[near]).astype(int) % 12
    c = np.bincount(pc, weights=S[near] ** 0.5, minlength=12)
    return c / (np.linalg.norm(c) + 1e-12)


def match_chord(c):
    name, root, s = max(((n, r, float(c @ v)) for n, r, v in TEMPLATES), key=lambda t: t[2])
    return name, root, s


def estimate_key(h):
    if h.sum() < 1e-6:
        return None
    best = (-2, None)
    for tonic in range(12):
        for prof, mode in ((KS_MAJ, 'major'), (KS_MIN, 'minor')):
            r = np.corrcoef(h, np.roll(prof, tonic))[0, 1]
            if r > best[0]:
                best = (r, (tonic, mode))
    return best[1]


def hz_to_note(f):
    midi = 69 + 12 * np.log2(f / 440)
    n = int(round(midi))
    return n % 12, n // 12 - 1, (midi - n) * 100


class Analyzer:
    def __init__(self):
        self.hist = deque(maxlen=HIST)
        self.keyhist = np.zeros(12)

    def solfege(self, pc):
        if MOVABLE_DO and (k := estimate_key(self.keyhist)):
            tonic, mode = k
            if mode == 'minor':
                tonic = (tonic + 3) % 12   # la-based minor: Do = relative major
            return SOLFEGE[(pc - tonic) % 12]
        return SOLFEGE[pc]

    def update(self, buf):
        self.keyhist *= 0.995
        x = buf[-YIN_N:]
        rms = float(np.sqrt(np.mean(x ** 2)))
        if rms < GATE:
            self.hist.clear()
            return None
        f0, cl = yin(x, SR)
        self.hist.append((f0, cl, rms))

        if cl < CLARITY_POLY:                       # polyphonic -> guitar chord
            name, root, s = match_chord(chroma(buf, SR))
            if s < CHORD_MIN:
                return dict(src='?', text='...', big='?', small='')
            iv = next(i for n, i in CHORDS.items() if name == NOTES[root] + n)
            for i in iv:
                self.keyhist[(root + i) % 12] += 1
            return dict(src='GUITAR', text=f'chord {name}  ({s:.2f})', big=name, small=f'chord  {s:.2f}')

        pitched = [h for h in self.hist if h[0]]
        if len(pitched) < 2:
            return dict(src='...', text='', big='...', small='')
        f = np.array([h[0] for h in pitched])
        cents = 1200 * np.log2(f / np.median(f))
        db = 20 * np.log10(np.array([h[2] for h in self.hist]) + 1e-9)
        slope = np.polyfit(np.arange(len(db)) * STEP, db, 1)[0] if len(db) > 2 else 0
        guitar = cents.std() < STD_CENTS and slope < DECAY_DB_S

        pc, octv, dev = hz_to_note(float(np.median(f)))
        self.keyhist[pc] += 1
        if guitar:
            return dict(src='GUITAR', text=f'note {NOTES[pc]}{octv}', big=NOTES[pc], small=f'note {NOTES[pc]}{octv}')
        sol = self.solfege(pc)
        return dict(src='VOICE', text=f'{NOTES[pc]}{octv} {dev:+.0f}c  {sol}', big=sol, small=f'{NOTES[pc]}{octv}  {dev:+.0f} cents')

    def key_str(self):
        k = estimate_key(self.keyhist)
        return f'{NOTES[k[0]]} {k[1]}' if k else '--'


def main():
    import sounddevice as sd
    state = {'buf': np.zeros(BUF, np.float32)}

    def cb(indata, frames, t, status):
        b = np.roll(state['buf'], -frames)
        b[-frames:] = indata[:, 0]
        state['buf'] = b

    an = Analyzer()
    print('listening... ctrl+c to quit')
    with sd.InputStream(samplerate=SR, channels=1, callback=cb, blocksize=1024):
        try:
            while True:
                sd.sleep(int(STEP * 1000))
                r = an.update(state['buf'].astype(np.float64))
                line = 'listening...' if r is None else f"[{r['src']:6}] {r['text']}"
                sys.stdout.write(f"\r{line:<40} | key: {an.key_str():<12}")
                sys.stdout.flush()
        except KeyboardInterrupt:
            print()


if __name__ == '__main__':
    main()
