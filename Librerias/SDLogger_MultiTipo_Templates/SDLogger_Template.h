/*
 * SDLogger con Templates - Soporta cualquier tipo de dato
 * int, long, float, double, uint8_t, etc.
 */

#ifndef SD_LOGGER_TEMPLATE_H
#define SD_LOGGER_TEMPLATE_H

#include <Arduino.h>
#include "SdFat.h"

#define SD_FAT_TYPE 3

class SDLogger {
  public:
    SDLogger();
    
    bool begin(uint8_t cs, uint8_t speedMHz = 10);
    
    // MÉTODO 1: Array de un solo tipo (TEMPLATE - cualquier tipo)
    template <typename T>
    void writeArray(T* data, int count, int decimals = 4) {
      if (!dataFile || count <= 0) {
        return;
      }
      
      // Escribir todos los valores excepto el último
      for (int i = 0; i < count - 1; i++) {
        dataFile.print(data[i], decimals);
        dataFile.print(";");
      }
      
      // Escribir el último valor con salto de línea
      dataFile.println(data[count - 1], decimals);
    }
    
    // MÉTODO 2: Tipos mixtos - iniciar línea
    void startLine();
    
    // MÉTODO 3: Agregar valor de cualquier tipo
    template <typename T>
    void addValue(T value, int decimals = 4) {
      if (!dataFile) {
        return;
      }
      
      if (lineStarted) {
        dataFile.print(";");
      }
      
      dataFile.print(value, decimals);
      lineStarted = true;
    }
    
    // MÉTODO 3b: Agregar String (para encabezados o texto)
    void addValue(String text);
    void addValue(const char* text);
    
    // MÉTODO 4: Finalizar línea
    void endLine();
    
    void flush();
    void close();
    String getFileName();
    
  private:
    #if SD_FAT_TYPE == 0
      SdFat sd;
      File dataFile;
    #elif SD_FAT_TYPE == 1
      SdFat32 sd;
      File32 dataFile;
    #elif SD_FAT_TYPE == 2
      SdExFat sd;
      ExFile dataFile;
    #elif SD_FAT_TYPE == 3
      SdFs sd;
      FsFile dataFile;
    #endif
    
    String fileName;
    int fileNumber;
    bool lineStarted;
    
    int findNextFileNumber();
    bool createNewFile(int number);
};

#endif
