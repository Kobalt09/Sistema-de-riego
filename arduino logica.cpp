#include <Servo.h>

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

// POSICIONES DEL SERVO BOMBA (ajustar según tu montaje)
const int pumpOffAngle = 0;    // posición reposo
const int pumpOnAngle = 90;    // posición que provoca flujo

// CONTROL DE TIEMPO
unsigned long lastCheck = 0;

void setup() {
  Serial.begin(9600);

  controlsensor.attach(9);
  pumpServo.attach(pumpSignalPin);
  pumpServo.write(pumpOffAngle);

  pinMode(ledMeasuring, OUTPUT);
  pinMode(ledWaiting, OUTPUT);
  digitalWrite(ledMeasuring, LOW);
  digitalWrite(ledWaiting, HIGH); // Por defecto: sistema esperando

  Serial.println("Sistema iniciado. Esperando comandos...");
  Serial.println("Comandos disponibles:");
  Serial.println("SET HUM <valor>");
  Serial.println("SET TIME <segundos>");
}

void loop() {
  recibirComandos();

  unsigned long now = millis();
  if (now - lastCheck >= interval * 1000UL) {
    lastCheck = now;
    medirHumedad();
  }
}

void medirHumedad() {
  // Indicar que estamos midiendo
  digitalWrite(ledMeasuring, HIGH);
  digitalWrite(ledWaiting, LOW);

  // Mover servo a posicion de lectura
  controlsensor.write(90);
  delay(700);

  int valor = analogRead(sensorPin);
  int humedad = map(valor, 0, 1023, 0, 100);


  // Envío para la interfaz (formato simple y parseable)
  Serial.print("HUM ");
  Serial.println(humedad); // Ejemplo: "HUM 42"

  // (Opcional) enviar valor RAW si lo necesitas:
  // Serial.print("RAW ");
  // Serial.println(valor);

  // Regresar servo sensor
  controlsensor.write(0);
  delay(500);

  if (humedad < humedadMin) {
    Serial.println("Humedad baja. Activando bomba (servo)...");
    pumpServo.write(pumpOnAngle);
    delay(pumpTimeMs); // tiempo de riego; ajustar según necesidad y montaje
    pumpServo.write(pumpOffAngle);
    Serial.println("Riego finalizado.");
  }

  // Volver a estado esperando
  digitalWrite(ledMeasuring, LOW);
  digitalWrite(ledWaiting, HIGH);
}

void recibirComandos() {
  if (!Serial.available()) return;

  String comando = Serial.readStringUntil('\n');
  comando.trim();

  if (comando.startsWith("SET HUM")) {
    int valor = comando.substring(7).toInt();
    humedadMin = valor;
    Serial.print("Humedad mínima actualizada a: ");
    Serial.println(humedadMin);
  }
  else if (comando.startsWith("SET TIME")) {
    int valor = comando.substring(8).toInt();
    interval = valor;
    Serial.print("Intervalo actualizado a: ");
    Serial.print(interval);
    Serial.println(" segundos.");
  }
  else {
    Serial.println("Comando no reconocido.");
  }
}