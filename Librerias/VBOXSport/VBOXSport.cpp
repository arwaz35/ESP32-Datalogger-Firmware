/*
 * VBOXSport.cpp
 * Versión 2.0 - Sistema adaptativo
 */

#include "VBOXSport.h"

#define F_SATS      0x00000001
#define F_TIME      0x00000002
#define F_LAT       0x00000004
#define F_LON       0x00000008
#define F_SPEED     0x00000010
#define F_HEADING   0x00000020
#define F_HEIGHT    0x00000040
#define F_VSPEED    0x00000080
#define F_AXLON     0x00000100
#define F_AXLAT     0x00000200

#define FX_BATT_TTE      0x00000001
#define FX_BATT_TTF      0x00000002
#define FX_BATT_CAP      0x00000004
#define FX_BATT_PERCENT  0x00000008
#define FX_MEDIA_CAP     0x00000010
#define FX_MEDIA_FREE    0x00000020
#define FX_HDOP          0x00000040

static const uint8_t HDR[7] = {'$', 'V', 'B', 'S', 'P', 'T', '$'};

VBOXSport::VBOXSport() {
  _bufferPos = 0;
  _stdFlags = 0;
  _extFlags = 0;
  _satellites = 0;
  _isDGPS = false;
  _timeMs = 0;
  _latitude = 0;
  _longitude = 0;
  _speedKmh = 0;
  _speedKnots = 0;
  _heading = 0;
  _heightM = 0;
  _verticalSpeedMs = 0;
  _accelLongG = 0;
  _accelLatG = 0;
  _batteryMinutes = 0;
  _batteryPercent = 0;
  _mediaCapacityKB = 0;
  _mediaFreeKB = 0;
  _hdop = 0;
  _hasTime = false;
  _hasLat = false;
  _hasLon = false;
  _hasSpeed = false;
  _hasHeading = false;
  _hasHeight = false;
  _hasVSpeed = false;
  _hasAccelLon = false;
  _hasAccelLat = false;
  _hasBattery = false;
  _hasBatteryPercent = false;
  _hasMediaCap = false;
  _hasMediaFree = false;
  _hasHDOP = false;
  _lastReconnect = 0;
  
  // Inicializar sistema de distancia
  _totalDistance = 0.0;
  _lastUpdateTime = 0;
  _distanceInitialized = false;
  _distanceInitialized = false;
  _minSpeedKmh = 0.8;  // Velocidad mínima por defecto
  _speedClampKmh = 0.0; // Clamp desactivado por defecto
  _useMac = false;
  memset(_targetMac, 0, 6);
}

bool VBOXSport::begin(const char* vboxName) {
  _useMac = false;
  return connectVBOX(vboxName);
}

bool VBOXSport::begin(uint8_t* macAddress) {
  _useMac = true;
  memcpy(_targetMac, macAddress, 6);
  return connectVBOX(macAddress);
}

bool VBOXSport::connectVBOX(const char* name) {
  _vboxName = String(name); // Guardar nombre para autoreconexión
  if (!_bt.begin("ESP32_VBOX", true)) {
    return false;
  }
  
  bool ok = false;
  uint32_t t0 = millis();
  
  // Intentar conectar durante 15 segundos
  while (!ok && (millis() - t0) < 15000) {
    if (_bt.connect(name)) {
      ok = true;
      Serial.println("VBOX conectado!");
    } else {
      Serial.println("Fallo al conectar VBOX, reintentando en 1s...");
      delay(1000);
    }
  }
  
  return ok;
}

bool VBOXSport::connectVBOX(uint8_t* macAddress) {
  if (!_bt.begin("ESP32_VBOX", true)) {
    return false;
  }
  
  bool ok = false;
  uint32_t t0 = millis();
  
  // Intentar conectar durante 15 segundos
  while (!ok && (millis() - t0) < 15000) {
    if (_bt.connect(macAddress)) {
      ok = true;
      Serial.println("VBOX conectado por MAC!");
    } else {
      Serial.println("Fallo al conectar VBOX (MAC), reintentando en 1s...");
      delay(1000);
    }
  }
  
  return ok;
}

