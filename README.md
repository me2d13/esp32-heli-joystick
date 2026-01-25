# ESP32 Heli Joystick

ESP32-S3 based USB HID helicopter joystick controller with WiFi and OTA support.

## Features

- **USB HID Joystick** - Native USB joystick with 3 axes and 32 buttons
  - Device name: `esp-heli-v1`
  - 3 axes: Cyclic X, Cyclic Y, Collective
  - 32 programmable buttons
- **Cyclic Axis via External Sensor Board** - Receives position data from AS5600 magnetic encoders via UART
- **RGB LED Status Indicator** - WS2812 RGB LED on GPIO 48
- **Optional WiFi Connectivity** - Connect to WiFi for web interface and OTA updates
- **OTA Updates** - Upload new firmware over WiFi without USB cable
- **Web Interface** - Real-time dashboard with WebSocket-powered live updates

## Hardware

- **Board**: ESP32-S3 (YD-ESP32-S3 or compatible)
- **USB**: Native USB HID support (no external USB-to-Serial chip needed)
- **RGB LED**: WS2812 on GPIO 48
- **Flash**: 16MB
- **PSRAM**: Disabled (not required)

## USB HID Joystick

The ESP32-S3 appears as a USB HID joystick device when connected to a computer.

### Specifications

- **Device Name**: esp-heli-v1
- **Manufacturer**: ESP32
- **Axes**: 3 (16-bit precision, range: 0 to 10000)
  - **Axis 0**: Cyclic X (left/right)
  - **Axis 1**: Cyclic Y (forward/back)
  - **Axis 2**: Collective (up/down)
- **Buttons**: 32 (0-31)

### Testing the Joystick

**Windows:**
1. Press `Win+R`, type `joy.cpl`, press Enter
2. The device "esp-heli-v1" should appear in the list
3. Select it and click "Properties" to see live axis movements and button states

**Linux:**
```bash
jstest /dev/input/js0
```

**macOS:**
Use a joystick testing application from the App Store

### Important: USB CDC Configuration

⚠️ **USB CDC (Serial) is disabled** to allow HID joystick functionality. This means:
- Serial debug output is **not available** via USB
- You cannot use the USB port for serial monitoring
- OTA updates via WiFi are the primary method for uploading new firmware

If you need serial debugging, you can temporarily enable USB CDC by changing in `platformio.ini`:
```ini
-DARDUINO_USB_CDC_ON_BOOT=0  →  -DARDUINO_USB_CDC_ON_BOOT=1
```
**Note**: Enabling CDC will disable the HID joystick functionality.

## Cyclic Axis (Hall Sensors)

The cyclic X and Y axes are read from an external ESP32 board equipped with two AS5600 magnetic rotary encoders. This sensor board continuously transmits position data over a UART serial connection.

### Sensor Board

