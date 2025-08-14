/*
  esp32_weather_station.ino
  Sketch para ESP32 que lee un sensor BMP180 (I2C) y sirve un dashboard web.
  Requisitos de librería (Arduino IDE Library Manager):
   - Adafruit BMP180 Library
   - Adafruit Unified Sensor
   - DHT Sensor Library

   Revisa el archivo `referencias.md` para obtener las librerias que te falten

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
const char* SSID = "a";          // <- PON TU SSID
const char* PASSWORD = "mmgvo2002";  // <- PON TU PASSWORD

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

// buffers para SMA
float tempBuf[SMA_SIZE];
float humBuf[SMA_SIZE];
float presBuf[SMA_SIZE];
int bufIndex = 0;
int bufCount = 0;

float smaTemp = 0, smaHum = 0, smaPres = 0;

String htmlPage = ""; // contenido de la página web (servido dinámicamente)

// ---------- FUNCIONES AUXILIARES ----------
float computeSMA(float *buf, int count) {
  float s = 0.0;
  for (int i = 0; i < count; ++i) s += buf[i];
  return s / (float)count;
}

// Punto de rocío (Magnus)
float dewPoint(float T, float RH) {
  // T en °C, RH en %
  const float a = 17.27;
  const float b = 237.7; // °C
  float gamma = (a * T) / (b + T) + log(RH / 100.0);
  float Td = (b * gamma) / (a - gamma);
  return Td;
}

// Presión al nivel del mar (approx)
float pressureSeaLevel(float P, float altitude) {
  // P en hPa, altitude en metros
  return P / pow(1 - (altitude / 44330.0), 5.255);
}

// Construye página HTML simple con fetch para /data
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
  <div class="card">Última actualización: <span id="time">-</span></div>

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
    document.getElementById('time').textContent = new Date(j.timestamp * 1000).toLocaleString();
    document.getElementById('ip').textContent = window.location.host;
  } catch (e) {
    console.log('update error', e);
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

// Endpoint /data
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
  payload += "\"timestamp\":" + String((long)(millis() / 1000));
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

  // Conectar WiFi
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Conectando a WiFi");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print('.');
    tries++;
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Conectado. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("No se pudo conectar a la red WiFi. Revisa SSID/Password.");
  }

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
      // keep previous sma values (buffers unchanged)
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
