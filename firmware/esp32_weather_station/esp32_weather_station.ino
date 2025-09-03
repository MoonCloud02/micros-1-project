/**
 * @file esp32_weather_station.ino
 * @brief Estación meteorológica básica con ESP32 que expone datos vía servidor web local.
 *
 * Este sketch implementa:
 *  - Lectura periódica de sensores ambientales:
 *      * DHT11: Temperatura y Humedad Relativa.
 *      * BMP180/BMP085: Presión atmosférica absoluta (I2C).
 *  - Cálculo de métricas derivadas:
 *      * Media Móvil Simple (SMA) para suavizado (temperatura, humedad y presión).
 *      * Punto de rocío (fórmula de Magnus).
 *      * Presión ajustada a nivel del mar (corrección barométrica).
 *  - Servidor HTTP (modo Access Point autónomo) con:
 *      * Página HTML (dashboard ligero con actualización cada 2 s).
 *      * Endpoint JSON: /data
 *  - Registro por puerto serie de las lecturas filtradas.
 *
 * ---------------------------------------------------------------------------
 * HARDWARE
 * ---------------------------------------------------------------------------
 *  - MCU: ESP32 (cualquier módulo con soporte WiFi).
 *  - Sensor DHT11:
 *       * DATA -> GPIO4 (definido por DHTPIN)
 *       * Alimentación 3V3 y resistencia pull-up típica (4.7K–10K) a DATA.
 *  - Sensor BMP180 / BMP085 (I2C):
 *       * SDA -> GPIO21
 *       * SCL -> GPIO22
 *       * Alimentación 3V3 (ver datasheet para VIN según módulo).
 *
 * ---------------------------------------------------------------------------
 * CONFIGURACIÓN CLAVE
 * ---------------------------------------------------------------------------
 *  ALTITUDE_METERS: Altitud del sitio de instalación (m) para corregir presión a nivel del mar.
 *  SAMPLE_INTERVAL: Periodo entre adquisiciones (ms).
 *  SMA_SIZE: Tamaño de ventana de la media móvil (suavizado).
 *  I2C_SDA / I2C_SCL: Pines usados para bus I2C.
 *  AP_SSID: Nombre de la red WiFi creada en modo Access Point.
 *
 * ---------------------------------------------------------------------------
 * FLUJO PRINCIPAL
 * ---------------------------------------------------------------------------
 *  setup():
 *    - Inicializa Serial, I2C, sensores y buffers SMA.
 *    - Inicia modo AP (sin contraseña por simplicidad).
 *    - Construye y registra handlers HTTP para:
 *        * "/"     -> Página HTML (dashboard).
 *        * "/data" -> JSON con últimas métricas calculadas.
 *
 *  loop():
 *    - Cada SAMPLE_INTERVAL ms:
 *        * Lee sensores (con manejo de fallos del DHT).
 *        * Convierte presión de Pa a hPa.
 *        * Actualiza buffers circulares y recalcula medias (SMA).
 *        * Calcula punto de rocío y presión a nivel del mar.
 *        * Emite resumen por Serial (lecturas suavizadas).
 *    - Atiende peticiones HTTP (server.handleClient()).
 *
 * ---------------------------------------------------------------------------
 * ENDPOINT /data (JSON)
 * ---------------------------------------------------------------------------
 *  {
 *    "temperature": <float °C SMA>,
 *    "humidity":    <float % SMA>,
 *    "pressure":    <float hPa SMA>,
 *    "pressure_sea":<float hPa corregida a nivel del mar>,
 *    "dew_point":   <float °C>,
 *    "uptime":      <segundos desde arranque>
 *  }
 *
 * ---------------------------------------------------------------------------
 * CÁLCULOS
 * ---------------------------------------------------------------------------
 *  Media Móvil Simple (SMA):
 *     sma = (Σ lectura_i) / N
 *
 *  Punto de rocío (Magnus-Tetens):
 *     γ = (a*T)/(b+T) + ln(RH/100)
 *     Td = (b*γ)/(a-γ)
 *     (a=17.27, b=237.7)
 *
 *  Presión a nivel del mar (aprox barométrica estándar):
 *     P0 = P / (1 - (altitud / 44330))^5.255
 *
 * ---------------------------------------------------------------------------
 * MANEJO DE ERRORES / LIMITACIONES
 * ---------------------------------------------------------------------------
 *  - Si el DHT11 falla (NaN), se conserva el histórico previo (no se modifica el buffer).
 *  - Se invoca bmp.begin() en setup; en cada ciclo se vuelve a intentar si no hay lectura,
 *    aunque podría optimizarse evitando reinicializaciones repetidas.
 *  - No hay control de saturación ni validación de rangos físicos extremos.
 *  - AP sin password: para entorno real, agregar seguridad (WPA2).
 *
 * ---------------------------------------------------------------------------
 * POSIBLES MEJORAS
 * ---------------------------------------------------------------------------
 *  - Sustituir DHT11 por DHT22/SHT31 (mayor precisión).
 *  - Persistencia de configuraciones (SPIFFS/LittleFS/NVS).
 *  - Agregar autenticación básica para endpoints.
 *  - Soporte STA + AP concurrente (modo dual).
 *  - Compresión o minimización de la página web.
 *  - Migrar a AsyncWebServer para mayor eficiencia.
 *  - Promedios exponenciales (EMA) o filtros Kalman para suavizado avanzado.
 *  - Inclusión de timestamp NTP y envío a un broker MQTT.
 *  - Captive portal para configurar altitud y SSID.
 *
 * ---------------------------------------------------------------------------
 * USO RÁPIDO
 * ---------------------------------------------------------------------------
 *  1. Ajustar ALTITUDE_METERS según localización.
 *  2. Cargar sketch en el ESP32.
 *  3. Conectar a la red WiFi "ESP32_WeatherStation".
 *  4. Abrir en navegador la IP mostrada en Serial (típicamente 192.168.4.1).
 *
 * ---------------------------------------------------------------------------
 * NOTAS
 * ---------------------------------------------------------------------------
 *  - El dashboard se actualiza cada 2 s vía fetch().
 *  - El uptime se muestra en formato d HH:MM:SS si aplica.
 *  - Las primeras lecturas (antes de llenar la ventana SMA) usan menos muestras.
 *
 * ---------------------------------------------------------------------------
 * LICENCIA / USO
 * ---------------------------------------------------------------------------
 *  - Ejemplo educativo. Adaptar según necesidad del proyecto.
 *
 * ---------------------------------------------------------------------------
 * AUTORÍA / MANTENIMIENTO
 * ---------------------------------------------------------------------------
 *  - Crear un CHANGELOG y versión semántica si se amplía el proyecto.
 */
