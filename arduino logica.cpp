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
#include <BlynkSimpleEsp32.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <time.h>

// **************************************************************************
// CREDENCIALES WIFI
// **************************************************************************
char ssid[] = "Wokwi-GUEST";
char pass[] = "";

// **************************************************************************
// OBJETOS Y VARIABLES
// **************************************************************************
Servo controlsensor;
Servo pumpServo;
BlynkTimer timer;

// PINES
const int sensorPin = 34;

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

  // Leer potenciómetro
  int valor = analogRead(sensorPin);

  // MAPEO PARA POTENCIÓMETRO:
  // Si quieres que al girar a la derecha aumente la humedad: (0, 4095, 0, 100)
  // Si quieres simular un sensor real (normalmente invertido): (0, 4095, 100,0)
  int humedad = map(valor, 0, 4095, 0, 100);

  // Imprimir y Enviar a Blynk
  Serial.print("Humedad Medida: ");
  Serial.print(humedad);
  Serial.println("%");
  Blynk.virtualWrite(V0, humedad); // ENVIAR DATO A V0

  // Regresar sensor
  controlsensor.write(0);
  delay(500);

  // Evaluar Riego
  if (humedad < humedadMin) {
    Serial.println("Humedad BAJA. Riego ACTIVADO.");
    Blynk.logEvent("alerta_riego", "Humedad baja detectada, regando...");
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

// V1: Humedad Mínima (0 - 100)
BLYNK_WRITE(V1) {
  int val = param.asInt();
  // Validar rango 0-100
  if (val < 0)
    val = 0;
  if (val > 100)
    val = 100;

  humedadMin = val;
  Serial.print(">>> BLYNK_WRITE(V1): Nueva Humedad Min configurada en: ");
  Serial.println(humedadMin);
}

// V2: Intervalo de Medición (5 - 3600 seg)
BLYNK_WRITE(V2) {
  int nuevoIntervalo = param.asInt();

  // Validar rango según especificación (5s a 3600s)
  if (nuevoIntervalo < 5)
    nuevoIntervalo = 5;
  if (nuevoIntervalo > 3600)
    nuevoIntervalo = 3600;

  if (nuevoIntervalo != interval) {
    interval = nuevoIntervalo;
    Serial.print(">>> BLYNK_WRITE(V2): Nuevo Intervalo configurado en: ");
    Serial.print(interval);
    Serial.println(" seg");

    // Reiniciar timer con nuevo tiempo
    if (timerId != -1) {
      timer.deleteTimer(timerId);
    }
    timerId = timer.setInterval(interval * 1000L, medirHumedad);
  }
}

// Cuando se conecta, sincronizamos valores por si la App tiene otros
BLYNK_CONNECTED() {
  Serial.println(">>> Blynk Conectado. Sincronizando Datastreams V1 y V2...");
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