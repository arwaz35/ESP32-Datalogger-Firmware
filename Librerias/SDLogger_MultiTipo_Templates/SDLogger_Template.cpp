/*
 * SDLogger_Template - Implementación
 */

#include "SDLogger_Template.h"

SDLogger::SDLogger() {
  fileNumber = -1;
  lineStarted = false;
}

bool SDLogger::begin(uint8_t cs, uint8_t speedMHz) {
  Serial.println("Iniciando SDLogger (versión multi-tipo)...");
  Serial.print("Tipo de sistema de archivos: ");
  
  #if SD_FAT_TYPE == 0
    Serial.println("FAT16/FAT32");
  #elif SD_FAT_TYPE == 1
    Serial.println("FAT32");
  #elif SD_FAT_TYPE == 2
    Serial.println("exFAT");
  #elif SD_FAT_TYPE == 3
    Serial.println("Universal (FAT16/FAT32/exFAT)");
  #endif
  
  SdSpiConfig config(cs, SHARED_SPI, SD_SCK_MHZ(speedMHz));
  
  if (!sd.begin(config)) {
    Serial.println("Error: No se pudo inicializar la tarjeta SD");
    return false;
  }
  
  Serial.println("✓ SD inicializada correctamente!");
  
  uint32_t cardSize = sd.card()->sectorCount() * 0.000512;
  Serial.print("Tamaño de tarjeta: ");
  Serial.print(cardSize);
  Serial.println(" MB");
  
  fileNumber = findNextFileNumber();
  if (fileNumber < 0) {
    Serial.println("Error: No se pudo encontrar número de archivo disponible");
    return false;
  }
  
  if (!createNewFile(fileNumber)) {
    Serial.println("Error: No se pudo crear el archivo");
    return false;
  }
  
  Serial.print("Archivo creado: ");
  Serial.println(fileName);
  
  return true;
}

int SDLogger::findNextFileNumber() {
  for (int i = 0; i <= 999; i++) {
    char testName[13];
    sprintf(testName, "/%03d.csv", i);
    if (!sd.exists(testName)) {
      return i;
    }
  }
  return -1;
}

bool SDLogger::createNewFile(int number) {
  char name[13];
  sprintf(name, "/%03d.csv", number);
  fileName = String(name);
  
  dataFile = sd.open(fileName.c_str(), FILE_WRITE);
  
  if (!dataFile) {
    return false;
  }
  
  return true;
}

void SDLogger::startLine() {
  lineStarted = false;
}

void SDLogger::endLine() {
  if (dataFile) {
    dataFile.println();
    lineStarted = false;
  }
}

// Sobrecarga para String
void SDLogger::addValue(String text) {
  if (!dataFile) {
    return;
  }
  
  if (lineStarted) {
    dataFile.print(";");
  }
  
  dataFile.print(text);
  lineStarted = true;
}

// Sobrecarga para const char*
void SDLogger::addValue(const char* text) {
  if (!dataFile) {
    return;
  }
  
  if (lineStarted) {
    dataFile.print(";");
  }
  
  dataFile.print(text);
  lineStarted = true;
}

void SDLogger::flush() {
  if (dataFile) {
    dataFile.flush();
  }
}

void SDLogger::close() {
  if (dataFile) {
    dataFile.close();
    Serial.println("Archivo cerrado");
  }
}

String SDLogger::getFileName() {
  return fileName;
}