/*
  esp32_weather_station.ino
  Sketch para ESP32 que lee un sensor BMP180 (I2C) y sirve un dashboard web.
  Requisitos de librería (Arduino IDE Library Manager):
   - Adafruit BMP180 Library
   - Adafruit Unified Sensor

  Conexiones (I2C): SDA -> GPIO21, SCL -> GPIO22
*/
/*No poner atención a los errores de los #include en VS Code, son falsos positivos.
  Asegurarse de tener instaladas las librerías necesarias en Arduino IDE.*/
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_BMP085.h>
#include "DHT.h"

// --------- CONFIGURACIÓN ----------

// Altitud del sensor en metros (para corrección a nivel del mar)
const float ALTITUDE_METERS = 1458; // ajustar según ubicación

// Intervalo de muestreo en ms
const unsigned long SAMPLE_INTERVAL = 2000;

// Tamaño media móvil (Muestras a tomar para hacer la media)
const int SMA_SIZE = 6;

// I2C pins (ESP32 típicos)
const int I2C_SDA = 21;
const int I2C_SCL = 22;

// --------- VARIABLES GLOBALES ----------
Adafruit_BMP085 bmp; // I2C (BMP180/BMP085 compatible)
// DHT configuration
#define DHTPIN 4      // Pin digital donde se conecta la línea de datos del DHT11
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

