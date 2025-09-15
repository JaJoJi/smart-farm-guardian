from flask import Flask, Response, render_template_string, make_response, request
import cv2
import serial
import time

arduino = serial.Serial('/dev/ttyACM0', 9600, timeout=1)  # ปรับพอร์ตตาม Odroid
time.sleep(2)  # รอ Arduino reset

app = Flask(__name__)
camera = cv2.VideoCapture(0)

def generate_frames():
    while True:
        success, frame = camera.read()
        if not success:
            break
        ret, buffer = cv2.imencode('.jpg', frame)
        frame = buffer.tobytes()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')

@app.route('/video')
def video():
    return Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/')
def index():
    html = '''
    <html>
        <head>
            <title>Odroid Camera Control</title>
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <style>
                body { text-align:center; font-family:sans-serif; }
                .btn { 
                    width:80px; height:80px; font-size:24px;
                    margin:10px; border-radius:15px; 
                    background:#007BFF; color:white; border:none;
                    user-select: none;
                }
                .btn:active { background:#0056b3; }
                .center-btn { background:#28a745; }
            </style>
        </head>
        <body>
            <h1>Live Camera + Servo Control</h1>
            <img src="/video" width="640" height="480"><br><br>

            <div>
                <button class="btn" data-cmd="U">⬆</button><br>
                <button class="btn" data-cmd="L">⬅</button>
                <button class="btn" data-cmd="R">➡</button><br>
                <button class="btn" data-cmd="D">⬇</button><br>
                <button class="btn center-btn" data-cmd="C">Center</button>
            </div>

            <script>
                var sendInterval;

                function startSend(cmd){
                    sendInterval = setInterval(() => {
                        fetch("/control?cmd=" + cmd);
                    }, 100);
                }

                function stopSend(){
                    clearInterval(sendInterval);
                }

                function sendCenter(){
                    fetch("/control?cmd=C");
                }

                // รองรับ Mouse + Touch
                document.querySelectorAll('.btn').forEach(btn => {
                    let cmd = btn.dataset.cmd;
                    if(cmd === "C"){
                        btn.addEventListener('click', sendCenter);
                    } else {
                        // Mouse events
                        btn.addEventListener('mousedown', () => startSend(cmd));
                        btn.addEventListener('mouseup', stopSend);
                        btn.addEventListener('mouseleave', stopSend);

                        // Touch events มือถือ
                        btn.addEventListener('touchstart', (e) => {
                            e.preventDefault();
                            startSend(cmd);
                        });
                        btn.addEventListener('touchend', stopSend);
                        btn.addEventListener('touchcancel', stopSend);
                    }
                });
            </script>
        </body>
    </html>
    '''
    return render_template_string(html)

@app.route('/control')
def control():
    cmd = request.args.get("cmd", "")
    if cmd in ["U","D","L","R","C"]:
        arduino.write(cmd.encode())
    return "OK"

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False)
