# Estación Meteorológica — Proyecto (ESP32)

Este documento explica paso a paso cómo diseñar, implementar y probar una estación meteorológica con ESP32 que mide presión atmosférica, temperatura y humedad, y muestra un dashboard web alojado en el propio ESP32.
## Checklist de requisitos

- [ ] 1. Metodología y modelo matemático
- [ ] 2. Consideraciones de diseño (hardware y MCU)
- [ ] 3. Diagrama de flujo del software
- [ ] 4. Elemento(s) de visualización
- [ ] 5. Prototipo funcional (código + pruebas)
- [ ] 6. Informe en formato IEEE
- [ ] 7. Repositorio en GitLab con trazabilidad

En las secciones siguientes se desarrollan cada uno de los puntos solicitados y se incluyen artefactos (firmware, instrucciones y plantilla de informe).
## 1. Metodología y modelo matemático

Objetivo: medir temperatura (T), humedad relativa (RH) y presión atmosférica (P) y ofrecer:

- Lecturas en tiempo real (dashboard web)
- Valores procesados: presión corregida al nivel del mar y punto de rocío

Metodología (resumen):

1. Tomar muestras periódicas del sensor (intervalo configurable, p.ej., 2–10 s).
2. Aplicar un filtro de medias móviles para reducir ruido.
3. Calcular variables derivadas (punto de rocío, presión al nivel del mar) usando fórmulas estándar.
4. Entregar lecturas crudas y procesadas por el servidor web integrado.
Modelo matemático y fórmulas:

- Media móvil simple (SMA) sobre N muestras: SMA = (1/N) * sum_{i=0}^{N-1} x[i]
- Punto de rocío (Magnus-Tetens aproximado):

	a = 17.27
	b = 237.7  (°C)
	gamma = (a * T) / (b + T) + ln(RH/100)
	Td = (b * gamma) / (a - gamma)

	donde T está en °C y RH en % (0-100). El resultado Td está en °C.
- Presión al nivel del mar (fórmula barométrica aproximada):

	P0 = P / pow(1 - (altitude / 44330.0), 5.255)

	donde P es la presión medida en hPa, altitude es la altura del sensor en metros (m) y P0 es la presión corregida al nivel del mar en hPa.

Observaciones: para alta precisión conviene calibrar con estaciones locales y considerar la temperatura media de la columna de aire; de todas formas la fórmula anterior es suficiente para trabajos académicos.
## 2. Consideraciones de diseño

Requisitos funcionales:
- Medir T, RH, P.
- Comunicación WiFi (ESP32) para servir dashboard.
- Interfaz web sencilla con actualizaciones periódicas (AJAX: JavaScript asíncrono y XML).

Hardware externo sugerido (opciones económicas, sin depender de Adafruit):

- Sensores: BMP180/BMP085 (presión) + DHT11 o DHT22 (humedad y temperatura). BMP180 es económico y fácil de usar por I2C; DHT11 es la opción más barata para T/RH (DHT22 mejora precisión si está disponible).
- Microcontrolador: ESP32 (p.ej., WROOM32 o nodemcu-32s).
- Cables y protoboard o placa perf. O placa base personalizada.

Conexiones (BMP180 por I2C, DHT por pin digital):
- BMP180 VCC -> 3.3V
- BMP180 GND -> GND
- BMP180 SDA -> ESP32 SDA pin (por defecto GPIO21)
- BMP180 SCL -> ESP32 SCL pin (por defecto GPIO22)
- DHT DATA -> pin digital configurable (ej. GPIO4) ; VCC -> 3.3V, GND -> GND

Consideraciones internas del MCU:

