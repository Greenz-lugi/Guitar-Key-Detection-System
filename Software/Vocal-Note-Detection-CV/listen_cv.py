"""
Same analyzer as listen.py, drawn over the webcam with OpenCV.
pip install numpy sounddevice opencv-python
python listen_cv.py     (q = quit, r = reset key)
"""
import time

import cv2
import numpy as np

import listen as L

FACE_CASCADE = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml')

CAMERA_INDEX = 0
COLORS = {'GUITAR': (60, 160, 255), 'VOICE': (255, 220, 80)}   # BGR
GREY = (180, 180, 180)
FONT = cv2.FONT_HERSHEY_DUPLEX


def put_text(frame, text, pos, scale=1.0, color=(255, 255, 255), thickness=2):
    x, y = pos
    cv2.putText(frame, text, (x + 2, y + 2), FONT, scale, (0, 0, 0), thickness + 2, cv2.LINE_AA)
    cv2.putText(frame, text, (x, y), FONT, scale, color, thickness, cv2.LINE_AA)


def text_center(frame, text, y, scale=1.0, color=(255, 255, 255), thickness=2):
    (tw, _), _ = cv2.getTextSize(text, FONT, scale, thickness)
    put_text(frame, text, ((frame.shape[1] - tw) // 2, y), scale, color, thickness)


def panel(frame, x1, y1, x2, y2, alpha=0.45):
    ov = frame.copy()
    cv2.rectangle(ov, (x1, y1), (x2, y2), (0, 0, 0), -1)
    cv2.addWeighted(ov, alpha, frame, 1 - alpha, 0, frame)


def draw(frame, r, key, hist, listening, vision_status):
    fh, fw = frame.shape[:2]
    src = r['src'] if r else None
    col = COLORS.get(src, GREY)

    if src == 'GUITAR':
        prompt = "Try Playing"
        prompt_color = COLORS['GUITAR']
    else:
        prompt = "Try Singing"
        prompt_color = COLORS['VOICE']

    # top badge
    panel(frame, 0, 0, fw, 50)
    put_text(frame, prompt, (20, 35), 0.9, prompt_color, 2)
    put_text(frame, f'KEY: {key}', (fw - 330, 35), 0.9, (255, 255, 255), 2)

    # vision banner
    panel(frame, 0, 52, 280, 95, alpha=0.5)
    vis_color = (0, 255, 120) if vision_status in ("Human", "Guitar") else (180, 180, 180)
    put_text(frame, f"Vision: {vision_status}", (15, 84), 0.7, vis_color, 2)

    # center readout (smaller, lower)
    if r and r['big']:
        cy = int(fh * 0.68)
        panel(frame, fw // 2 - 150, cy - 70, fw // 2 + 150, cy + 35)
        text_center(frame, r['big'], cy + 5, 2.0, col, 4)
        text_center(frame, r['small'], cy + 28, 0.6, (255, 255, 255), 1)

    # pitch-class histogram
    panel(frame, 0, fh - 110, fw, fh)
    m = max(hist.max(), 1e-6)
    bw = fw // 12
    for i in range(12):
        h = int(60 * hist[i] / m)
        cv2.rectangle(frame, (i * bw + 12, fh - 30 - h), (i * bw + bw - 12, fh - 30), col, -1)
        text_center_x = i * bw + bw // 2 - 12
        put_text(frame, L.NOTES[i], (text_center_x, fh - 8), 0.5, (255, 255, 255), 1)


def detect_vision(frame):
    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
    small = cv2.resize(gray, (0, 0), fx=0.5, fy=0.5)
    faces = FACE_CASCADE.detectMultiScale(small, scaleFactor=1.1, minNeighbors=5, minSize=(30, 30))
    has_face = len(faces) > 0

    blur = cv2.GaussianBlur(gray, (5, 5), 0)
    _, thresh = cv2.threshold(blur, 60, 255, cv2.THRESH_BINARY_INV)
    thresh = cv2.morphologyEx(thresh, cv2.MORPH_OPEN, np.ones((7, 7), np.uint8))
    contours, _ = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    for c in contours:
        area = cv2.contourArea(c)
        if area < 8000:
            continue
        x, y, w, h = cv2.boundingRect(c)
        long_side, short_side = max(w, h), min(w, h)
        if long_side / float(short_side) > 3.0 and area / float(w * h) > 0.35:
            return "Guitar"   # guitar wins even if a face is also in frame
    return "Human" if has_face else "Nothing"


def main():
    import sounddevice as sd
    state = {'buf': np.zeros(L.BUF, np.float32)}

    def cb(indata, frames, t, status):
        b = np.roll(state['buf'], -frames)
        b[-frames:] = indata[:, 0]
        state['buf'] = b

    cap = cv2.VideoCapture(CAMERA_INDEX)
    have_cam = cap.isOpened()
    if have_cam:
        cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1280)
        cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

    an = L.Analyzer()
    last = 0.0
    last_valid_r = None
    display_timer = 0.0

    with sd.InputStream(samplerate=L.SR, channels=1, callback=cb, blocksize=1024):
        while True:
            if have_cam:
                ok, frame = cap.read()
                if not ok:
                    break
                frame = cv2.flip(frame, 1)
                vision_status = detect_vision(frame)
            else:
                frame = np.zeros((720, 1280, 3), np.uint8)
                vision_status = "No camera detected"

            if time.time() - last >= L.STEP:
                last = time.time()
                r = an.update(state['buf'].astype(np.float64))

                # blahh
                if r is not None:
                    last_valid_r = r
                    display_timer = time.time()  # Reset timer when we get a fresh note
                elif time.time() - display_timer > 15.0:
                    last_valid_r = None          # Clear out after 15 seconds of silence


            draw(frame, last_valid_r, an.key_str(), an.keyhist, last_valid_r is not None, vision_status)
            cv2.imshow('Key Finder', frame)

            k = cv2.waitKey(1) & 0xFF
            if k == ord('q'):
                break
            if k == ord('r'):
                an.keyhist[:] = 0

    if have_cam:
        cap.release()
    cv2.destroyAllWindows()


if __name__ == '__main__':
    main()
