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
3. Cambia las constantes `SSID` y `PASSWORD` con los datos de tu red WiFi.
4. Ajusta `ALTITUDE_METERS` con la altitud local si deseas la presión corregida al nivel del mar.
5. Selecciona la placa "ESP32 Dev Module" y el puerto correspondiente y sube el sketch.
6. Abre el Monitor Serial a 115200 bps para ver la IP asignada y logs.
7. Abre la IP en un navegador para ver el dashboard.

Notas:
- El sketch ahora usa BMP180/BMP085 para presión y DHT11 para temperatura y humedad.
- DHT11 es menos preciso y más lento que DHT22; si dispones de DHT22 considéralo como mejora.
- Si tu BMP utiliza otra dirección I2C, revisa las conexiones y la biblioteca.