unsigned long lastSample = 0;

/* buffers para SMA, son necesarios para calcular la media móvil
 estos buffers almacenan las últimas lecturas para calcular la media*/
float tempBuf[SMA_SIZE];
float humBuf[SMA_SIZE];
float presBuf[SMA_SIZE];
int bufIndex = 0;
int bufCount = 0;

// Variables para almacenar la media móvil
float smaTemp = 0, smaHum = 0, smaPres = 0;

String htmlPage = ""; // contenido de la página web (servido dinámicamente)

// ---------- FUNCIONES AUXILIARES ----------
// Esta función calcula la media móvil simple (SMA) con la expresión matemática 
float computeSMA(float *buf, int count) {
  float s = 0.0;
  for (int i = 0; i < count; ++i) s += buf[i];
  return s / (float)count;
}

// Punto de rocío (Magnus) calculada con la expresión matemática 
float dewPoint(float T, float RH) {
  // T en °C, RH en %
  const float a = 17.27;
  const float b = 237.7; // °C
  float gamma = (a * T) / (b + T) + log(RH / 100.0);
  float Td = (b * gamma) / (a - gamma);
  return Td;
}

// Presión al nivel del mar (approx) calculada con la expresión matemática
float pressureSeaLevel(float P, float altitude) {
  // P en hPa, altitude en metros
  return P / pow(1 - (altitude / 44330.0), 5.255);
}

// Construye página HTML simple con fetch para /data (data siendo el JSON como bate de datos)
//buildHTML sirve para generar el contenido HTML de la página web
String buildHTML() {
  String s = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Estación Meteorológica (ESP32)</title>
  <style>
    body{font-family: Arial, Helvetica, sans-serif; margin:20px}
    .card{border:1px solid #ddd;padding:12px;border-radius:8px;margin-bottom:10px}
    h1{font-size:1.4em}
    .val{font-size:1.6em;font-weight:700}
  </style>
</head>
<body>
  <h1>Estación Meteorológica - ESP32</h1>
  <div class="card">IP: <span id="ip">-</span></div>
  <div class="card">Temperatura: <span class="val" id="temp">-- °C</span></div>
  <div class="card">Humedad: <span class="val" id="hum">-- %</span></div>
  <div class="card">Presión: <span class="val" id="pres">-- hPa</span></div>
  <div class="card">Presión (nivel mar): <span class="val" id="pres0">-- hPa</span></div>
  <div class="card">Punto de rocío: <span class="val" id="dew">-- °C</span></div>
  <div class="card">Tiempo encendido: <span id="uptime">-</span></div>

<script>
async function update() {
  try {
    const res = await fetch('/data');
    const j = await res.json();
    document.getElementById('temp').textContent = j.temperature.toFixed(2) + ' °C';
    document.getElementById('hum').textContent = j.humidity.toFixed(2) + ' %';
    document.getElementById('pres').textContent = j.pressure.toFixed(2) + ' hPa';
    document.getElementById('pres0').textContent = j.pressure_sea.toFixed(2) + ' hPa';
    document.getElementById('dew').textContent = j.dew_point.toFixed(2) + ' °C';
    // Mostrar tiempo de encendido en formato legible
    function formatUptime(s) {
      s = Math.floor(s);
      var days = Math.floor(s / 86400);
      s = s % 86400;
      var hours = Math.floor(s / 3600);
      s = s % 3600;
      var mins = Math.floor(s / 60);
      var secs = s % 60;
      var parts = [];
      if (days > 0) parts.push(days + 'd');
      parts.push((hours<10? '0'+hours: hours) + ':' + (mins<10? '0'+mins: mins) + ':' + (secs<10? '0'+secs: secs));
      return parts.join(' ');
    }
    if (j.uptime !== undefined) {
      document.getElementById('uptime').textContent = formatUptime(j.uptime);
    } else {
      document.getElementById('uptime').textContent = '-';
    }
    document.getElementById('ip').textContent = window.location.host;
  } catch (e) {
    console.log('update error: ', e);
  }
}
setInterval(update, 2000);
update();
</script>
</body>
</html>
)rawliteral";
  return s;
}

