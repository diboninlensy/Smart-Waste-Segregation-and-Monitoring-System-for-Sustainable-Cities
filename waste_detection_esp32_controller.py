from flask import Flask, Response
from ultralytics import YOLO
import cv2
import requests
import time

app = Flask(__name__)

# =========================
# LOAD MODELS
# =========================
waste_model = YOLO("best.pt")
ewaste_model = YOLO("ewaste.pt")

# =========================
# CAMERA URL
# =========================
IP_CAMERA_URL = "http://192.0.0.4:8080/video"

# =========================
# ESP32 CONFIG
# =========================
ESP32_URL = "http://10.48.248.224/update"  # 🔴 CHANGE THIS
last_sent_label = None
last_sent_time = 0
SEND_DELAY = 1.0  # seconds (avoid rapid triggering)

# =========================
# CLASS GROUPS
# =========================
non_bio_classes = ["GLASS", "METAL", "PLASTIC"]


# =========================
# CLASSIFICATION FUNCTION
# =========================
def classify_frame(frame):

    # 1. Check E-Waste first
    ewaste_results = ewaste_model(frame, conf=0.5)

    for r in ewaste_results:
        if r.boxes is not None and len(r.boxes) > 0:
            return "E-WASTE", r

    # 2. Normal waste
    waste_results = waste_model(frame, conf=0.5)

    for r in waste_results:
        if r.boxes is not None:
            for box in r.boxes:
                cls_id = int(box.cls[0])
                class_name = waste_model.names[cls_id]

                if class_name in non_bio_classes:
                    return "NON-BIODEGRADABLE", r
                else:
                    return "BIODEGRADABLE", r

    return "NO DETECTION", None


# =========================
# SEND DATA TO ESP32
# =========================
def send_to_esp32(label):
    global last_sent_label, last_sent_time

    current_time = time.time()

    # Avoid duplicate + too frequent sending
    if label == "NO DETECTION":
        return

    if label != last_sent_label or (current_time - last_sent_time > SEND_DELAY):
        try:
            requests.get(ESP32_URL, params={"label": label}, timeout=0.5)
            print("✅ Sent to ESP32:", label)

            last_sent_label = label
            last_sent_time = current_time

        except Exception as e:
            print("❌ ESP32 Error:", e)


# =========================
# VIDEO STREAM
# =========================
def generate_frames():

    cap = cv2.VideoCapture(IP_CAMERA_URL)

    if not cap.isOpened():
        print("❌ Camera not accessible")
        return

    while True:
        success, frame = cap.read()
        if not success:
            print("⚠️ Frame not received")
            break

        # Resize for performance
        frame = cv2.resize(frame, (640, 480))

        # Detection
        label, result = classify_frame(frame)

        # Send to ESP32
        send_to_esp32(label)

        # Draw bounding boxes
        if result:
            frame = result[0].plot()

        # Show label
        cv2.putText(frame, label, (20, 50),
                    cv2.FONT_HERSHEY_SIMPLEX, 1,
                    (0, 255, 0), 2)

        # Encode frame
        _, buffer = cv2.imencode('.jpg', frame)
        frame_bytes = buffer.tobytes()

        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')


# =========================
# ROUTES
# =========================
@app.route('/')
def index():
    return """
    <html>
    <head>
        <title>Smart Waste Detection</title>
    </head>
    <body>
        <h2>Live Waste Detection + ESP32 Control</h2>
        <img src="/video_feed" width="800">
    </body>
    </html>
    """


@app.route('/video_feed')
def video_feed():
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')


# =========================
# RUN SERVER
# =========================
if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)