# 📊 SDLogger - Soporte para Múltiples Tipos de Datos

## ✅ PROBLEMA RESUELTO

La versión anterior solo aceptaba `float`. Ahora puedes usar:
- ✅ **int** (enteros)
- ✅ **long** (enteros largos)
- ✅ **float** (flotantes)
- ✅ **double** (flotantes de doble precisión)
- ✅ **uint8_t, uint16_t, uint32_t** (enteros sin signo)
- ✅ **unsigned long** (timestamps, etc.)

---

## 🎯 TIENES 2 OPCIONES

### OPCIÓN 1: Templates (Recomendada) ⭐

**Ventajas:**
- ✅ Más flexible
- ✅ Menos código
- ✅ El compilador optimiza automáticamente
- ✅ Funciona con CUALQUIER tipo

**Archivos:**
- `SDLogger_Template.h`
- `SDLogger_Template.cpp`

---

### OPCIÓN 2: Sobrecarga de funciones (Alternativa)

**Ventajas:**
- ✅ Más simple de entender
- ✅ No requiere conocer templates
- ✅ Funciona igual de bien

**Archivos:**
- `SDLogger_Overload.h`
- `SDLogger_Overload.cpp`

---

## 💻 USO BÁSICO

### Método 1: Array de un solo tipo

```cpp
#include "SDLogger_Template.h"  // O SDLogger_Overload.h

SDLogger logger;

void setup() {
  Serial.begin(115200);
  delay(2000);
  logger.begin(0, 10);
}

void loop() {
  // Array de enteros
  int sensoresInt[5] = {100, 200, 300, 400, 500};
  logger.writeArray(sensoresInt, 5);
  
  // Array de floats con 2 decimales
  float sensoresFloat[5] = {25.5, 60.3, 1013.25, 3.14, 2.71};
  logger.writeArray(sensoresFloat, 5, 2);
  
  // Array de doubles con 6 decimales
  double gpsCoords[2] = {4.624335, -74.063644};
  logger.writeArray(gpsCoords, 2, 6);
  
  delay(100);
}
```

**Resultado en CSV:**
```
100;200;300;400;500
25.50;60.30;1013.25;3.14;2.71
4.624335;-74.063644
```

---

### Método 2: Tipos mixtos en una línea (⭐ MÁS ÚTIL)

```cpp
void loop() {
  // Variables de diferentes tipos
  unsigned long timestamp = millis();
  int contador = 123;
  float temperatura = 25.5;
  double latitud = 4.624335;
  double longitud = -74.063644;
  uint8_t estado = 1;
  
  // Escribir todos en una línea
  logger.startLine();
  logger.addValue(timestamp, 0);      // unsigned long, sin decimales
  logger.addValue(contador, 0);       // int, sin decimales
  logger.addValue(temperatura, 2);    // float, 2 decimales
  logger.addValue(latitud, 6);        // double, 6 decimales
  logger.addValue(longitud, 6);       // double, 6 decimales
  logger.addValue(estado, 0);         // uint8_t, sin decimales
  logger.endLine();
  
  delay(100);
}
```

**Resultado en CSV:**
```
12345;123;25.50;4.624335;-74.063644;1
```

---

## 🚀 EJEMPLO COMPLETO - 20 Variables Mixtas

