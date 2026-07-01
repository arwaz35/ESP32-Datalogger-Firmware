/*
 * VBOXSport.h
 * Librería para VBOX Sport con ESP32
 * Versión 2.0 - Optimizada para baja latencia
 */

#ifndef VBOXSPORT_H
#define VBOXSPORT_H

#include <Arduino.h>
#include <BluetoothSerial.h>

class VBOXSport {
public:
  // Constructor
  VBOXSport();
  
  // Conexión
  bool begin(const char* vboxName);
  bool begin(uint8_t* macAddress);
  void disconnect();
  bool isConnected();
  void update();  // Llamar en loop() - obtiene datos más recientes
  
  // GPS básico
  uint8_t satellites();
  bool isDGPS();
  
  // Tiempo
  bool hasTime();
  uint32_t timeMs();           // Milisegundos desde medianoche UTC
  void getTime(uint8_t& h, uint8_t& m, uint8_t& s, uint16_t& ms);
  void getLocalTime(uint8_t& h, uint8_t& m, uint8_t& s, int8_t offsetHours = -5); // Hora local (default: Bogotá UTC-5)
  
  // Posición
  bool hasPosition();
  double latitude();           // Grados (N+/S-)
  double longitude();          // Grados (E+/W-)
  
  // Velocidad
  bool hasSpeed();
  double speedKmh();
  double speedKnots();
  double speedMph();
  
  // Rumbo y altura
  bool hasHeading();
  double heading();            // Grados desde norte verdadero
  
  bool hasHeight();
  double heightM();
  double heightFt();
  
  bool hasVerticalSpeed();
  double verticalSpeedMs();
  double verticalSpeedFpm();   // Feet per minute
  
  // Aceleraciones
  bool hasAcceleration();
  double accelLongG();
  double accelLatG();
  
  // Calidad GPS
  bool hasHDOP();
  double hdop();
  String hdopQuality();        // "Excelente", "Bueno", "Moderado", "Pobre"
  
  // Batería
  bool hasBattery();
  bool hasBatteryPercent();    // Verifica si el porcentaje está disponible
  uint16_t batteryMinutes();
  bool isBatteryCharging();
  uint8_t batteryPercent();      // Porcentaje actual (0-100)
  
  // Almacenamiento
  bool hasStorage();
  uint32_t storageCapacityKB();
  uint32_t storageFreeKB();
  float storageCapacityMB();
  float storageFreeMB();
  float storageUsedPercent();
  
  // Distancia y viaje
  void resetDistance();          // Resetear distancia a cero
  double distanceMeters();       // Distancia acumulada en metros
  double distanceKm();           // Distancia acumulada en kilómetros
  void setMinSpeedKmh(double minSpeed); // Velocidad mínima para contar distancia (default: 0.8 km/h)
  void setSpeedClamp(double minSpeedKmh); // Velocidad mínima para reportar movimiento (default: 0.0)
  
  // Debug
  void printData();            // Imprime todos los datos formateados
  uint32_t getStdFlags();
  uint32_t getExtFlags();
  
private:
  BluetoothSerial _bt;
  
  // Buffer simple para mensaje actual
  static const size_t BUFFER_SIZE = 128;
  uint8_t _buffer[BUFFER_SIZE];
  size_t _bufferPos;
  
  // Datos parseados
  uint32_t _stdFlags, _extFlags;
  uint8_t _satellites;
  bool _isDGPS;
  uint32_t _timeMs;
  double _latitude, _longitude;
  double _speedKmh, _speedKnots;
  double _heading, _heightM;
  double _verticalSpeedMs;
  double _accelLongG;
  double _accelLatG;
  
  uint16_t _batteryMinutes;
  uint8_t _batteryPercent;
  uint32_t _mediaCapacityKB;
  uint32_t _mediaFreeKB;
  double _hdop;
  
  bool _hasTime, _hasLat, _hasLon, _hasSpeed, _hasHeading;
  bool _hasHeight, _hasVSpeed, _hasAccelLon, _hasAccelLat;
  bool _hasBattery, _hasBatteryPercent, _hasMediaCap, _hasMediaFree, _hasHDOP;
  
  uint32_t _lastReconnect;
  
  // Variables para cálculo de distancia
  double _totalDistance;         // Distancia acumulada en metros
  uint32_t _lastUpdateTime;      // Última vez que se actualizó
  bool _distanceInitialized;     // Si ya se inicializó el sistema
  double _minSpeedKmh;           // Velocidad mínima para contar distancia
  double _speedClampKmh;         // Velocidad mínima para reportar velocidad
  
  // Método privado para reconexión
  String _vboxName;
  
  // Métodos privados
  double minutes100k_to_deg(int32_t raw);
  size_t payloadLenFromFlags(uint32_t fStd, uint32_t fExt);
  bool tryParse();
  bool connectVBOX(const char* name);
  bool connectVBOX(uint8_t* macAddress);
  
  bool _useMac;
  uint8_t _targetMac[6];
};

#endif