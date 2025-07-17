from flask import Flask, render_template, request, jsonify
from flask_socketio import SocketIO
import csv
from datetime import datetime
from threading import Thread, Event, Lock
import time
from multiprocessing import Value

app = Flask(__name__)
socketio = SocketIO(app, async_mode='threading')

# Thread-safe variables
total_bytes_received = Value('i', 0)  # Integer shared between threads
rx_speed_kbps = Value('d', 0.0)       # Double shared between threads
last_measurement_time = Value('d', time.time())
is_recording = False
csv_filename = "sine_wave_data.csv"
data_lock = Lock()  # Lock for thread-safe CSV writing

def handle_esp32_ws():
    import websockets
    import asyncio

    async def connect_to_esp32():
        ESP32_IP = "192.168.0.232"  # Update to your ESP32's IP
        uri = f"ws://{ESP32_IP}:81"
        
        while True:
            try:
                async with websockets.connect(uri) as ws:
                    while True:
                        data = await ws.recv()
                        with total_bytes_received.get_lock():
                            total_bytes_received.value += len(data)
                        
                        try:
                            import json
                            data_dict = json.loads(data)
                            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")
                            
                            with rx_speed_kbps.get_lock(), data_lock:
                                socketio.emit('sine_update', {
                                    'timestamp': timestamp,
                                    'value': data_dict['value'],
                                    'tx_speed': data_dict['tx_speed'],
                                    'rx_speed': rx_speed_kbps.value,
                                    'is_recording': is_recording
                                })
                                
                                if is_recording:
                                    with open(csv_filename, 'a', newline='') as f:
                                        writer = csv.writer(f)
                                        writer.writerow([
                                            timestamp, 
                                            data_dict['value'], 
                                            data_dict['tx_speed'], 
                                            rx_speed_kbps.value
                                        ])
                            
                        except Exception as e:
                            print(f"Data error: {e}")
                            
            except Exception as e:
                print(f"Connection error: {e}")
                await asyncio.sleep(5)

    asyncio.run(connect_to_esp32())

def calculate_speed():
    global last_measurement_time
    while True:
        current_time = time.time()
        with total_bytes_received.get_lock(), rx_speed_kbps.get_lock():
            if current_time - last_measurement_time.value >= 1.0:
                rx_speed_kbps.value = (total_bytes_received.value * 8) / 1000
                total_bytes_received.value = 0
                last_measurement_time.value = current_time
        time.sleep(0.1)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/start_recording', methods=['POST'])
def start_recording():
    global is_recording
    is_recording = True
    with data_lock:
        with open(csv_filename, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(['Timestamp', 'Value', 'TX Speed (kbps)', 'RX Speed (kbps)'])
    return jsonify({'status': 'Recording started'})

@app.route('/stop_recording', methods=['POST'])
def stop_recording():
    global is_recording
    is_recording = False
    return jsonify({'status': 'Recording stopped'})

if __name__ == '__main__':
    Thread(target=handle_esp32_ws, daemon=True).start()
    Thread(target=calculate_speed, daemon=True).start()
    socketio.run(app, debug=True, use_reloader=False)