- Pines: reservar GPIO21(SDA) y GPIO22(SCL) para I2C; usar pines libres para LEDs u otros actuadores.
- Memoria: página web pequeña embebida en el sketch; suficiente en ESP32.
- Consumo: para uso con batería, implementar modos de ahorro (deep sleep) y wake periódicamente; en este proyecto asumimos alimentación permanente.
- Seguridad: proteger SSID/contraseña y añadir opción para cambiar credenciales en el futuro (por ejemplo captive portal o endpoint seguro), no séra usado en este proyecto dada la simplicidad que el mismo requiere.
Librerías recomendadas (Arduino IDE Library Manager):
- "Adafruit BMP085 Unified" (compatible con BMP180/BMP085) o librería equivalente para BMP180.
- "DHT sensor library" (p. ej. la de Adafruit) para DHT11/DHT22.
Alternativa: usar librerías de SparkFun o implementaciones ligeras si se quiere evitar Adafruit.

## 3. Diagrama de flujo (secuencial)

Resumen en texto (pseudodiagrama):

1. Inicio
2. Inicializar Serial, I2C, sensor BMP180
3. Conectar a WiFi
	- Si falla, seguir reintentando y reportar por Serial
4. Iniciar servidor web y/o endpoints.
5. Bucle principal:
	a. Leer sensor (T, RH, P)
	b. Aplicar filtro (SMA)
	c. Calcular punto de rocío y presión al nivel del mar (con altura configurada)
	d. Actualizar estructura de datos compartida
	e. Imprimir por Serial/Mandar datos a la web
	f. Esperar intervalo de muestreo
6. Responder peticiones HTTP: página estática y endpoint JSON con datos actuales
7. Fin (solo en apagado)
Diagrama de flujo sencillo (ASCII):

Start
	|
	v
Init peripherals (Serial, I2C, Sensor)
	|
Connect WiFi -> If fail retry
	|
Start WebServer
	|
Loop:
	|- Read sensor -> Filter -> Compute derived -> Update shared data -> Serial print
	|- Handle HTTP requests
	v
 Repeat

## 4. Visualización de datos

Se proveen dos elementos de visualización:

- Terminal Serial (IDE Arduino): para debug y salida periódica de lecturas.
- Dashboard web alojado en el ESP32: página HTML+JS que consulta /data cada 2 s y actualiza valores en pantalla.

La página muestra: temperatura (°C), humedad (%), presión (hPa), presión nivel mar (hPa), punto de rocío (°C) y timestamp.
## 5. Prototipo funcional (firmware)

Archivos incluidos en este repositorio:

- `firmware/esp32_weather_station.ino` — Sketch para Arduino IDE (ESP32). Incluye servidor web y lectura de BMP180.
- `firmware/README.md` — Instrucciones rápidas para subir el sketch y librerías requeridas.

Pasos para probar:

1. Conecta el BMP180 al ESP32 por I2C (SDA=21, SCL=22 por defecto).
2. Instala las librerías indicadas en el `firmware/README.md`.
3. Abre y edita el archivo `.ino` para poner tu SSID y contraseña.
4. Sube el sketch desde Arduino IDE (board: ESP32 Dev Module).
5. Abre la IP mostrada por Serial en tu navegador para ver el dashboard.
## 6. Informe en formato IEEE

En `report/` se incluye un archivo `IEEE_report.md` con la estructura básica (plantilla) para pasar al informe final en el archivo `IEEE_report.doc`. Debe contener: Resumen, Introducción, Metodología, Diseño de hardware, Diseño de software, Resultados, Discusión, Conclusiones y Referencias.

Las referencias se pueden encontrar en el archivo `report/referencias.md`

## 7. Repositorio en GitLab

Para la calificación se creó este proyecto en GitLab y se importó este repositorio. Recomendaciones que se tomaron:

- Mantener branches: `main` para entrega, `dev` para desarrollo
- Usar commits descriptivos y Merge Requests
- Incluir issues y milestones para trazabilidad

---

Pasos a tomar:

1. Añadir el sketch `firmware/esp32_weather_station.ino` al ESP32.
2. Leer `firmware/README.md` con instrucciones.
3. Modificar `report/IEEE_report.doc` para hacer el reporte final.

---

Revisa la carpeta `firmware` creada en el repositorio para el código y procedimientos.
Proyecto: Estación Meteorológica (presión, temperatura y humedad)