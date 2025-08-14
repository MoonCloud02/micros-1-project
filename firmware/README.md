Firmware - Estación Meteorológica (ESP32)

Archivos:
- esp32_weather_station.ino : Sketch principal

Librerías necesarias (instalar desde Library Manager de Arduino IDE):
- Adafruit BMP085 Unified (para BMP180/BMP085) o "Adafruit BMP085 Unified" disponible en Library Manager
- DHT sensor library by Adafruit (o similar) para DHT11

Conexiones para este sketch (BMP180 + DHT11):
- BMP180 (I2C): SDA->GPIO21, SCL->GPIO22, VCC->3.3V, GND->GND
- DHT11: VCC->3.3V (o 5V si tu módulo lo requiere), GND->GND, DATA->GPIO4 (puedes cambiar `DHTPIN` en el sketch)

Instrucciones de uso:
1. Conecta los sensores al ESP32:
	- BMP180 (I2C): SDA->GPIO21, SCL->GPIO22, VCC->3.3V, GND->GND.
	- DHT11: VCC->3.3V (o 5V si tu módulo lo requiere), GND->GND, DATA->GPIO4 (o el pin que configures en el sketch).
2. Abre `esp32_weather_station.ino` en Arduino IDE.
3. Opcional: si quieres, cambia `AP_SSID` en el sketch (por defecto `"ESP32_WeatherStation"`). Las credenciales `SSID`/`PASSWORD` para red externa ya no son necesarias en la versión actual, porque el dispositivo iniciará un punto de acceso local.
4. Ajusta `ALTITUDE_METERS` con la altitud local si deseas la presión corregida al nivel del mar.
5. Selecciona la placa "ESP32 Dev Module" y el puerto correspondiente y sube el sketch.
6. Abre el Monitor Serial a 115200 bps; verás mensajes indicando que el AP fue iniciado y la IP del AP (normalmente `192.168.4.1`).
7. Conéctate desde tu PC o móvil a la red WiFi `ESP32_WeatherStation` (o el `AP_SSID` que hayas configurado) y abre `http://192.168.4.1/` para ver el dashboard.

Notas:
- El sketch usa BMP180/BMP085 para presión y DHT11 para temperatura y humedad.
- DHT11 es menos preciso y más lento que DHT22; si se dispone de DHT22 es una mejora.
- Si el BMP utiliza otra dirección I2C, revisar las conexiones y la biblioteca.