void VBOXSport::disconnect() {
  _bt.disconnect();
  _bt.end();
}

bool VBOXSport::isConnected() {
  return _bt.connected();
}

void VBOXSport::update() {
  if (!_bt.connected()) {
    if (millis() - _lastReconnect > 3000) {
      _lastReconnect = millis();
      if (_useMac) {
         Serial.println("Intentando reconectar VBOX (MAC)...");
         if (_bt.connect(_targetMac)) {
           Serial.println("Reconectado exitosamente!");
         }
      } else if (_vboxName.length() > 0) {
        Serial.println("Intentando reconectar VBOX...");
        if (_bt.connect(_vboxName.c_str())) {
          Serial.println("Reconectado exitosamente!");
        }
      }
    }
    return;
  }
  
  size_t available = _bt.available();
  
  if (available < 10) {
    delay(5);
    available = _bt.available();
    if (available < 10) return;
  }
  
  if (available < 120) {
    // MODO RAPIDO: procesar primer mensaje
    _bufferPos = 0;
    while (_bt.available() && _bufferPos < BUFFER_SIZE) {
      uint8_t b = _bt.read();
      
      if (_bufferPos < 7) {
        if (b == HDR[_bufferPos]) {
          _buffer[_bufferPos++] = b;
        } else {
          if (b == '$') {
            _bufferPos = 1;
            _buffer[0] = '$';
          } else {
            _bufferPos = 0;
          }
        }
      } else {
        _buffer[_bufferPos++] = b;
        if (_bufferPos >= 20 && tryParse()) {
          return;
        }
      }
    }
  } else {
    // MODO SALTO: buscar ultimo mensaje
    uint8_t tempBuf[600];
    size_t totalBytes = 0;
    while (_bt.available() && totalBytes < 600) {
      tempBuf[totalBytes++] = _bt.read();
    }
    
    int lastMsgStart = -1;
    for (int i = (int)totalBytes - 60; i >= 0; i--) {
      bool isHeader = true;
      for (int j = 0; j < 7; j++) {
        if (i + j >= (int)totalBytes || tempBuf[i + j] != HDR[j]) {
          isHeader = false;
          break;
        }
      }
      if (isHeader && i + 7 < (int)totalBytes && tempBuf[i + 7] == ',') {
        lastMsgStart = i;
        break;
      }
    }
    
    if (lastMsgStart >= 0) {
      _bufferPos = 0;
      for (size_t i = lastMsgStart; i < totalBytes && _bufferPos < BUFFER_SIZE; i++) {
        _buffer[_bufferPos++] = tempBuf[i];
      }
      
      if (_bufferPos >= 20) {
        tryParse();
      }
    }
  }
}

