# DoodleBot v2

### Libraries:
- [WebSerial](https://github.com/ayushsharma82/WebSerial)~2.1.1 (may need [ESP Async TCP](https://github.com/ESP32Async/ESPAsyncTCP)~2.0.0 and [ESP Async WebServer](https://github.com/ESP32Async/ESPAsyncWebServer)~3.7.7)
- [ArduinoEigen](https://github.com/hideakitai/ArduinoEigen)~0.3.2
- [AccelStepper](http://www.airspayce.com/mikem/arduino/AccelStepper/)~1.64


### Gcode generation
1. image -> svg: https://picsvg.com/
2. svg -> gcode: https://sameer.github.io/svg2gcode
3. gcode viewer: https://ncviewer.com/

Add an M3 line before the first G1 line, and add an M5 line at the end of the file.
