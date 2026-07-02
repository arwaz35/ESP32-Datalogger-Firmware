/*
 * Ejemplos de SDLogger con múltiples tipos de datos
 * Muestra cómo usar int, long, float, double, uint8_t, etc.
 */

#include "SDLogger_Template.h"

SDLogger logger;

// ═══════════════════════════════════════════════════════════
// EJEMPLO 1: Array de un solo tipo
// ═══════════════════════════════════════════════════════════

void ejemplo1_array_simple() {
  // Arrays de diferentes tipos
  int sensoresInt[5] = {100, 200, 300, 400, 500};
  float sensoresFloat[5] = {25.5, 60.3, 1013.25, 3.14, 2.71};
  double sensoresDouble[5] = {3.14159265, 2.71828182, 1.41421356, 1.73205080, 2.23606797};
  long sensoresLong[5] = {1000000L, 2000000L, 3000000L, 4000000L, 5000000L};
  
  void setup() {
    Serial.begin(115200);
    delay(2000);
    
    if (!logger.begin(0, 10)) {
      Serial.println("Error");
      while(1);
    }
  }
  
  void loop() {
    // Escribir array de enteros
    logger.writeArray(sensoresInt, 5);
    
    // Escribir array de floats con 2 decimales
    logger.writeArray(sensoresFloat, 5, 2);
    
    // Escribir array de doubles con 6 decimales
    logger.writeArray(sensoresDouble, 5, 6);
    
    // Escribir array de longs (sin decimales)
    logger.writeArray(sensoresLong, 5, 0);
    
    delay(1000);
  }
}

// ═══════════════════════════════════════════════════════════
// EJEMPLO 2: Tipos mixtos en una misma línea (RECOMENDADO)
// ═══════════════════════════════════════════════════════════

void ejemplo2_tipos_mixtos() {
  // Variables de diferentes tipos
  unsigned long timestamp;
  int contador;
  float temperatura;
  double latitud;
  double longitud;
  uint8_t estado;
  
  void setup() {
    Serial.begin(115200);
    delay(2000);
    
    if (!logger.begin(0, 10)) {
      Serial.println("Error");
      while(1);
    }
    
    Serial.println("Grabando tipos mixtos...");
  }
  
  void loop() {
    // Actualizar valores
    timestamp = millis();
    contador++;
    temperatura = 25.5 + random(0, 100) * 0.01;
    latitud = 4.624335;
    longitud = -74.063644;
    estado = random(0, 256);
    
    // Escribir línea con tipos mixtos
    logger.startLine();
    logger.addValue(timestamp, 0);        // long sin decimales
    logger.addValue(contador, 0);         // int sin decimales
    logger.addValue(temperatura, 2);      // float con 2 decimales
    logger.addValue(latitud, 6);          // double con 6 decimales
    logger.addValue(longitud, 6);         // double con 6 decimales
    logger.addValue(estado, 0);           // uint8_t sin decimales
    logger.endLine();
    
    // Flush periódico
    static int count = 0;
    if (++count >= 20) {
      logger.flush();
      count = 0;
      Serial.println(".");
    }
    
    delay(100);
  }
}

// ═══════════════════════════════════════════════════════════
// EJEMPLO 3: Data logging real con 20 variables mixtas
// ═══════════════════════════════════════════════════════════

#include "SDLogger_Template.h"

#define CS_PIN 0

SDLogger logger;

// Variables de diferentes tipos
unsigned long timestamp;
int contador = 0;

// Sensores ambientales (float)
float temperatura1, temperatura2, temperatura3;
float humedad1, humedad2;
float presion;

// IMU (double para mayor precisión)
double accelX, accelY, accelZ;
double gyroX, gyroY, gyroZ;

// GPS (double para coordenadas)
double latitud, longitud;
float altitud;

// Sistema (int y uint8_t)
int voltajeMv;
uint8_t estadoBateria;
uint16_t rpm;