bool VBOXSport::tryParse() {
  if (_bufferPos < 17) return false;
  if (_buffer[7] != ',') return false;
  
  uint32_t fStd = ((uint32_t)_buffer[8] << 24) | ((uint32_t)_buffer[9] << 16) |
                  ((uint32_t)_buffer[10] << 8) | _buffer[11];
  uint32_t fExt = ((uint32_t)_buffer[12] << 24) | ((uint32_t)_buffer[13] << 16) |
                  ((uint32_t)_buffer[14] << 8) | _buffer[15];
  
  if (_buffer[16] != ',') return false;
  
  size_t payLen = payloadLenFromFlags(fStd, fExt);
  size_t totalLen = 7 + 1 + 8 + 1 + payLen + 2;
  
  if (_bufferPos < totalLen) return false;
  
  _stdFlags = fStd;
  _extFlags = fExt;
  
  _hasTime = _hasLat = _hasLon = _hasSpeed = _hasHeading = false;
  _hasHeight = _hasVSpeed = _hasAccelLon = _hasAccelLat = false;
  _hasBattery = _hasBatteryPercent = _hasMediaCap = _hasMediaFree = _hasHDOP = false;
  
  size_t pos = 17;
  
  // Guardar velocidad anterior para cálculo de distancia
  double prevSpeedMs = _speedKmh / 3.6;  // Convertir km/h a m/s
  
  if (fStd & F_SATS) {
    _satellites = _buffer[pos] & 0x7F;
    _isDGPS = (_buffer[pos] & 0x80) != 0;
    pos += 1;
  }
  
  if (fStd & F_TIME) {
    uint32_t t = ((uint32_t)_buffer[pos] << 16) | ((uint32_t)_buffer[pos+1] << 8) | _buffer[pos+2];
    _timeMs = t * 10;
    _hasTime = true;
    pos += 3;
  }
  
  if (fStd & F_LAT) {
    int32_t lat = ((int32_t)_buffer[pos] << 24) | ((uint32_t)_buffer[pos+1] << 16) |
                  ((uint32_t)_buffer[pos+2] << 8) | _buffer[pos+3];
    _latitude = minutes100k_to_deg(lat);
    _hasLat = true;
    pos += 4;
  }
  
  if (fStd & F_LON) {
    int32_t lon = ((int32_t)_buffer[pos] << 24) | ((uint32_t)_buffer[pos+1] << 16) |
                  ((uint32_t)_buffer[pos+2] << 8) | _buffer[pos+3];
    _longitude = -minutes100k_to_deg(lon);
    _hasLon = true;
    pos += 4;
  }
  
  if (fStd & F_SPEED) {
    uint16_t spd = ((uint16_t)_buffer[pos] << 8) | _buffer[pos+1];
    _speedKnots = spd / 100.0;
    _speedKmh = _speedKnots * 1.852;
    _hasSpeed = true;
    pos += 2;
  }
  
  if (fStd & F_HEADING) {
    uint16_t hdg = ((uint16_t)_buffer[pos] << 8) | _buffer[pos+1];
    _heading = hdg / 100.0;
    _hasHeading = true;
    pos += 2;
  }
  
  if (fStd & F_HEIGHT) {
    uint32_t h = ((uint32_t)_buffer[pos] << 16) | ((uint32_t)_buffer[pos+1] << 8) | _buffer[pos+2];
    if (h & 0x800000) h |= 0xFF000000;
    _heightM = ((int32_t)h) / 100.0;
    _hasHeight = true;
    pos += 3;
  }
  
  if (fStd & F_VSPEED) {
    int16_t vs = ((int16_t)_buffer[pos] << 8) | _buffer[pos+1];
    _verticalSpeedMs = vs / 100.0;
    _hasVSpeed = true;
    pos += 2;
  }
  
  if (fStd & F_AXLON) {
    int16_t ax = ((int16_t)_buffer[pos] << 8) | _buffer[pos+1];
    _accelLongG = ax / 100.0;
    _hasAccelLon = true;
    pos += 2;
  }
  
  if (fStd & F_AXLAT) {
    int16_t ay = ((int16_t)_buffer[pos] << 8) | _buffer[pos+1];
    _accelLatG = ay / 100.0;
    _hasAccelLat = true;
    pos += 2;
  }
  
  if (fExt & FX_BATT_TTE) {
    _batteryMinutes = ((uint16_t)_buffer[pos] << 8) | _buffer[pos+1];
    _hasBattery = true;
    pos += 2;
  }
  
  if (fExt & FX_BATT_TTF) {
    pos += 2;
  }
  
  if (fExt & FX_BATT_CAP) {
    pos += 2;
  }
  
  if (fExt & FX_BATT_PERCENT) {
    _batteryPercent = _buffer[pos+1];
    _hasBatteryPercent = true;
    pos += 2;
  }
  
  if (fExt & FX_MEDIA_CAP) {
    _mediaCapacityKB = ((uint32_t)_buffer[pos] << 24) | ((uint32_t)_buffer[pos+1] << 16) |
                       ((uint32_t)_buffer[pos+2] << 8) | _buffer[pos+3];
    _hasMediaCap = true;
    pos += 4;
  }
  
  if (fExt & FX_MEDIA_FREE) {
    _mediaFreeKB = ((uint32_t)_buffer[pos] << 24) | ((uint32_t)_buffer[pos+1] << 16) |
                   ((uint32_t)_buffer[pos+2] << 8) | _buffer[pos+3];
    _hasMediaFree = true;
    pos += 4;
  }
  
  if (fExt & FX_HDOP) {
    uint16_t h = ((uint16_t)_buffer[pos] << 8) | _buffer[pos+1];
    _hdop = h / 100.0;
    _hasHDOP = true;
    pos += 2;
  }
  
  // CALCULAR DISTANCIA usando velocidad
  if (_hasSpeed) {
    uint32_t currentTime = millis();
    
    if (_distanceInitialized) {
      // Calcular tiempo transcurrido en segundos
      double deltaTimeS = (currentTime - _lastUpdateTime) / 1000.0;
      
      // Evitar saltos si hay mucho tiempo entre actualizaciones (> 2 segundos)
      if (deltaTimeS > 0 && deltaTimeS < 2.0) {
        // Velocidad promedio entre la anterior y la actual
        double currentSpeedMs = _speedKmh / 3.6;
        double avgSpeedMs = (prevSpeedMs + currentSpeedMs) / 2.0;
        double avgSpeedKmh = avgSpeedMs * 3.6;
        
        // FILTRO: Solo contar distancia si velocidad >= mínima configurada
        if (avgSpeedKmh >= _minSpeedKmh) {
          // Distancia = velocidad * tiempo
          double deltaDistance = avgSpeedMs * deltaTimeS;
          
          // Acumular solo si la distancia es razonable (< 100m entre updates)
          if (deltaDistance >= 0 && deltaDistance < 100.0) {
            _totalDistance += deltaDistance;
          }
        }
      }
    } else {
      // Primera inicialización
      _distanceInitialized = true;
    }
    
    _lastUpdateTime = currentTime;
  }
  
  return true;
}

