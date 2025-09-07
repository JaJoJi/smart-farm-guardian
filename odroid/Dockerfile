FROM python:3.11-slim

# ติดตั้ง dependencies สำหรับ OpenCV ARM
RUN apt-get update && apt-get install -y \
    libgl1 \
    libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

COPY camera_stream.py .

EXPOSE 5000

CMD ["python", "camera_stream.py"]