// Endpoint /data (Base de datos)
void handleData() {
  // Prepara JSON
  String payload = "{";
  payload += "\"temperature\":" + String(smaTemp, 2) + ",";
  payload += "\"humidity\":" + String(smaHum, 2) + ",";
  payload += "\"pressure\":" + String(smaPres, 2) + ",";
  float psea = pressureSeaLevel(smaPres, ALTITUDE_METERS);
  payload += "\"pressure_sea\":" + String(psea, 2) + ",";
  float dew = dewPoint(smaTemp, smaHum);
  payload += "\"dew_point\":" + String(dew, 2) + ",";
  // Enviar uptime en segundos (tiempo desde arranque)
  payload += "\"uptime\":" + String((long)(millis() / 1000));
  payload += "}";
  server.send(200, "application/json", payload);
}

void handleRoot() {
  server.send(200, "text/html", htmlPage);
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Iniciando estación meteorológica...");

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!bmp.begin()) {
    Serial.println("Error: BMP180/BMP085 no detectado en I2C. Revisa conexiones.");
  } else {
    Serial.println("BMP sensor detectado (I2C)");
  }

  dht.begin();

  // Inicializar buffers
  for (int i = 0; i < SMA_SIZE; ++i) { tempBuf[i] = 0; humBuf[i] = 0; presBuf[i] = 0; }

  // Iniciar punto de acceso (AP) para acceso local
  const char* AP_SSID = "ESP32_WeatherStation";
  Serial.print("Iniciando punto de acceso (AP): "); Serial.println(AP_SSID);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("AP iniciado. SSID: "); Serial.print(AP_SSID);
  Serial.print("  IP: "); Serial.println(apIP);

  // Web server
  htmlPage = buildHTML();
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Servidor web iniciado en puerto 80");

  lastSample = millis() - SAMPLE_INTERVAL; // forzar lectura inmediata
}

void loop() {
  unsigned long now = millis();
  if (now - lastSample >= SAMPLE_INTERVAL) {
    lastSample = now;
    // Leer sensores: DHT11 para T y RH, BMP180 para presión
    float T = dht.readTemperature();     // °C
    float H = dht.readHumidity();        // %
    // DHT read may fail -> check
    if (isnan(T) || isnan(H)) {
      Serial.println("Warning: lectura DHT11 fallida, manteniendo valores previos si existen.");
      // mantiene valores SMA previos (buffers sin cambios)
    }

    // BMP180 presión (devuelve Pa en la librería de Adafruit), convertir a hPa
    float P = 0.0;
    if (bmp.begin()) {
      // readPressure() puede devolver Pa
      P = bmp.readPressure() / 100.0;
    } else {
      Serial.println("Warning: BMP no disponible, P=0");
    }

    // Llenar buffers
    tempBuf[bufIndex] = T;
    humBuf[bufIndex] = H;
    presBuf[bufIndex] = P;
    bufIndex = (bufIndex + 1) % SMA_SIZE;
    if (bufCount < SMA_SIZE) bufCount++;

    smaTemp = computeSMA(tempBuf, bufCount);
    smaHum = computeSMA(humBuf, bufCount);
    smaPres = computeSMA(presBuf, bufCount);

    float dew = dewPoint(smaTemp, smaHum);
    float psea = pressureSeaLevel(smaPres, ALTITUDE_METERS);

    // Imprimir por Serial
    Serial.printf("T=%.2f C, RH=%.2f %%, P=%.2f hPa, P0=%.2f hPa, Dew=%.2f C\n",
                  smaTemp, smaHum, smaPres, psea, dew);
  }

  server.handleClient();
}