uint8_t VBOXSport::satellites() {
  return _satellites;
}

bool VBOXSport::isDGPS() {
  return _isDGPS;
}

bool VBOXSport::hasTime() {
  return _hasTime;
}

uint32_t VBOXSport::timeMs() {
  return _timeMs;
}

void VBOXSport::getTime(uint8_t& h, uint8_t& m, uint8_t& s, uint16_t& ms) {
  h = _timeMs / 3600000;
  m = (_timeMs % 3600000) / 60000;
  s = (_timeMs % 60000) / 1000;
  ms = _timeMs % 1000;
}

void VBOXSport::getLocalTime(uint8_t& h, uint8_t& m, uint8_t& s, int8_t offsetHours) {
  // Convertir tiempo UTC a local aplicando offset
  int32_t localTimeMs = _timeMs + (offsetHours * 3600000);
  
  // Manejar cambio de día
  if (localTimeMs < 0) {
    localTimeMs += 86400000;  // +24 horas
  } else if (localTimeMs >= 86400000) {
    localTimeMs -= 86400000;  // -24 horas
  }
  
  h = localTimeMs / 3600000;
  m = (localTimeMs % 3600000) / 60000;
  s = (localTimeMs % 60000) / 1000;
}

bool VBOXSport::hasPosition() {
  return _hasLat && _hasLon;
}

double VBOXSport::latitude() {
  return _latitude;
}

double VBOXSport::longitude() {
  return _longitude;
}

bool VBOXSport::hasSpeed() {
  return _hasSpeed;
}

double VBOXSport::speedKmh() {
  if (_speedKmh < _speedClampKmh) return 0.0;
  return _speedKmh;
}

double VBOXSport::speedKnots() {
  if (_speedKmh < _speedClampKmh) return 0.0;
  return _speedKnots;
}

double VBOXSport::speedMph() {
  if (_speedKmh < _speedClampKmh) return 0.0;
  return _speedKmh * 0.621371;
}

bool VBOXSport::hasHeading() {
  return _hasHeading;
}

double VBOXSport::heading() {
  return _heading;
}

bool VBOXSport::hasHeight() {
  return _hasHeight;
}

double VBOXSport::heightM() {
  return _heightM;
}

double VBOXSport::heightFt() {
  return _heightM * 3.28084;
}

