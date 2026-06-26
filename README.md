# IoT Webserver

IoT System, written in Python,HTML,CSS,JS. 

ESP32-S3 microcontroller interacts with Flask web server to display temperature readings and enable control of lights.

See System Overview pdf for full details, including circuit diagram.

Contains:

Flask backend, communicates with ESP32 via HTTP/JSON.

Web dashboard frontend, uses AJAX for live updates.

## Requirements:

flask             #backend web server
flask-cors        #cross origin requests, to test frontend seperately from backend

## Virtual Environment Setup (Do Once)

### 1. clone the repository:
``` bash
git clone git@github.com:dan-vce/IoT-webserver.git 
```

### 2. Create venv (from inside project folder):
``` bash
python -m venv .venv
```

### 3. Activate venv

Windows Powershell:
``` .\.venv\Scripts\Activate.ps1 ```

Windows CMD:
``` .\.venv\Scripts\activate.bat ```

Linux/MacOS
``` source .venv/bin/activate ```

### 4. Install Packages
```
pip install -r requirements.txt
```

## How to run server (Do each time)

### 1. Activate venv

Windows Powershell: ``` .\.venv\Scripts\Activate.ps1 ```

Windows CMD: ``` .\.venv\Scripts\activate.bat ```

Linux/MacOS:  ``` source .venv/bin/activate ```

### 2. Activate Server

```
python app.py
```

### 3. Access webpage

View terminal output and ctrl+click on address to run

### When finished: Deactivate (close) venv

Stop web server, Ctrl+C

With env open:
``` deactivate ```

## API Routes

| Method | Route | Who calls it | Purpose |
|--------|-------|-------------|---------|
| `POST` | `/sensor` | ESP32 | Push sensor reading; receives pending LED command in response |
| `GET` | `/api/led/command` | ESP32 | Optional polling for pending command |
| `GET` | `/` | Browser | Serve dashboard page |
| `GET` | `/api/data` | Browser (AJAX) | Get latest sensor data |
| `POST` | `/api/led` | Browser | Send LED pattern command |

