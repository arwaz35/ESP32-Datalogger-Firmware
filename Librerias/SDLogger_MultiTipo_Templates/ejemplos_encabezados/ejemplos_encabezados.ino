/*
 * Diferentes formas de escribir encabezados
 */

#include "SDLogger_Template.h"

SDLogger logger;

// ═══════════════════════════════════════════════════════════
// MÉTODO 1: Encabezados uno por uno (como lo hiciste)
// ═══════════════════════════════════════════════════════════
void metodo1_individual() {
  logger.startLine();
  logger.addValue("Hora");
  logger.addValue("Satelites");
  logger.addValue("Latitud");
  logger.addValue("Longitud");
  logger.addValue("Velocidad");
  logger.endLine();
  logger.flush();
}

// ═══════════════════════════════════════════════════════════
// MÉTODO 2: Array de Strings
// ═══════════════════════════════════════════════════════════
void metodo2_array() {
  String encabezados[] = {
    "Hora", "Satelites", "Latitud", "Longitud", "Velocidad",
    "Altura", "AccelX", "AccelY", "Calidad", "Bateria"
  };
  
  logger.startLine();
  for (int i = 0; i < 10; i++) {
    logger.addValue(encabezados[i]);
  }
  logger.endLine();
  logger.flush();
}

// ═══════════════════════════════════════════════════════════
// MÉTODO 3: Con const char* (ahorra memoria)
// ═══════════════════════════════════════════════════════════
void metodo3_const_char() {
  const char* encabezados[] = {
    "Hora", "Satelites", "Latitud", "Longitud", "Velocidad",
    "Altura", "AccelX", "AccelY", "Calidad", "Bateria"
  };
  
  logger.startLine();
  for (int i = 0; i < 10; i++) {
    logger.addValue(encabezados[i]);
  }
  logger.endLine();
  logger.flush();
}

// ═══════════════════════════════════════════════════════════
// MÉTODO 4: Función helper personalizada
// ═══════════════════════════════════════════════════════════
void escribirEncabezados() {
  logger.startLine();
  logger.addValue("Timestamp");
  logger.addValue("GPS_Sats");
  logger.addValue("GPS_Lat");
  logger.addValue("GPS_Lon");
  logger.addValue("Vel_kmh");
  logger.addValue("Alt_m");
  logger.addValue("Acc_X_g");
  logger.addValue("Acc_Y_g");
  logger.addValue("Signal_Quality");
  logger.addValue("Battery_pct");
  logger.addValue("Distance_km");
  logger.endLine();
  logger.flush();
}

// ═══════════════════════════════════════════════════════════
// MÉTODO 5: Macro para escribir más fácil
// ═══════════════════════════════════════════════════════════
#define WRITE_HEADER(...) \
  do { \
    const char* headers[] = {__VA_ARGS__}; \
    logger.startLine(); \
    for (unsigned int i = 0; i < sizeof(headers)/sizeof(headers[0]); i++) { \
      logger.addValue(headers[i]); \
    } \
    logger.endLine(); \
    logger.flush(); \
  } while(0)

void metodo5_con_macro() {
  WRITE_HEADER("Hora", "Satelites", "Latitud", "Longitud", "Velocidad");
}

// ═══════════════════════════════════════════════════════════
// EJEMPLO COMPLETO - Mejor práctica
// ═══════════════════════════════════════════════════════════

#include "SDLogger_Template.h"

#define CS_PIN 0
#define NUM_VARIABLES 11

SDLogger logger;

// Definir encabezados como constantes (ahorra RAM)
const char* ENCABEZADOS[NUM_VARIABLES] = {
  "Timestamp_ms",
  "GPS_Satellites",
  "GPS_Latitude",
  "GPS_Longitude",
  "Speed_kmh",
  "Altitude_m",
  "Accel_X_g",
  "Accel_Y_g",
  "Signal_Quality",
  "Battery_percent",
  "Distance_km"
};

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  if (!logger.begin(CS_PIN, 10)) {
    Serial.println("Error SD");
    while(1);
  }
  
  Serial.println("Escribiendo encabezados...");
  
  // Escribir encabezados usando el array
  logger.startLine();
  for (int i = 0; i < NUM_VARIABLES; i++) {
    logger.addValue(ENCABEZADOS[i]);
  }
  logger.endLine();
  logger.flush();
  
  Serial.println("Encabezados escritos!");
  Serial.println("Iniciando grabación...");
}

void loop() {
  // Tu código de grabación aquí
  
  // Ejemplo:
  unsigned long timestamp = millis();
  int satelites = 8;
  double lat = 4.624335;
  double lon = -74.063644;
  float vel = 45.2;
  float alt = 2640.5;
  float ax = 0.05;
  float ay = -0.02;
  int calidad = 4;
  uint8_t bat = 85;
  float dist = 12.5;
  
  logger.startLine();
  logger.addValue(timestamp, 0);
  logger.addValue(satelites, 0);
  logger.addValue(lat, 6);
  logger.addValue(lon, 6);
  logger.addValue(vel, 1);
  logger.addValue(alt, 1);
  logger.addValue(ax, 3);
  logger.addValue(ay, 3);
  logger.addValue(calidad, 0);
  logger.addValue(bat, 0);
  logger.addValue(dist, 1);
  logger.endLine();
  
  static int count = 0;
  if (++count >= 20) {
    logger.flush();
    count = 0;
  }
  
  delay(100);
}

// ═══════════════════════════════════════════════════════════
// CONSEJOS Y MEJORES PRÁCTICAS
// ═══════════════════════════════════════════════════════════

/*
 * 1. EVITAR ESPACIOS EN ENCABEZADOS
 *    ❌ "Aceleración X"  → Problemas al importar en Excel/Python
 *    ✅ "Aceleracion_X"  → Fácil de procesar
 * 
 * 2. USAR GUIÓN BAJO EN LUGAR DE ESPACIOS
 *    ✅ "GPS_Latitude", "Speed_kmh", "Battery_percent"
 * 
 * 3. INCLUIR UNIDADES EN EL NOMBRE
 *    ✅ "Temperatura_C", "Velocidad_kmh", "Altura_m"
 * 
 * 4. SER CONSISTENTE
 *    ✅ CamelCase: "GpsLatitude", "SpeedKmh"
 *    ✅ snake_case: "gps_latitude", "speed_kmh"
 *    ✅ UPPER_CASE: "GPS_LATITUDE", "SPEED_KMH"
 * 
 * 5. NOMBRES DESCRIPTIVOS PERO CORTOS
 *    ❌ "Valor_de_temperatura_ambiente_en_grados_celsius"
 *    ✅ "Temp_C"
 * 
 * 6. ESCRIBIR ENCABEZADOS SOLO UNA VEZ
 *    - En setup(), NO en loop()
 *    - Hacer flush() inmediatamente después
 * 
 * 7. USAR ARRAYS PARA MUCHOS ENCABEZADOS
 *    - Más fácil de mantener
 *    - Menos código repetitivo
 */