bool VBOXSport::hasVerticalSpeed() {
  return _hasVSpeed;
}

double VBOXSport::verticalSpeedMs() {
  return _verticalSpeedMs;
}

double VBOXSport::verticalSpeedFpm() {
  return _verticalSpeedMs * 196.85;
}

bool VBOXSport::hasAcceleration() {
  return _hasAccelLon || _hasAccelLat;
}

double VBOXSport::accelLongG() {
  return _accelLongG;
}

double VBOXSport::accelLatG() {
  return _accelLatG;
}

bool VBOXSport::hasHDOP() {
  return _hasHDOP;
}

double VBOXSport::hdop() {
  return _hdop;
}

String VBOXSport::hdopQuality() {
  if (_hdop < 1.0) return "Excelente";
  else if (_hdop < 2.0) return "Bueno";
  else if (_hdop < 5.0) return "Moderado";
  else return "Pobre";
}

bool VBOXSport::hasBattery() {
  return _hasBattery;
}

uint16_t VBOXSport::batteryMinutes() {
  return _batteryMinutes;
}

bool VBOXSport::isBatteryCharging() {
  return _batteryMinutes == 0xFFFF;
}

bool VBOXSport::hasBatteryPercent() {
  return _hasBatteryPercent;
}

uint8_t VBOXSport::batteryPercent() {
  return _batteryPercent;
}

bool VBOXSport::hasStorage() {
  return _hasMediaCap && _hasMediaFree;
}

uint32_t VBOXSport::storageCapacityKB() {
  return _mediaCapacityKB;
}

uint32_t VBOXSport::storageFreeKB() {
  return _mediaFreeKB;
}

float VBOXSport::storageCapacityMB() {
  return _mediaCapacityKB / 1024.0;
}

float VBOXSport::storageFreeMB() {
  return _mediaFreeKB / 1024.0;
}

float VBOXSport::storageUsedPercent() {
  if (_mediaCapacityKB == 0) return 0;
  uint32_t usedKB = _mediaCapacityKB - _mediaFreeKB;
  return (usedKB * 100.0) / _mediaCapacityKB;
}

uint32_t VBOXSport::getStdFlags() {
  return _stdFlags;
}

uint32_t VBOXSport::getExtFlags() {
  return _extFlags;
}

void VBOXSport::resetDistance() {
  _totalDistance = 0.0;
  _distanceInitialized = false;
  _lastUpdateTime = millis();
}

double VBOXSport::distanceMeters() {
  return _totalDistance;
}

double VBOXSport::distanceKm() {
  return _totalDistance / 1000.0;
}

void VBOXSport::setMinSpeedKmh(double minSpeed) {
  _minSpeedKmh = minSpeed;
}

void VBOXSport::setSpeedClamp(double minSpeedKmh) {
  _speedClampKmh = minSpeedKmh;
}

