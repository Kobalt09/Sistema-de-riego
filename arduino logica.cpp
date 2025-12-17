// **************************************************************************
// CONFIGURACIÓN BLYNK
// **************************************************************************
// 1. Ve a blynk.cloud -> Templates -> Crear Nuevo Template
// 2. Copia estas 3 lineas de "Firmware Configuration" y pégalas aquí:
#define BLYNK_TEMPLATE_ID "TMPL2M3Hol8vV"
#define BLYNK_TEMPLATE_NAME "Monitor de riego"
#define BLYNK_AUTH_TOKEN "MkGbBwmpKaV2DYEUepVLTzdiXVmdzaYc"

// **************************************************************************
// LIBRERÍAS
// **************************************************************************
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <time.h>
#include <ESP32Servo.h>

// **************************************************************************
// CREDENCIALES WIFI
// **************************************************************************
char ssid[] = "PVZ";
char pass[] = "";

// **************************************************************************
// OBJETOS Y VARIABLES
// **************************************************************************
Servo controlsensor;
Servo pumpServo;
BlynkTimer timer;

// PINES
const int sensorPin = 16;
const int ledMeasuring = 2; 
const int ledWaiting = 4;   
const int pumpSignalPin = 18; 

// CONFIGURACIÓN (Valores por defecto)
int humedadMin = 40;
unsigned long interval = 15;       
unsigned long pumpTimeMs = 3000;

// HELPERS
int timerId = -1; // Para controlar el timer dinámico

// **************************************************************************
// FUNCIONES LÓGICAS (Hardware)
// **************************************************************************

// Función principal de medición
void medirHumedad() {
  Serial.println(">>> Iniciando medición...");
  
  // LED
  digitalWrite(ledMeasuring, HIGH);
  digitalWrite(ledWaiting, LOW);

  // Mover sensor
  controlsensor.write(90);
  delay(700); // Pequeño delay bloqueante inevitable por el servo

  // Leer
  int valor = analogRead(sensorPin);
  int humedad = map(valor, 0, 4095, 0, 100);
  
  // Imprimir y Enviar a Blynk
  Serial.print("Humedad: ");
  Serial.println(humedad);
  Blynk.virtualWrite(V0, humedad); // ENVIAR DATO A V0

  // Regresar sensor
  controlsensor.write(0);
  delay(500);

  // Evaluar Riego
  if (humedad < humedadMin) {
    Serial.println("Humedad BAJA. Riego ACTIVADO.");
    Blynk.logEvent("alerta_riego", "Humedad baja detectada, regando..."); // Opcional: Evento
    /*
    pumpServo.write(90); // ON
    delay(pumpTimeMs);   
    pumpServo.write(0);  // OFF
    
    Serial.println("Riego FINALIZADO.");
    */
  } else {
    Serial.println("Humedad OK.");
  }

  // Restaurar LEDs
  digitalWrite(ledMeasuring, LOW);
  digitalWrite(ledWaiting, HIGH);
}

// **************************************************************************
// HANDLERS DE BLYNK (Inputs desde la App)
// **************************************************************************

// V1: Slider/Input para Humedad Mínima
BLYNK_WRITE(V1) {
  humedadMin = param.asInt();
  Serial.print("Nueva Humedad Min: ");
  Serial.println(humedadMin);
}

// V2: Slider/Input para Intervalo (segundos)
BLYNK_WRITE(V2) {
  int nuevoIntervalo = param.asInt();
  if (nuevoIntervalo < 5) nuevoIntervalo = 5; // Protección mínima
  
  if (nuevoIntervalo != interval) {
    interval = nuevoIntervalo;
    Serial.print("Nuevo Intervalo: ");
    Serial.println(interval);
    
    // Reiniciar timer con nuevo tiempo
    if (timerId != -1) {
      timer.deleteTimer(timerId);
    }
    timerId = timer.setInterval(interval * 1000L, medirHumedad);
  }
}

// Cuando se conecta, sincronizamos valores por si la App tiene otros
BLYNK_CONNECTED() {
  Blynk.syncVirtual(V1, V2);
}

// **************************************************************************
// SETUP Y LOOP
// **************************************************************************

void setup() {
  Serial.begin(115200);

  // Configurar Pines
  controlsensor.attach(13); // Pin compatible con ESP32
  pumpServo.attach(pumpSignalPin);
  pumpServo.write(0); // Asegurar apagado

  pinMode(ledMeasuring, OUTPUT);
  pinMode(ledWaiting, OUTPUT);
  digitalWrite(ledMeasuring, LOW);
  digitalWrite(ledWaiting, HIGH);

  // Conectar a Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Configurar timer inicial
  timerId = timer.setInterval(interval * 1000L, medirHumedad);
  
  Serial.println("Sistema Blynk Iniciado.");
}

void loop() {
  Blynk.run();
  timer.run();
}