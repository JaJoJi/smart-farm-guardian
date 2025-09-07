from flask import Flask, Response, render_template_string, make_response
import cv2

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
    resp = Response(generate_frames(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')
    resp.headers['ngrok-skip-browser-warning'] = '1'
    return resp

@app.route('/')
def index():
    resp = make_response(render_template_string('''
        <html>
            <head><title>Odroid Camera</title></head>
            <body>
                <h1>Live Camera</h1>
                <img src="/video" width="640" height="480">
            </body>
        </html>
    '''))
    resp.headers['ngrok-skip-browser-warning'] = '1'
    return resp

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000, debug=False)
