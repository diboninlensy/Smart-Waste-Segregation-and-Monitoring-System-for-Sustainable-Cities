from flask import Flask, Response
from ultralytics import YOLO
import cv2

app = Flask(__name__)

# Load models
waste_model = YOLO("best.pt")
ewaste_model = YOLO("ewaste.pt")

# IP Camera URL (CHANGE THIS)
IP_CAMERA_URL = "http://10.65.220.16:8080/video"
# OR RTSP:
# IP_CAMERA_URL = "rtsp://username:password@192.168.1.100:554/stream"

# Class groups
non_bio_classes = ["GLASS", "METAL", "PLASTIC"]

def classify_frame(frame):

    # 1. Check E-Waste first
    ewaste_results = ewaste_model(frame, conf=0.5)

    for r in ewaste_results:
        if len(r.boxes) > 0:
            return "E-WASTE", r

    # 2. Normal waste
    waste_results = waste_model(frame, conf=0.5)

    for r in waste_results:
        for box in r.boxes:
            cls_id = int(box.cls[0])
            class_name = waste_model.names[cls_id]

            if class_name in non_bio_classes:
                return "NON-BIODEGRADABLE", r
            else:
                return "BIODEGRADABLE", r

    return "NO DETECTION", None


def generate_frames():
    cap = cv2.VideoCapture(IP_CAMERA_URL)

    while True:
        success, frame = cap.read()
        if not success:
            break

        label, result = classify_frame(frame)

        # Draw bounding boxes
        if result:
            frame = result[0].plot()

        # Put label
        cv2.putText(frame, label, (20, 50),
                    cv2.FONT_HERSHEY_SIMPLEX, 1,
                    (0, 255, 0), 2)

        # Encode frame
        _, buffer = cv2.imencode('.jpg', frame)
        frame = buffer.tobytes()

        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')


@app.route('/')
def index():
    return """
    <html>
    <head>
        <title>Waste Detection System</title>
    </head>
    <body>
        <h2>Live Waste Detection</h2>
        <img src="/video_feed" width="800">
    </body>
    </html>
    """


@app.route('/video_feed')
def video_feed():
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=True)