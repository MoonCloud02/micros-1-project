# Estación Meteorológica con ESP32 usando BMP180 y DHT11

## Resumen

Se presenta el diseño, implementación y pruebas de una estación meteorológica basada en un ESP32, que mide presión atmosférica, temperatura y humedad mediante un sensor BMP180 (presión) y un DHT11 (temperatura y humedad). El sistema procesa las lecturas con un filtro de media móvil, calcula variables derivadas (punto de rocío y presión corregida al nivel del mar) y publica un dashboard web alojado en el propio ESP32 para monitoreo en tiempo real.

## 1. Introducción

La medición local de variables meteorológicas básicas (temperatura, humedad y presión) es útil en aplicaciones educativas, agrícolas y de monitoreo ambiental. Este proyecto busca construir una estación económica evitando módulos costosos, con un enfoque en reproducibilidad y trazabilidad del desarrollo.

## 2. Objetivos

- Diseñar y construir una estación meteorológica con ESP32 que mida presión, temperatura y humedad.
- Implementar un servidor web embebido que muestre un dashboard con las lecturas en tiempo real.
- Aplicar procesamiento básico de señales (media móvil) y cálculos meteorológicos estándar (punto de rocío y corrección barométrica).
- Documentar el proceso y entregar un informe en formato IEEE.

## 3. Metodología y modelo matemático

A continuación se describe, en orden cronológico y práctico, el procedimiento seguido durante el desarrollo del proyecto. Cada paso corresponde a una acción reproducible que puede seguirse para reinstanciar el prototipo.

1) Selección de sensores y decisiones iniciales

- Elegimos BMP180/BMP085 para presión (I2C) y DHT11 para temperatura/humedad por su bajo coste y disponibilidad. Se decidió priorizar reproducibilidad y simplicidad sobre alta precisión.

2) Cableado y montaje básico

- Conectar BMP180 vía I2C: VCC -> 3.3V, GND -> GND, SDA -> GPIO21, SCL -> GPIO22.
- Conectar DHT11: VCC -> 3.3V (o 5V si el módulo lo requiere), GND -> GND, DATA -> GPIO4.

3) Preparación del entorno (IDE y librerías)

- Usar Arduino IDE con soporte de ESP32 instalado.
- Instalar librerías: `Adafruit BMP085 Unified` (compatible con BMP180) y `DHT sensor library` (o equivalente) desde Library Manager.

4) Implementación básica del firmware

- Inicializar Serial (115200 bps) para logs de diagnóstico.
- Inicializar I2C (`Wire.begin(I2C_SDA, I2C_SCL)`), probar detección de BMP con `bmp.begin()` y arrancar `dht.begin()`.

5) Diseñar muestreo y buffering

- Decidimos un muestreo no bloqueante con `millis()` y `SAMPLE_INTERVAL = 2000` ms, compatible con la tasa de refresco del DHT11.
- Implementamos buffers circulares por variable (`tempBuf`, `humBuf`, `presBuf`) y variables `bufIndex` y `bufCount` para manejar la ventana móvil.
- Definimos `SMA_SIZE = 6` como ventana por defecto y añadimos lógica para incrementar `bufCount` durante el arranque hasta alcanzar la ventana completa.

6) Lectura y saneamiento de datos

- En cada tick de muestreo se leen `T` y `H` desde el DHT11 y `P` desde el BMP180 (conversión Pa -> hPa: `P = readPressure() / 100.0`).
- Si `dht.readTemperature()` o `dht.readHumidity()` devuelven `NaN`, el firmware registra el error en Serial y preserva las entradas anteriores en el buffer para no introducir datos inválidos en la SMA.

7) Cálculo de la media móvil (SMA)

- Insertar la lectura válida en la posición `bufIndex`, incrementar `bufIndex` módulo `SMA_SIZE` y, si aplica, incrementar `bufCount`.
- Calcular `smaTemp`, `smaHum` y `smaPres` usando la función `computeSMA()` que suma las `bufCount` muestras válidas y divide por `bufCount`.

8) Cálculos derivados

- Punto de rocío: aplicar la aproximación Magnus-Tetens (constantes a=17.27, b=237.7 °C). Se protege el logaritmo ante RH=0 con un clamp mínimo.
- Presión al nivel del mar: aplicar la fórmula barométrica `pressureSeaLevel(P, h)` con `ALTITUDE_METERS` (valor por defecto 1458 m); convertir a hPa antes de la corrección.

9) Exposición de datos y servidor web