The sensor data comes from a separate ESP32-S3 running the [esp32-as5600](https://github.com/me2d13/esp32-as5600) firmware. This board:
- Reads two AS5600 magnetic encoders (one for each axis)
- Transmits 12-bit angle data (0-4095) via UART at 115200 baud
- Uses a binary protocol with checksums for reliable data transfer

### Wiring

Connect the sensor board to this joystick controller:

| Sensor Board | Joystick Controller |
|--------------|---------------------|
| TX (GPIO 43) | RX (GPIO 13)        |
| GND          | GND                 |

### Binary Protocol

The sensor board sends 7-byte packets:

```
Byte 0:     Start Marker (0xAA)
Byte 1-2:   Sensor 1 Angle (uint16_t, little-endian, 0-4095) → Cyclic X
Byte 3-4:   Sensor 2 Angle (uint16_t, little-endian, 0-4095) → Cyclic Y
Byte 5:     Checksum (XOR of bytes 1-4)
Byte 6:     End Marker (0x55)
```

### Calibration

The physical stick movement typically doesn't cover the full 0-4095 sensor range. Calibration constants in `include/config.h` define the actual range for each axis:

```cpp
// Cyclic X axis (left/right) calibration
#define CYCLIC_X_SENSOR_MIN   500    // Sensor value at full left
#define CYCLIC_X_SENSOR_MAX   3500   // Sensor value at full right
#define CYCLIC_X_INVERT       false  // Invert axis direction

// Cyclic Y axis (forward/back) calibration
#define CYCLIC_Y_SENSOR_MIN   500    // Sensor value at full back
#define CYCLIC_Y_SENSOR_MAX   3500   // Sensor value at full forward
#define CYCLIC_Y_INVERT       false  // Invert axis direction
```

**To calibrate:**
1. Move the cyclic stick to each extreme position
2. Note the raw sensor values (via web interface or debug output)
3. Update the MIN/MAX values in `config.h`
4. Set `INVERT` to `true` if the axis moves opposite to expected

The sensor values are mapped linearly from the calibration range to the joystick axis range (0 to 10000).

## Collective Axis (Analog Input)

The collective axis is read from an analog voltage input (0-3.3V) on GPIO 11. This is a temporary solution until the I2C collective sensor is wired.

### Hardware

- **Input Pin**: GPIO 11 (PIN_COL_I2C_D)
- **Input Type**: Analog voltage (0-3.3V)
- **ADC Resolution**: 12-bit (0-4095)

### Calibration

The collective axis supports calibration with overflow handling for cases where the sensor range wraps around the ADC boundary (0/4095). Configuration is in `include/config.h`:

```cpp
// Collective axis (up/down) calibration
// Note: This axis wraps around at the ADC boundary (0/4095)
// Physical range: 1370 (down) → 4095 → 0 → 1500 (up)
#define COLLECTIVE_SENSOR_MIN 1370   // Sensor value at full down position
#define COLLECTIVE_SENSOR_MAX 1500   // Sensor value at full up position
#define COLLECTIVE_INVERT     true   // Set to true to invert axis direction
```

**Overflow Handling:**

The collective axis implementation automatically detects and handles wrap-around at the ADC boundary. When the physical range crosses from 4095 to 0 (or vice versa), the values are normalized into a continuous range before mapping to the joystick axis.

**To calibrate:**
1. Move the collective to full down position and note the raw value (visible on web interface)
2. Move to full up position and note the raw value
3. Update `COLLECTIVE_SENSOR_MIN` and `COLLECTIVE_SENSOR_MAX` in `config.h`
4. Set `COLLECTIVE_INVERT` to `true` if the axis moves opposite to expected
5. Values outside the calibration range are automatically clamped to the nearest edge

## Configuration

### WiFi Setup

WiFi credentials can be configured in two ways:

#### Method 1: Using secrets.h (Recommended)

1. Edit `include/secrets.h` with your WiFi credentials:
   ```cpp
   #define WIFI_SSID_SECRET           "YourWiFiNetwork"
   #define WIFI_PASSWORD_SECRET       "YourPassword"
   ```

2. This file is gitignored and won't be committed to your repository

#### Method 2: Direct configuration (Not recommended for public repos)

Edit `include/config.h` and modify the default values:
```cpp
#define WIFI_SSID           "YourWiFiNetwork"
#define WIFI_PASSWORD       "YourPassword"
```

**Note**: If `WIFI_SSID` is left empty, WiFi functionality is completely disabled and the code runs without any WiFi overhead.

### Other Settings

Edit `include/config.h` to customize:
- `RGB_LED_BRIGHTNESS` - LED brightness (0-255)
- `WEB_SERVER_PORT` - Web server port (default: 80)
- `WIFI_CONNECT_TIMEOUT` - WiFi connection timeout in milliseconds

## Building and Uploading

### Prerequisites

- [PlatformIO](https://platformio.org/)
- USB cable for initial upload

### Build

```bash
pio run
```

### Upload via USB (Default)

```bash
pio run --target upload
```

This uses the default environment and uploads via COM7.

### Upload via OTA (Wireless)

After WiFi is configured and the ESP32 is connected to your network:

```bash
pio run -e ota --target upload
```

**Requirements:**
- ESP32 must be connected to WiFi
- Default OTA password: `admin`
- Default IP address in config: `192.168.1.31`

**To update the IP address:**
Edit `platformio.ini` and change the `upload_port` in the `[env:ota]` section to match your ESP32's IP address.

## LED Status Indicators

The RGB LED shows the current system status:

- **Purple** - System starting up
- **Yellow (blinking)** - Connecting to WiFi
- **Green (solid)** - WiFi connected successfully
- **Red (solid)** - WiFi disabled or connection failed

## Usage

### First Boot

1. Upload the firmware via USB
2. If WiFi is configured, the ESP32 will:
   - Connect to your WiFi network
   - Print the IP address to serial (if you have the UART port connected)
   - Start the web server
   - Enable OTA updates

3. The RGB LED will show green when successfully connected to WiFi

### Web Interface

Once connected to WiFi, open your browser and navigate to:
- `http://<IP_ADDRESS>/` - Main dashboard
- `http://esp32-heli-joy.local/` - mDNS address (may not work on all networks)

The web dashboard provides **real-time monitoring** of the joystick state via WebSocket:

- **Axes Display** - Visual bars showing Cyclic X, Cyclic Y, and Collective positions (0 to 10000, center at 5000)
- **Raw Sensor Values** - Current raw readings from AS5600 sensors (0-4095) and collective analog input (0-4095)
- **Button Grid** - All 32 buttons displayed with pressed/released state
- **Connection Status** - WebSocket connection state and cyclic data validity
- **Update Rate** - Current refresh rate (typically 20 Hz)

The dashboard automatically reconnects if the WebSocket connection is lost.

### OTA Updates

The default OTA password is `admin`. You can change this in `src/web_server.cpp`:
```cpp
ArduinoOTA.setPassword("your_secure_password");
```

## Project Structure

```
esp32-heli-joystick/
├── include/
│   ├── config.h              # Main configuration file
│   ├── secrets.h             # WiFi credentials (gitignored)
│   ├── secrets.h.template    # Template for secrets.h
│   ├── buttons.h             # Button handling interface
│   ├── collective.h          # Collective axis interface
│   ├── cyclic_serial.h       # Cyclic sensor serial receiver interface
│   ├── joystick.h            # USB HID joystick interface
│   ├── status_led.h          # RGB LED status indicator interface
│   └── web_server.h          # Web server interface
├── src/
│   ├── main.cpp              # Main application code
│   ├── buttons.cpp           # Button scanning and handling
│   ├── collective.cpp        # Collective axis analog input with overflow handling
│   ├── cyclic_serial.cpp     # Cyclic sensor data receiver (AS5600 protocol)
│   ├── joystick.cpp          # USB HID joystick implementation
│   ├── status_led.cpp        # RGB LED status indicator
│   └── web_server.cpp        # Web server and WiFi implementation
├── platformio.ini            # PlatformIO configuration
└── README.md                 # This file
```

## Dependencies

- **Adafruit NeoPixel** @ ^1.12.0 - RGB LED control
- **robtillaart/AS5600** @ ^0.6.1 - Magnetic encoder sensor library
- **schnoog/Joystick_ESP32S2** @ ^0.9.4 - USB HID joystick support for ESP32-S3
- **links2004/WebSockets** @ ^2.4.1 - WebSocket server for real-time dashboard
- **bblanchon/ArduinoJson** @ ^6.21.3 - JSON serialization for WebSocket data
- **WiFi** (built-in) - WiFi connectivity
- **WebServer** (built-in) - HTTP web server
- **ESPmDNS** (built-in) - mDNS responder
- **ArduinoOTA** (built-in) - Over-the-air updates

## Troubleshooting

### WiFi won't connect

- Check your WiFi credentials in `secrets.h`
- Ensure your WiFi network is 2.4GHz (ESP32 doesn't support 5GHz)
- Check the WiFi signal strength

### OTA upload fails

- Verify the ESP32 is connected to WiFi
- Check the IP address is correct
- Ensure the OTA password matches
- Try power cycling the ESP32

### RGB LED doesn't work

- Verify GPIO 48 is correct for your board
- Check the LED brightness setting in config.h
- Some boards may have different RGB LED configurations

### Cyclic axes not responding

- Check the wiring between sensor board and joystick controller (TX→RX, GND→GND)
- Verify the sensor board is powered and running
- Ensure baud rate matches (115200 by default)
- Check that the AS5600 sensors are detecting the magnets on the sensor board

### Cyclic axes moving in wrong direction

- Set `CYCLIC_X_INVERT` or `CYCLIC_Y_INVERT` to `true` in `config.h`

### Cyclic axes not reaching full range

- Calibrate the sensor range by updating `CYCLIC_X_SENSOR_MIN/MAX` and `CYCLIC_Y_SENSOR_MIN/MAX` in `config.h`

## License

MIT License

## Author

Created for ESP32-S3 helicopter joystick project
