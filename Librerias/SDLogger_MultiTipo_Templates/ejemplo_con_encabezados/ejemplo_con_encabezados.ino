/*
 * Ejemplo completo con ENCABEZADOS
 * Muestra cómo escribir encabezados de columnas en el CSV
 */

#include "SDLogger_Template.h"

#define CS_PIN 0

SDLogger logger;

// Variables
unsigned long timestamp;
int satelites;
double latitud, longitud;
float velocidad, altura;
float accelX, accelY;
int calidad;
uint8_t bateria;
float distancia;

unsigned long lastTime = 0;
int writeCount = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  SDLogger con Encabezados             ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // Inicializar logger
  if (!logger.begin(CS_PIN, 10)) {
    Serial.println("❌ Error al inicializar SD");
    while(1) delay(1000);
  }
  
  Serial.println("✅ Logger inicializado");
  Serial.print("Archivo: ");
  Serial.println(logger.getFileName());
  
  // ═══════════════════════════════════════════════════════════
  // ESCRIBIR ENCABEZADOS (primera línea del archivo)
  // ═══════════════════════════════════════════════════════════
  Serial.println("\nEscribiendo encabezados...");
  
  logger.startLine();
  logger.addValue("Hora");
  logger.addValue("Satelites");
  logger.addValue("Latitud");
  logger.addValue("Longitud");
  logger.addValue("Velocidad");
  logger.addValue("Altura");
  logger.addValue("Aceleracion_X");
  logger.addValue("Aceleracion_Y");
  logger.addValue("Calidad");
  logger.addValue("Bateria");
  logger.addValue("Distancia");
  logger.endLine();
  
  logger.flush();  // Asegurar que se escriban los encabezados
  
  Serial.println("✅ Encabezados escritos");
  Serial.println("\nIniciando grabación de datos...\n");
}

void loop() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastTime >= 100) {  // 10 Hz
    lastTime = currentTime;
    
    // Simular lectura de sensores
    timestamp = millis();
    satelites = random(0, 13);
    latitud = 4.624335 + random(-1000, 1000) * 0.000001;
    longitud = -74.063644 + random(-1000, 1000) * 0.000001;
    velocidad = random(0, 1200) * 0.1;  // 0-120 km/h
    altura = 2640.5 + random(-50, 50) * 0.1;
    accelX = random(-2000, 2000) * 0.001;
    accelY = random(-2000, 2000) * 0.001;
    calidad = random(1, 6);
    bateria = random(50, 101);
    distancia = random(0, 10000) * 0.1;
    
    // Escribir línea de datos
    logger.startLine();
    logger.addValue(timestamp, 0);
    logger.addValue(satelites, 0);
    logger.addValue(latitud, 6);
    logger.addValue(longitud, 6);
    logger.addValue(velocidad, 1);
    logger.addValue(altura, 1);
    logger.addValue(accelX, 3);
    logger.addValue(accelY, 3);
    logger.addValue(calidad, 0);
    logger.addValue(bateria, 0);
    logger.addValue(distancia, 1);
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
      Serial.print(satelites);
      Serial.print(" sats] GPS:");
      Serial.print(latitud, 4);
      Serial.print(",");
      Serial.print(longitud, 4);
      Serial.print(" Vel:");
      Serial.print(velocidad, 1);
      Serial.println(" km/h");
    }
  }
}
