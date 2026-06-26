/*
 * Ejemplo de uso de la librería VBOXSport
 * 
 * Instrucciones de instalación:
 * 1. Crear carpeta: Arduino/libraries/VBOXSport/
 * 2. Copiar VBOXSport.h y VBOXSport.cpp en esa carpeta
 * 3. Reiniciar Arduino IDE
 * 4. Abrir este ejemplo
 */

#include <VBOXSport.h>

// Crear instancia de VBOX
VBOXSport vbox;

// Configuración - Cambiar por el nombre de tu VBOX
const char* VBOX_NAME = "VBSport 07019094";

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println("\n=== VBOX Sport Example ===\n");
  
  // Conectar al VBOX
  Serial.print("Conectando a ");
  Serial.println(VBOX_NAME);
  
  if (vbox.begin(VBOX_NAME)) {
    Serial.println("¡Conectado exitosamente!");
    
    // Resetear distancia al inicio
    vbox.resetDistance();
    Serial.println("Distancia reseteada a 0");
    
    // Configurar velocidad mínima (opcional, default es 0.8 km/h)
    vbox.setMinSpeedKmh(0.8);  // No contar distancia si velocidad < 0.8 km/h
  } else {
    Serial.println("Error de conexión");
  }
}

void loop() {
  // IMPORTANTE: Actualizar datos del VBOX
  vbox.update();
  
  // Verificar conexión
  if (!vbox.isConnected()) {
    Serial.println("Desconectado...");
    delay(1000);
    return;
  }
  
  // ===== EJEMPLO 1: Mostrar todos los datos formateados =====
  // vbox.printData();
  
  // ===== EJEMPLO 2: Acceso individual a datos =====
  
  // GPS básico
  Serial.print("Satélites: ");
  Serial.print(vbox.satellites());
  if (vbox.isDGPS()) Serial.print(" (DGPS)");
  Serial.println();
  
  // Posición
  if (vbox.hasPosition()) {
    Serial.print("Lat: ");
    Serial.print(vbox.latitude(), 7);
    Serial.print("° | Lon: ");
    Serial.print(vbox.longitude(), 7);
    Serial.println("°");
  }
  
  // Velocidad en diferentes unidades
  if (vbox.hasSpeed()) {
    Serial.print("Velocidad: ");
    Serial.print(vbox.speedKmh(), 2);
    Serial.print(" km/h | ");
    Serial.print(vbox.speedMph(), 2);
    Serial.print(" mph | ");
    Serial.print(vbox.speedKnots(), 2);
    Serial.println(" nudos");
  }
  
  // Rumbo
  if (vbox.hasHeading()) {
    Serial.print("Rumbo: ");
    Serial.print(vbox.heading(), 1);
    Serial.println("°");
  }
  
  // Altura
  if (vbox.hasHeight()) {
    Serial.print("Altura: ");
    Serial.print(vbox.heightM(), 2);
    Serial.print(" m (");
    Serial.print(vbox.heightFt(), 1);
    Serial.println(" ft)");
  }
  
  // Aceleraciones (útil para análisis de manejo deportivo)
  if (vbox.hasAcceleration()) {
    Serial.print("Aceleración - Long: ");
    Serial.print(vbox.accelLongG(), 3);
    Serial.print("G | Lat: ");
    Serial.print(vbox.accelLatG(), 3);
    Serial.println("G");
  }
  
  // Calidad GPS
  if (vbox.hasHDOP()) {
    Serial.print("HDOP: ");
    Serial.print(vbox.hdop(), 2);
    Serial.print(" (");
    Serial.print(vbox.hdopQuality());
    Serial.println(")");
  }
  
  // Batería
  if (vbox.hasBattery()) {
    Serial.print("Batería: ");
    if (vbox.isBatteryCharging()) {
      Serial.println("Cargando");
    } else {
      Serial.print(vbox.batteryMinutes());
      Serial.print(" minutos restantes");
      if (vbox.hasBatteryPercent()) {
        Serial.print(" (");
        Serial.print(vbox.batteryPercent());
        Serial.print("%)");
      }
      Serial.println();
    }
  }
  
  // Almacenamiento
  if (vbox.hasStorage()) {
    Serial.print("Almacenamiento: ");
    Serial.print(vbox.storageFreeMB(), 2);
    Serial.print(" MB libres (");
    Serial.print(100.0 - vbox.storageUsedPercent(), 1);
    Serial.println("% libre)");
  }
  
  // Distancia recorrida
  Serial.print("Distancia: ");
  Serial.print(vbox.distanceMeters(), 2);
  Serial.print(" m (");
  Serial.print(vbox.distanceKm(), 3);
  Serial.println(" km)");
  
  // Tiempo UTC
  if (vbox.hasTime()) {
    uint8_t h, m, s;
    uint16_t ms;
    vbox.getTime(h, m, s, ms);
    Serial.print("Hora UTC: ");
    if (h < 10) Serial.print("0");
    Serial.print(h);
    Serial.print(":");
    if (m < 10) Serial.print("0");
    Serial.print(m);
    Serial.print(":");
    if (s < 10) Serial.print("0");
    Serial.println(s);
    
    // Hora local de Bogotá (UTC-5)
    vbox.getLocalTime(h, m, s);  // Por defecto UTC-5
    Serial.print("Hora Bogotá: ");
    if (h < 10) Serial.print("0");
    Serial.print(h);
    Serial.print(":");
    if (m < 10) Serial.print("0");
    Serial.print(m);
    Serial.print(":");
    if (s < 10) Serial.print("0");
    Serial.println(s);
  }
  
  Serial.println("---");
  
  delay(1000);  // Actualizar cada segundo
}

// ===== EJEMPLO 3: Función para detectar frenadas fuertes =====
void detectHardBraking() {
  if (vbox.hasAcceleration()) {
    float longG = vbox.accelLongG();
    
    // Detectar frenada fuerte (desaceleración > 0.5G)
    if (longG < -0.5) {
      Serial.println("¡FRENADA FUERTE DETECTADA!");
      Serial.print("Desaceleración: ");
      Serial.print(-longG, 2);
      Serial.println("G");
    }
  }
}

// ===== EJEMPLO 4: Función para detectar curvas cerradas =====
void detectSharpTurn() {
  if (vbox.hasAcceleration()) {
    float latG = abs(vbox.accelLatG());
    
    // Detectar curva cerrada (aceleración lateral > 0.4G)
    if (latG > 0.4) {
      Serial.println("Curva cerrada");
      Serial.print("Aceleración lateral: ");
      Serial.print(latG, 2);
      Serial.println("G");
    }
  }
}

// ===== EJEMPLO 5: Calcular distancia entre dos puntos =====
double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
  // Fórmula de Haversine
  const double R = 6371000; // Radio de la Tierra en metros
  
  double dLat = (lat2 - lat1) * DEG_TO_RAD;
  double dLon = (lon2 - lon1) * DEG_TO_RAD;
  
  double a = sin(dLat/2) * sin(dLat/2) +
             cos(lat1 * DEG_TO_RAD) * cos(lat2 * DEG_TO_RAD) *
             sin(dLon/2) * sin(dLon/2);
  
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  
  return R * c; // Distancia en metros
}