void VBOXSport::printData() {
  Serial.println("\n╔════════════════════════════════════════════════════════════════════╗");
  Serial.println("║                    DATOS VBOX DECODIFICADOS                        ║");
  Serial.println("╠════════════════════════════════════════════════════════════════════╣");
  
  Serial.print("║ Satélites: ");
  Serial.print(_satellites);
  if (_isDGPS) Serial.print(" (DGPS)");
  Serial.println();
  
  if (_hasTime) {
    uint8_t h, m, s;
    uint16_t ms;
    getTime(h, m, s, ms);
    Serial.print("║ Tiempo UTC: ");
    if (h < 10) Serial.print("0");
    Serial.print(h);
    Serial.print(":");
    if (m < 10) Serial.print("0");
    Serial.print(m);
    Serial.print(":");
    if (s < 10) Serial.print("0");
    Serial.print(s);
    Serial.print(".");
    if (ms < 100) Serial.print("0");
    if (ms < 10) Serial.print("0");
    Serial.println(ms);
  }
  
  if (hasPosition()) {
    Serial.print("║ Posición: ");
    Serial.print(_latitude, 7);
    Serial.print("°");
    Serial.print(_latitude >= 0 ? "N" : "S");
    Serial.print(", ");
    Serial.print(_longitude, 7);
    Serial.print("°");
    Serial.println(_longitude >= 0 ? "E" : "W");
  }
  
  if (_hasSpeed) {
    Serial.print("║ Velocidad: ");
    Serial.print(_speedKmh, 2);
    Serial.print(" km/h (");
    Serial.print(_speedKnots, 2);
    Serial.println(" nudos)");
  }
  
  if (_hasHeading) {
    Serial.print("║ Rumbo: ");
    Serial.print(_heading, 2);
    Serial.println("° (norte verdadero)");
  }
  
  if (_hasHeight) {
    Serial.print("║ Altura: ");
    Serial.print(_heightM, 2);
    Serial.println(" m (WGS84)");
  }
  
  if (_hasVSpeed) {
    Serial.print("║ Vel. vertical: ");
    Serial.print(_verticalSpeedMs, 2);
    Serial.println(" m/s");
  }
  
  if (hasAcceleration()) {
    Serial.println("╟────────────────────────────────────────────────────────────────────╢");
    if (_hasAccelLon) {
      Serial.print("║ Acel. longitudinal: ");
      Serial.print(_accelLongG, 3);
      Serial.println(" G");
    }
    if (_hasAccelLat) {
      Serial.print("║ Acel. lateral: ");
      Serial.print(_accelLatG, 3);
      Serial.println(" G");
    }
  }
  
  if (_hasHDOP) {
    Serial.println("╟────────────────────────────────────────────────────────────────────╢");
    Serial.print("║ HDOP: ");
    Serial.print(_hdop, 2);
    Serial.print(" (");
    Serial.print(hdopQuality());
    Serial.println(")");
  }
  
  if (hasBattery() || hasStorage()) {
    Serial.println("╟────────────────────────────────────────────────────────────────────╢");
    if (hasBattery()) {
      Serial.print("║ Batería: ");
      if (isBatteryCharging()) {
        Serial.println("Cargando");
      } else {
        Serial.print(_batteryMinutes);
        Serial.print(" min (");
        Serial.print(_batteryMinutes / 60);
        Serial.print("h ");
        Serial.print(_batteryMinutes % 60);
        Serial.print("m)");
        if (hasBatteryPercent()) {
          Serial.print(" - ");
          Serial.print(_batteryPercent);
          Serial.print("%");
        }
        Serial.println();
      }
    }
    if (hasStorage()) {
      Serial.print("║ Almacenamiento: ");
      Serial.print(storageFreeMB(), 2);
      Serial.print(" MB libres de ");
      Serial.print(storageCapacityMB(), 2);
      Serial.print(" MB (");
      Serial.print(100.0 - storageUsedPercent(), 1);
      Serial.println("% libre)");
    }
  }
  
  Serial.println("╚════════════════════════════════════════════════════════════════════╝");
}

double VBOXSport::minutes100k_to_deg(int32_t raw) {
  double minutes = (double)raw / 100000.0;
  return minutes / 60.0;
}

size_t VBOXSport::payloadLenFromFlags(uint32_t fStd, uint32_t fExt) {
  size_t n = 0;
  if (fStd & F_SATS)     n += 1;
  if (fStd & F_TIME)     n += 3;
  if (fStd & F_LAT)      n += 4;
  if (fStd & F_LON)      n += 4;
  if (fStd & F_SPEED)    n += 2;
  if (fStd & F_HEADING)  n += 2;
  if (fStd & F_HEIGHT)   n += 3;
  if (fStd & F_VSPEED)   n += 2;
  if (fStd & F_AXLON)    n += 2;
  if (fStd & F_AXLAT)    n += 2;
  if (fExt & FX_BATT_TTE)      n += 2;
  if (fExt & FX_BATT_TTF)      n += 2;
  if (fExt & FX_BATT_CAP)      n += 2;
  if (fExt & FX_BATT_PERCENT)  n += 2;
  if (fExt & FX_MEDIA_CAP)     n += 4;
  if (fExt & FX_MEDIA_FREE)    n += 4;
  if (fExt & FX_HDOP)          n += 2;
  return n;
}