#include <WiFi.h>
#include <WebServer.h>
#include <Servo.h>

// --------------------------------------------------------------------------
// CREDENCIALES WIFI - ¡EDITA ESTO!
const char* ssid = "TU_RED_WIFI";
const char* password = "TU_CONTRASEÑA_WIFI";
// --------------------------------------------------------------------------

WebServer server(80);

Servo controlsensor;
Servo pumpServo;

// CONFIGURACIONES MODIFICABLES
int humedadMin = 40;               // Humedad mínima aceptada
unsigned long interval = 15;       // Intervalo de medición (segundos)
unsigned long pumpTimeMs = 3000;   // Tiempo de "bombeo" en milisegundos

// PINES
const int sensorPin = A0;
const int ledMeasuring = 7; // LED encendido mientras mide
const int ledWaiting = 4;   // LED encendido mientras espera
const int pumpSignalPin = 2; // Señal del servo que hará la acción de bomba

// POSICIONES DEL SERVO BOMBA
const int pumpOffAngle = 0;    // posición reposo
const int pumpOnAngle = 90;    // posición que provoca flujo

// VARIABLES GLOBALES
unsigned long lastCheck = 0;
int ultimaHumedad = 0; // Para guardar el último valor leído

// --------------------------------------------------------------------------
// FUNCIONES DE CONTROL
// --------------------------------------------------------------------------

void medirHumedad() {
  // Indicar que estamos midiendo
  digitalWrite(ledMeasuring, HIGH);
  digitalWrite(ledWaiting, LOW);

  // Mover servo a posicion de lectura
  controlsensor.write(90);
  delay(700);

  int valor = analogRead(sensorPin);
  int humedad = map(valor, 0, 1023, 0, 100);
  ultimaHumedad = humedad;

  Serial.print("Humedad medida: ");
  Serial.println(humedad);

  // Regresar servo sensor
  controlsensor.write(0);
  delay(500);

  if (humedad < humedadMin) {
    Serial.println("Humedad baja. Activando bomba (servo)...");
    pumpServo.write(pumpOnAngle);
    delay(pumpTimeMs); 
    pumpServo.write(pumpOffAngle);
    Serial.println("Riego finalizado.");
  }

  // Volver a estado esperando
  digitalWrite(ledMeasuring, LOW);
  digitalWrite(ledWaiting, HIGH);
}

// --------------------------------------------------------------------------
// FUNCIONES DEL SERVIDOR WEB
// --------------------------------------------------------------------------

void addCorsHeaders() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET,POST,OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "*");
}

void handleRoot() {
  addCorsHeaders();
  String html = "<h1>Sistema de Riego ESP32</h1>";
  html += "<p>Humedad Ultima: " + String(ultimaHumedad) + "%</p>";
  html += "<p>Config Humedad Min: " + String(humedadMin) + "%</p>";
  html += "<p>Intervalo: " + String(interval) + "s</p>";
  server.send(200, "text/html", html);
}

void handleData() {
  addCorsHeaders();
  // Devuelve JSON con el estado
  String json = "{";
  json += "\"humedad\": " + String(ultimaHumedad) + ",";
  json += "\"conf_min\": " + String(humedadMin) + ",";
  json += "\"conf_interval\": " + String(interval);
  json += "}";
  server.send(200, "application/json", json);
}

void handleSet() {
  addCorsHeaders();
  // Ejemplo de uso: /set?hum=30&time=60
  if (server.hasArg("hum")) {
    humedadMin = server.arg("hum").toInt();
    Serial.println("Config HUM actualizada: " + String(humedadMin));
  }
  if (server.hasArg("time")) {
    interval = server.arg("time").toInt();
    Serial.println("Config TIME actualizada: " + String(interval));
  }
  
  server.send(200, "text/plain", "OK");
}

void handleOptions() {
  addCorsHeaders();
  server.send(200);
}

// --------------------------------------------------------------------------
// SETUP Y LOOP
// --------------------------------------------------------------------------

void setup() {
  Serial.begin(9600);
  
  // Pines
  controlsensor.attach(9);
  pumpServo.attach(pumpSignalPin);
  pumpServo.write(pumpOffAngle);

  pinMode(ledMeasuring, OUTPUT);
  pinMode(ledWaiting, OUTPUT);
  digitalWrite(ledMeasuring, LOW); // LED start
  digitalWrite(ledWaiting, HIGH);

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Conectado! IP: ");
  Serial.println(WiFi.localIP());

  // Rutas Web
  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleData);
  server.on("/set", HTTP_GET, handleSet); // Usando GET por simplicidad en tests rapidos
  server.on("/set", HTTP_POST, handleSet);
  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
        handleOptions();
    } else {
        server.send(404, "text/plain", "Not found");
    }
  });

  server.begin();
  Serial.println("Servidor HTTP iniciado");
}

void loop() {
  server.handleClient(); // Atender peticiones web

  unsigned long now = millis();
  if (now - lastCheck >= interval * 1000UL) {
    lastCheck = now;
    medirHumedad();
  }
}