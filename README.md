# ESP32 Heli Joystick

ESP32-S3 based USB HID helicopter joystick controller with WiFi and OTA support.

## Features

- **USB HID Joystick** - Native USB joystick with 3 axes and 32 buttons
  - Device name: `esp-heli-v1`
  - 3 axes: Cyclic X, Cyclic Y, Collective
  - 32 programmable buttons
  - Smooth demo animation included
- **RGB LED Status Indicator** - WS2812 RGB LED on GPIO 48
- **Optional WiFi Connectivity** - Connect to WiFi for web interface and OTA updates
- **OTA Updates** - Upload new firmware over WiFi without USB cable
- **Web Interface** - Simple web dashboard (more features coming soon)

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
- **Axes**: 3 (8-bit signed, range: -127 to 127)
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
│   ├── joystick.h            # USB HID joystick interface
│   ├── status_led.h          # RGB LED status indicator interface
│   └── web_server.h          # Web server interface
├── src/
│   ├── main.cpp              # Main application code
│   ├── joystick.cpp          # USB HID joystick implementation
│   ├── status_led.cpp        # RGB LED status indicator
│   ├── web_server.cpp        # Web server and WiFi implementation
├── platformio.ini            # PlatformIO configuration
└── README.md                 # This file
```

## Dependencies

- **Adafruit NeoPixel** @ ^1.12.0 - RGB LED control
- **robtillaart/AS5600** @ ^0.6.1 - Magnetic encoder sensor library
- **schnoog/Joystick_ESP32S2** @ ^0.9.4 - USB HID joystick support for ESP32-S3
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

## License

MIT License

## Author

Created for ESP32-S3 helicopter joystick project
