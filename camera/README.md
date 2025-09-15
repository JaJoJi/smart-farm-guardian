# Smart Farm Guardian Camera

This project allows you to stream live video from a webcam connected to an Odroid board. The video can be accessed securely from anywhere using ngrok.

## Features
- Live video streaming using Flask and OpenCV
- Accessible from anywhere via ngrok tunnel
- Simple web interface to view the camera feed
- Ready for further expansion (e.g., camera rotation, multiple cameras)

## Requirements
- Odroid board (or any Linux machine with a USB webcam)
- Webcam compatible with OpenCV
- Docker & Docker Compose installed
- Ngrok account for public URL access

## Setup Instructions
## 1. Set your ngrok token

Create a `.env` file in the project root and add your ngrok token:

```env NGROK_TOKEN=<your-ngrok-auth-token>```

## 2. Start the Docker containers

```docker compose up --build```

This will start two containers:

flask-camera (your webcam streaming app)

ngrok (creates a public URL to access the Flask app)

## 3. Check logs to find the public URL

Look for a line like: ``` Forwarding https://<random-subdomain>.ngrok.io -> http://flask-camera:5000 ```
This is your public URL to access the camera stream.

## 4. Open the camera feed in a browser
```https://<random-subdomain>.ngrok.io```

## 5. Stop the containers
To stop the containers, press CTRL+C in the terminal where Docker is running, or run:
```docker compose down```