- Construir una página HTML embebida (`buildHTML()`) y publicar un endpoint JSON `/data` que devuelve las variables filtradas: `temperature`, `humidity`, `pressure`, `pressure_sea`, `dew_point`, `timestamp`.
- Ejecutar el ESP32 en modo Access Point (`WiFi.mode(WIFI_AP)` + `WiFi.softAP(AP_SSID)`) con `AP_SSID = "ESP32_WeatherStation"` para facilitar pruebas sin infraestructura de red.
- Llamar `server.handleClient()` en el `loop()` para atender peticiones HTTP.

10) Pruebas, diagnóstico y ajustes

- Verificar detección de sensores en el arranque a través de mensajes Serial.
- Conectarse al AP del ESP32 y comprobar la página en `http://192.168.4.1/` y el JSON en `/data`.
- Registrar fallos de lectura en Serial e iterar sobre tiempos de muestreo y `SMA_SIZE` para equilibrar ruido/latencia.

11) Documentación y entrega

- Crear un diagrama de flujo visual del código `esp32_weather_station.ino`
- Documentar los pines, parámetros (`SAMPLE_INTERVAL`, `SMA_SIZE`, `ALTITUDE_METERS`) y el uso del firmware en `firmware/README.md`.
- Exportar resultados y capturas para incluir en el informe y calcular métricas de error frente a referencias externas.

Notas finales

Este paso a paso refleja exactamente las decisiones implementadas en el sketch: muestreo con `millis()`, buffers circulares con SMA, protección frente a `NaN`, conversión de unidades (Pa->hPa), fórmulas para punto de rocío y presión nivel mar, y servidor web en modo AP para monitorización local.

## 4. Consideraciones de diseño

### Hardware

Componentes principales:

- ESP32 (por ejemplo WROOM32)
- Sensor de presión BMP180/BMP085 (módulo I2C económico)
- Sensor DHT11 para temperatura y humedad (digital, económico)
- Cables, protoboard o placa perforada, fuente 5V/3.3V según módulos o simplemente conexión a una batería portátil o PC con Arduino IDE

Conexiones recomendadas:

- BMP180 (I2C): VCC -> 3.3V, GND -> GND, SDA -> GPIO21, SCL -> GPIO22
- DHT11: VCC -> 3.3V (o 5V si el módulo lo requiere), GND -> GND, DATA -> GPIO4 (configurable en el firmware)

Notas:

- El DHT11 es económico pero presenta menor precisión y menor tasa de muestreo que DHT22; si se requiere mayor exactitud, recomiendo DHT22.
- El BMP180 es compatible con la librería Adafruit BMP085 Unified y proporciona medidas de presión adecuadas para propósitos académicos.

### Software

El firmware, desarrollado para Arduino IDE, realiza las siguientes tareas principales:

1. Inicializa periféricos (I2C, DHT, servidor web, Serial).
2. Inicia un punto de acceso (AP) local para servir el dashboard — no requiere conexión a una red externa en la versión actual.
3. Bucle periódico: lectura de sensores (DHT11 y BMP180), aplicación de SMA, cálculo de punto de rocío y presión al nivel del mar, actualización de variables compartidas y salida por Serial.
4. Servir página HTML estática y endpoint JSON `/data` con las lecturas actuales; la página realiza peticiones periódicas (fetch) para actualizar el dashboard.

El código fuente principal se encuentra en: `firmware/esp32_weather_station/esp32_weather_station.ino` del repositorio.

Parámetros de configuración en el firmware (valores por defecto en el sketch):

- Intervalo de muestreo: `SAMPLE_INTERVAL = 2000` ms (2 s)
- Tamaño de la media móvil (SMA): `SMA_SIZE = 6` muestras
- Altitud para corrección barométrica: `ALTITUDE_METERS = 1458` m (ajustable)
- Pines I2C (BMP180): SDA = GPIO21, SCL = GPIO22
- Pin DHT11: `DHTPIN = GPIO4`
- Modo de red: Access Point por defecto; SSID por defecto `ESP32_WeatherStation` (IP típica `192.168.4.1`)
- Endpoints HTTP: `/` (dashboard) y `/data` (JSON con campos: temperature, humidity, pressure, pressure_sea, dew_point, timestamp)

## 5. Diagrama de flujo

El flujo secuencial es:

1. Inicio: inicializar Serial, I2C, BMP180 y DHT11.
2. Iniciar un punto de acceso (AP) local y no depender de una red externa: crear SSID local (por defecto `ESP32_WeatherStation`) y exponer la IP (por defecto `192.168.4.1`).
3. Iniciar servidor web y endpoints.
4. Bucle principal:
   - Leer DHT11 (T, RH) y BMP180 (P).
   - Actualizar buffers para SMA y calcular valores filtrados.
   - Calcular punto de rocío y presión a nivel del mar.
   - Enviar datos por Serial y atender clientes HTTP.

![Diagrama de flujo del sketch](report/diagram.png)

