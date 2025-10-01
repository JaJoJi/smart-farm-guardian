from flask import Flask, Response, render_template_string, request, jsonify
import cv2
import serial
import time
import base64

# --- Serial ---
arduino = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
time.sleep(2)

# --- Flask ---
app = Flask(__name__)
camera = cv2.VideoCapture(0)  # ใช้ resolution default ของ webcam

# --- Camera settings ---
brightness = 50
zoom_factor = 1

def generate_frames():
    global brightness, zoom_factor
    while True:
        success, frame = camera.read()
        if not success:
            break

        # ปรับ brightness
        frame = cv2.convertScaleAbs(frame, alpha=1, beta=brightness-50)

        # Digital zoom
        if zoom_factor > 1:
            h, w = frame.shape[:2]
            new_w, new_h = w // zoom_factor, h // zoom_factor
            x1, y1 = w//2 - new_w//2, h//2 - new_h//2
            x2, y2 = x1 + new_w, y1 + new_h
            frame = cv2.resize(frame[y1:y2, x1:x2], (w, h))

        ret, buffer = cv2.imencode('.jpg', frame)
        frame_bytes = buffer.tobytes()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')

@app.route('/video')
def video():
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

# --- Webpage ---
@app.route('/')
def index():
    html = '''
<html>
<head>
    <title>Odroid Camera + Servo</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { text-align:center; font-family:sans-serif; }
        .btn { width:80px; height:80px; font-size:24px; margin:10px; border-radius:15px; background:#007BFF; color:white; border:none; user-select:none; }
        .btn:active { background:#0056b3; }
        .center-btn { background:#28a745; }
        .action-btn { background:#FF9800; width:120px; }
        img { max-width:100%; height:auto; border:1px solid #ccc; margin-top:8px; }
    </style>
</head>
<body>
    <h1>Live Camera + Servo Control</h1>
    <img src="/video" id="live">

    <div>
        <button class="btn" data-cmd="U">⬆</button><br>
        <button class="btn" data-cmd="L">⬅</button>
        <button class="btn" data-cmd="R">➡</button><br>
        <button class="btn" data-cmd="D">⬇</button><br>
        <button class="btn center-btn" data-cmd="C">Center</button>
    </div>

    <hr>

    <h3>Brightness</h3>
    <input type="range" min="0" max="100" value="50" onchange="sendData('/set_brightness', this.value)">
    <h3>Zoom</h3>
    <input type="range" min="1" max="4" value="1" onchange="sendData('/set_zoom', this.value)">

    <hr>

    <button class="btn action-btn" onclick="captureImage()">📷 Capture</button>
    <button class="btn action-btn" onclick="sendData('/servo','NOD')">🙆 Nod</button>
    <button class="btn action-btn" onclick="sendData('/servo','SHAKE')">🙅 Shake</button>

    <h3>Latest Capture</h3>
    <img id="preview" style="display:none;">
    <br>
    <a id="downloadBtn" href="#" download="capture.png" style="display:none;">⬇️ Download</a>

<script>
function sendCommand(state, cmd){
    fetch("/control?cmd=" + cmd + "&state=" + state);
}

function sendData(url, value){
    fetch(url, {
        method:"POST",
        headers:{"Content-Type":"application/x-www-form-urlencoded"},
        body:"value=" + value
    });
}

function captureImage(){
    fetch('/capture')
    .then(res => res.json())
    .then(data => {
        if(data.data_url){
            const preview = document.getElementById('preview');
            const downloadBtn = document.getElementById('downloadBtn');
            preview.src = data.data_url;
            preview.style.display = "block";
            downloadBtn.href = data.data_url;
            downloadBtn.style.display = "inline-block";
        } else {
            alert("Capture failed");
        }
    });
}

// Servo buttons
document.querySelectorAll('.btn').forEach(btn => {
    let cmd = btn.dataset.cmd;
    if(!cmd) return;
    if(cmd === "C"){
        btn.addEventListener('click', () => sendCommand("press", cmd));
    } else {
        btn.addEventListener('mousedown', () => sendCommand("press", cmd));
        btn.addEventListener('mouseup',   () => sendCommand("release", cmd));
        btn.addEventListener('mouseleave',() => sendCommand("release", cmd));
        btn.addEventListener('touchstart', e => { e.preventDefault(); sendCommand("press", cmd); });
        btn.addEventListener('touchend',   () => sendCommand("release", cmd));
        btn.addEventListener('touchcancel',() => sendCommand("release", cmd));
    }
});
</script>
</body>
</html>
'''
    return render_template_string(html)

# --- APIs ---
@app.route('/set_brightness', methods=['POST'])
def set_brightness():
    global brightness
    brightness = int(request.form['value'])
    return "OK"

@app.route('/set_zoom', methods=['POST'])
def set_zoom():
    global zoom_factor
    zoom_factor = int(request.form['value'])
    return "OK"

@app.route('/capture')
def capture():
    success, frame = camera.read()
    if success:
        ret, buffer = cv2.imencode('.jpg', frame)
        jpg_as_text = base64.b64encode(buffer).decode('utf-8')
        data_url = f"data:image/jpeg;base64,{jpg_as_text}"
        return jsonify({'data_url': data_url})
    return jsonify({'error': 'Failed'})

@app.route('/servo', methods=['POST'])
def servo():
    action = request.form['value']
    if action == "NOD":
        arduino.write("NOD\n".encode())
    elif action == "SHAKE":
        arduino.write("SHAKE\n".encode())
    return "OK"

@app.route('/control')
def control():
    cmd = request.args.get("cmd", "")
    state = request.args.get("state", "")
    if cmd in ["U","D","L","R"]:
        if state=="press": arduino.write(f"{cmd}_ON\n".encode())
        else: arduino.write(f"{cmd}_OFF\n".encode())
    elif cmd=="C":
        arduino.write("CENTER\n".encode())
    return "OK"

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False)