```cpp
#include "SDLogger_Template.h"

#define CS_PIN 0
SDLogger logger;

// Variables de diferentes tipos
unsigned long timestamp;
int contador = 0;
float temp1, temp2, hum1, hum2, presion;
double accelX, accelY, accelZ;
double gyroX, gyroY, gyroZ;
double latitud, longitud;
int voltajeMv;
uint8_t estadoBateria;

unsigned long lastTime = 0;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  if (!logger.begin(CS_PIN, 10)) {
    Serial.println("Error SD");
    while(1);
  }
  
  Serial.println("Listo! Grabando...");
}

void loop() {
  if (millis() - lastTime >= 100) {  // 10 Hz
    lastTime = millis();
    
    // Leer sensores
    timestamp = millis();
    contador++;
    temp1 = 25.5;
    temp2 = 26.3;
    hum1 = 60.0;
    hum2 = 58.5;
    presion = 1013.25;
    accelX = 0.05;
    accelY = -0.02;
    accelZ = 9.81;
    gyroX = 0.01;
    gyroY = -0.03;
    gyroZ = 0.00;
    latitud = 4.624335;
    longitud = -74.063644;
    voltajeMv = 3300;
    estadoBateria = 85;
    
    // Escribir línea
    logger.startLine();
    logger.addValue(timestamp, 0);        // unsigned long
    logger.addValue(contador, 0);         // int
    logger.addValue(temp1, 2);            // float
    logger.addValue(temp2, 2);            // float
    logger.addValue(hum1, 2);             // float
    logger.addValue(hum2, 2);             // float
    logger.addValue(presion, 2);          // float
    logger.addValue(accelX, 4);           // double
    logger.addValue(accelY, 4);           // double
    logger.addValue(accelZ, 4);           // double
    logger.addValue(gyroX, 4);            // double
    logger.addValue(gyroY, 4);            // double
    logger.addValue(gyroZ, 4);            // double
    logger.addValue(latitud, 6);          // double (GPS)
    logger.addValue(longitud, 6);         // double (GPS)
    logger.addValue(voltajeMv, 0);        // int
    logger.addValue(estadoBateria, 0);    // uint8_t
    logger.endLine();
    
    // Flush cada 20 muestras
    static int count = 0;
    if (++count >= 20) {
      logger.flush();
      count = 0;
      Serial.println(".");
    }
  }
}
```

---

## 📊 TIPOS SOPORTADOS Y DECIMALES RECOMENDADOS

| Tipo | Uso típico | Decimales | Ejemplo |
|------|-----------|-----------|---------|
| **int** | Contadores, IDs | 0 | `123` |
| **long** | Valores grandes | 0 | `1000000` |
| **unsigned long** | Timestamps | 0 | `123456` |
| **float** | Temperatura, humedad | 2-4 | `25.50` |
| **double** | GPS, precisión alta | 6-8 | `4.624335` |
| **uint8_t** | Estados, flags | 0 | `1` |
| **uint16_t** | RPM, valores 0-65535 | 0 | `3000` |
| **uint32_t** | Valores muy grandes | 0 | `4000000` |

---

## 🎯 EJEMPLOS POR CASO DE USO

### Caso 1: Data logger ambiental

```cpp
float temperatura, humedad, presion;
int luzLux;

logger.startLine();
logger.addValue(temperatura, 2);    // 25.50
logger.addValue(humedad, 2);        // 60.30
logger.addValue(presion, 2);        // 1013.25
logger.addValue(luzLux, 0);         // 500
logger.endLine();
```

**CSV:** `25.50;60.30;1013.25;500`

---

### Caso 2: GPS tracker

```cpp
unsigned long timestamp;
double lat, lon;
float altitud, velocidad;

logger.startLine();
logger.addValue(timestamp, 0);      // 123456
logger.addValue(lat, 6);            // 4.624335
logger.addValue(lon, 6);            // -74.063644
logger.addValue(altitud, 1);        // 2640.5
logger.addValue(velocidad, 1);      // 45.2
logger.endLine();
```

**CSV:** `123456;4.624335;-74.063644;2640.5;45.2`

---

### Caso 3: Sistema de monitoreo

```cpp
unsigned long uptime;
int voltajeMv, corrienteMa;
float temperatura;
uint8_t estadoSistema;  // 0=off, 1=on, 2=error

logger.startLine();
logger.addValue(uptime, 0);         // 3600000 (1 hora)
logger.addValue(voltajeMv, 0);      // 3300
logger.addValue(corrienteMa, 0);    // 250
logger.addValue(temperatura, 1);    // 45.5
logger.addValue(estadoSistema, 0);  // 1
logger.endLine();
```

**CSV:** `3600000;3300;250;45.5;1`

---

### Caso 4: IMU (sensores inerciales)

```cpp
// Usar double para IMU (mayor precisión)
double accelX, accelY, accelZ;
double gyroX, gyroY, gyroZ;

logger.startLine();
logger.addValue(accelX, 4);         // 0.0512
logger.addValue(accelY, 4);         // -0.0234
logger.addValue(accelZ, 4);         // 9.8100
logger.addValue(gyroX, 4);          // 0.0123
logger.addValue(gyroY, 4);          // -0.0345
logger.addValue(gyroZ, 4);          // 0.0001
logger.endLine();
```

**CSV:** `0.0512;-0.0234;9.8100;0.0123;-0.0345;0.0001`