## 6. Protocolo de pruebas y resultados esperados

Pruebas recomendadas:

- Verificar detección de sensores en el arranque (logs Serial).
- Conectarse a la red WiFi creada por el ESP32 (AP) y acceder al dashboard vía navegador en `http://192.168.4.1/` (o la IP mostrada en Serial).
- Verificar el endpoint JSON `http://<AP_IP>/data` devuelve un objeto con las claves: `temperature`, `humidity`, `pressure`, `pressure_sea`, `dew_point`, `timestamp`.
- Comparar lecturas con un termómetro y barómetro de referencia, calcular error medio y desviación estándar.

Métricas reportadas:

- Error medio y RMSE para temperatura y presión frente a referencia están dentro del rango aceptable de precisión.
- Comportamiento temporal de la SMA (estabilidad vs. latencia) adecuada para la correcta visualización de los datos.
- Capturas del dashboard y logs Serial.

## 7. Discusión

Tomando como referencia los datasheets y mis conocimientos en programación, encontré algunos problemas que pueden surgir a la hora de implementar esta estación meteorologica en un entorno más real de trabajo, estos problemas los llamaré `fuentes de error` y serán descritas a continuación.

Fuentes de error: precisión limitada del DHT11, dependencia de la altitud para la corrección barométrica, influencia de la ubicación del sensor (radiación solar, calor por efecto de cercanía al MCU) y la necesidad de calibración para mediciones precisas según ubicación.

Posibles soluciones: 
- Cambiar DHT11 por DHT22 para mayor precisión
- Añadir algún sensor para detectar la altitud y ubicación de la estación, esto se puede hacer de varias maneras, incluyendo el uso de un GPS y una API que provea el dato de altitud y se introduzca automaticamente en el código

## 8. Conclusiones y trabajo futuro

El proyecto demuestra la viabilidad de construir una estación meteorológica económica con ESP32, BMP180 y DHT11, y de publicar un dashboard web embebido. Trabajo futuro recomendado:

- Reemplazar DHT11 por DHT22 o sensores más precisos.
- Implementar registro de datos en SD o envío a servidor remoto para análisis histórico.
- Implementar calibración y pruebas de campo para validar precisión.

## Referencias
1. Adafruit Industries, "Adafruit BMP180 / BMP085 Barometric Pressure + Temperature sensor — Datasheet", Adafruit, 2015. [Online]. Available: https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf

2. ETC (Electronic Technology Co.), "DHT11 Temperature and Humidity Sensor Datasheet". [Online]. Available: https://www.alldatasheet.com/datasheet-pdf/pdf/1440068/ETC/DHT11.html

3. Adafruit, "Adafruit-BMP085-Library" (GitHub repository). [Online]. Available: https://github.com/adafruit/Adafruit-BMP085-Library

4. Adafruit, "DHT-sensor-library" (GitHub repository). [Online]. Available: https://github.com/adafruit/DHT-sensor-library

5. Arduino Documentation, "Wire (I2C)". [Online]. Available: https://docs.arduino.cc/learn/communication/wire/

6. Arduino Libraries, "WiFi" (GitHub). [Online]. Available: https://github.com/arduino-libraries/WiFi

7. Espressif, "WebServer.h" (ESP32 Arduino core). [Online]. Available: https://github.com/espressif/arduino-esp32/blob/master/libraries/WebServer/src/WebServer.h

8. Coordinates Converter, "Altímetro / conversor de coordenadas" (web tool). [Online]. Available: https://coordinates-converter.com/es/altimetro

9. Corporate Finance Institute, "Simple Moving Average (SMA)" (article). [Online]. Available: https://corporatefinanceinstitute.com/resources/career-map/sell-side/capital-markets/simple-moving-average-sma/

10. Calculator Academy, "Fog / Dew point calculator" (explanatory page). [Online]. Available: https://calculator.academy/fog-equation-calculator/

11. Ayeeg, "How is dew point calculated?" (explanatory article). [Online]. Available: https://ayeeg.com/how-is-dew-point-calculated/

12. Arduino Forum, "Barometric pressure to altitude" (discussion and examples). [Online]. Available: https://forum.arduino.cc/t/barometric-pressure-to-altitude/297866

13. WXForum, "Discussion on barometric correction and practical use". [Online]. Available: https://www.wxforum.net/index.php?topic=25549.0

14. O. V. Alduchov and R. J. Eskridge, "Improved Magnus Form Approximation of Saturation Vapor Pressure," Journal of Applied Meteorology, vol. 35, pp. 601–609, 1996.
 
15. Mermaid, "Mermaid — JavaScript-based diagramming and visualization tool", [Online]. Available: https://mermaid.js.org