// Control de tiempo
unsigned long lastWriteTime = 0;
const unsigned long writeInterval = 100;  // 10 Hz
int writeCount = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  SDLogger - Tipos Mixtos (20 vars)    ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  if (!logger.begin(CS_PIN, 10)) {
    Serial.println("❌ Error al inicializar SD");
    while(1) delay(1000);
  }
  
  Serial.println("✅ Logger inicializado");
  Serial.print("Archivo: ");
  Serial.println(logger.getFileName());
  Serial.println("\nGrabando 20 variables de tipos mixtos a 10 Hz...\n");
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastWriteTime >= writeInterval) {
    lastWriteTime = currentTime;
    
    // Leer sensores (simulado)
    timestamp = millis();
    contador++;
    
    temperatura1 = 25.5 + random(-50, 50) * 0.1;
    temperatura2 = 26.3 + random(-50, 50) * 0.1;
    temperatura3 = 24.8 + random(-50, 50) * 0.1;
    humedad1 = 60.0 + random(-100, 100) * 0.1;
    humedad2 = 58.5 + random(-100, 100) * 0.1;
    presion = 1013.25 + random(-50, 50) * 0.1;
    
    accelX = random(-2000, 2000) * 0.001;
    accelY = random(-2000, 2000) * 0.001;
    accelZ = 9.81 + random(-100, 100) * 0.001;
    gyroX = random(-500, 500) * 0.001;
    gyroY = random(-500, 500) * 0.001;
    gyroZ = random(-500, 500) * 0.001;
    
    latitud = 4.624335 + random(-1000, 1000) * 0.000001;
    longitud = -74.063644 + random(-1000, 1000) * 0.000001;
    altitud = 2640.5 + random(-100, 100) * 0.1;
    
    voltajeMv = 3300 + random(-200, 200);
    estadoBateria = random(0, 101);
    rpm = random(0, 5000);
    
    // Escribir línea con todos los tipos
    logger.startLine();
    logger.addValue(timestamp, 0);        // unsigned long
    logger.addValue(contador, 0);         // int
    logger.addValue(temperatura1, 2);     // float (2 decimales)
    logger.addValue(temperatura2, 2);     // float
    logger.addValue(temperatura3, 2);     // float
    logger.addValue(humedad1, 2);         // float
    logger.addValue(humedad2, 2);         // float
    logger.addValue(presion, 2);          // float
    logger.addValue(accelX, 4);           // double (4 decimales)
    logger.addValue(accelY, 4);           // double
    logger.addValue(accelZ, 4);           // double
    logger.addValue(gyroX, 4);            // double
    logger.addValue(gyroY, 4);            // double
    logger.addValue(gyroZ, 4);            // double
    logger.addValue(latitud, 6);          // double (6 decimales para GPS)
    logger.addValue(longitud, 6);         // double
    logger.addValue(altitud, 1);          // float (1 decimal)
    logger.addValue(voltajeMv, 0);        // int
    logger.addValue(estadoBateria, 0);    // uint8_t
    logger.addValue(rpm, 0);              // uint16_t
    logger.endLine();
    
    writeCount++;
    
    // Flush cada 20 escrituras
    if (writeCount >= 20) {
      logger.flush();
      writeCount = 0;
      Serial.print(".");
    }
    
    // Mostrar progreso cada 5 segundos
    if (writeCount == 10) {
      Serial.print(" [");
      Serial.print(contador);
      Serial.print("] T:");
      Serial.print(temperatura1, 1);
      Serial.print("°C GPS:");
      Serial.print(latitud, 4);
      Serial.print(",");
      Serial.println(longitud, 4);
    }
  }
}

// ═══════════════════════════════════════════════════════════
// EJEMPLO 4: Modo compacto con macro (opcional)
// ═══════════════════════════════════════════════════════════

void ejemplo4_con_macro() {
  // Definir macro para escribir más fácil
  #define LOG_START() logger.startLine()
  #define LOG_VAL(val, dec) logger.addValue(val, dec)
  #define LOG_END() logger.endLine()
  
  void setup() {
    Serial.begin(115200);
    delay(2000);
    logger.begin(0, 10);
  }
  
  void loop() {
    int temp = 25;
    float hum = 60.5;
    double lat = 4.624335;
    
    LOG_START();
    LOG_VAL(temp, 0);
    LOG_VAL(hum, 2);
    LOG_VAL(lat, 6);
    LOG_END();
    
    delay(100);
  }
}

// ═══════════════════════════════════════════════════════════
// EJEMPLO 5: Optimizado - arrays separados por tipo
// ═══════════════════════════════════════════════════════════

void ejemplo5_arrays_por_tipo() {
  #define NUM_INT 5
  #define NUM_FLOAT 10
  #define NUM_DOUBLE 5
  
  int datosInt[NUM_INT];
  float datosFloat[NUM_FLOAT];
  double datosDouble[NUM_DOUBLE];
  
  void setup() {
    Serial.begin(115200);
    delay(2000);
    logger.begin(0, 10);
  }
  
  void loop() {
    // Leer sensores por tipo
    for (int i = 0; i < NUM_INT; i++) {
      datosInt[i] = random(0, 1000);
    }
    
    for (int i = 0; i < NUM_FLOAT; i++) {
      datosFloat[i] = random(0, 10000) * 0.01;
    }
    
    for (int i = 0; i < NUM_DOUBLE; i++) {
      datosDouble[i] = random(0, 100000) * 0.00001;
    }
    
    // Escribir línea combinando arrays
    logger.startLine();
    
    // Escribir enteros
    for (int i = 0; i < NUM_INT; i++) {
      logger.addValue(datosInt[i], 0);
    }
    
    // Escribir floats
    for (int i = 0; i < NUM_FLOAT; i++) {
      logger.addValue(datosFloat[i], 2);
    }
    
    // Escribir doubles
    for (int i = 0; i < NUM_DOUBLE; i++) {
      logger.addValue(datosDouble[i], 6);
    }
    
    logger.endLine();
    
    delay(100);
  }
}