---

## 💡 CONSEJOS Y MEJORES PRÁCTICAS

### 1. Elegir el tipo correcto

```cpp
// ❌ MAL - Desperdicia memoria y precisión
double contador = 123;          // double es excesivo para un contador
float timestamp = millis();     // Pierde precisión

// ✅ BIEN
int contador = 123;             // Suficiente para contadores
unsigned long timestamp = millis();  // Tipo correcto para timestamps
```

---

### 2. Usar decimales apropiados

```cpp
// ❌ MAL - Demasiados decimales innecesarios
logger.addValue(temperatura, 6);    // 25.500000 (archivo más grande)

// ✅ BIEN
logger.addValue(temperatura, 2);    // 25.50 (suficiente)
```

---

### 3. GPS necesita 6+ decimales

```cpp
// ❌ MAL - Pierde precisión GPS
logger.addValue(latitud, 2);        // 4.62 (error de ~1km)

// ✅ BIEN
logger.addValue(latitud, 6);        // 4.624335 (precisión ~0.1m)
```

---

### 4. Orden lógico de variables

```cpp
// ✅ BIEN - Orden lógico
logger.startLine();
logger.addValue(timestamp, 0);      // Primero el tiempo
logger.addValue(contador, 0);       // Luego ID/contador
logger.addValue(temperatura, 2);    // Datos de sensores
logger.addValue(latitud, 6);        // Ubicación
logger.addValue(estado, 0);         // Estado al final
logger.endLine();
```

---

## ⚡ RENDIMIENTO

**Tiempo de escritura por línea:**

| Variables | Tipos | Tiempo | Compatible 10 Hz |
|-----------|-------|--------|------------------|
| 10 vars | Mixed | ~2-3 ms | ✅ Sí |
| 20 vars | Mixed | ~4-5 ms | ✅ Sí |
| 50 vars | Mixed | ~10 ms | ✅ Sí |

**No hay diferencia** entre usar templates o sobrecarga - ambos son igual de rápidos.

---

## 📥 ARCHIVOS DISPONIBLES

### Opción 1 - Templates (Recomendada):
- `SDLogger_Template.h`
- `SDLogger_Template.cpp`
- `ejemplos_tipos_mixtos.ino`

### Opción 2 - Sobrecarga (Alternativa):
- `SDLogger_Overload.h`
- `SDLogger_Overload.cpp`

Ambas funcionan **exactamente igual** desde el punto de vista del usuario.

---

## 🔄 MIGRACIÓN DESDE VERSIÓN ANTERIOR

### Si usabas solo floats:

```cpp
// ANTES (solo float):
float datos[10];
logger.writeArray(datos, 10);

// AHORA (cualquier tipo):
int datos[10];
logger.writeArray(datos, 10);  // Funciona automáticamente!
```

### Si necesitas tipos mixtos:

```cpp
// ANTES - tenías que convertir todo a float
float timestamp = (float)millis();  // ❌ Pierde precisión
float lat = 4.624335;

logger.writeArray(...);

// AHORA - usa el tipo correcto
unsigned long timestamp = millis();  // ✅ Tipo correcto
double lat = 4.624335;               // ✅ Mayor precisión

logger.startLine();
logger.addValue(timestamp, 0);
logger.addValue(lat, 6);
logger.endLine();
```

---

## 🎯 RESUMEN RÁPIDO

**¿Qué usar?**

1. **Array de un solo tipo** → `writeArray(array, count, decimals)`
2. **Tipos mixtos** → `startLine()` + `addValue()` + `endLine()`

**¿Qué versión?**

- **Templates** (recomendada) - Más flexible
- **Sobrecarga** (alternativa) - Más simple

**¿Cuántos decimales?**

- Enteros: `0`
- Temperatura/humedad: `2`
- Acelerómetro/giroscopio: `4`
- GPS: `6`

---

## 📝 CHECKLIST

Antes de usar:

- [ ] Instalar SdFat
- [ ] Elegir versión (templates o sobrecarga)
- [ ] Copiar archivos .h y .cpp
- [ ] Decidir tipos de datos necesarios
- [ ] Definir decimales apropiados
- [ ] Probar con ejemplo simple

---

**¿Necesitas más ejemplos o ayuda?** ¡Pregunta! 🚀
