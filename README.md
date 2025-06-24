## Automated watering with ESP32
A backend component of IoT project for automated plant irrigation.

There is also an Android client available, you can check it out [here](https://github.com/baddabudda/automated-watering-client/tree/master).

### Description
The ESP32 microcontroller hosts a web server.
It can be configured in `wificonfig.h`.
Configuration parameters persist during power outages using ESP32's Preferences storage.

### Features
- Periodic soil moisture checks
- Automatic irrigation based on soil moisture
- HTTP API
- Configuration through API

### Endpoints
This project exposes API that can be used for creating your own client.

Base URL will look like
> ```
> http://<esp32-ip>/
> ```

Response format is `application/json`.

Available endpoints:
| Endpoint | Method | Description | Parameters |
| -------- | ------ | ----------- | ---------- |
| `/` | `GET` | Get system status (soil moisture, water level) | `None` |
| `/water` | `GET` | Trigger manual irrigation | `None` |
| `/config` | `GET` | Get currernt system configuration | `None` |
| `/change` | `GET` | Change system configuration | `json { "waterMax": float, "soilMin": int, "soilMax": int, "moistThreshold": int, "checkInterval": int, "wateringDuration": int}` |
| `/test` | `GET` | Check connection | `None` |

### Configuration info
Since system configuration can be configured via `/change` endpoint, this section provides a brief description of system parameters.
- `waterMax`: describes water level when the tank is full (height in cm)
- `soilMin`: describes the baseline for moisturized soil (`0 <= soilMin <= 4095`)
- `soilMax`: describes the baseline for dry soil (`0 <= soilMax <= 4095`)
- `moistThreshold`: describes irrigation trigger point; if the value received from the YL-69 is greater than threshold, perform irrigation (`0 <= moistThreshold <= 4095`)
- `checkInterval`: describes polling frequency, i.e. how often system checks soil moisturization (time in minutes)
- `wateringDuration`: describes how long the soil will be irrigated (time in seconds)

### Components used
- ESP32
- YL-69 moisture sensor
- HC-SR04 ultrasonic sensor
- Water pump
- LM2596S-ADJ voltage regulator
- 24V relay module
