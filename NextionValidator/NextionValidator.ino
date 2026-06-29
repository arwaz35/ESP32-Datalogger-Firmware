/**
 * NextionValidator.ino
 * 
 * Programa de diagnóstico independiente para validar la comunicación serial 
 * bidireccional entre el ESP32 y la pantalla Nextion.
 * 
 * Monitorea los bytes entrantes en hexadecimal, decodifica los eventos táctiles
 * y permite enviar comandos interactivos desde la consola serial de la PC.
 */

#define pnext Serial1

// Configuración de pines de la pantalla Nextion (idéntico a FirmwareV2_0)
const int NEXTION_RX_PIN = 35; // Conecta al TX de la Nextion
const int NEXTION_TX_PIN = 32; // Conecta al RX de la Nextion

// Velocidad de comunicación por defecto
// NOTA: Si no recibes datos, prueba cambiando esta velocidad a 9600 baudios,
// que es la velocidad por defecto de fábrica de muchas pantallas Nextion.
const long NEXTION_BAUDRATE = 115200; 

void setup() {
  // Iniciar puerto serial de depuración hacia el PC
  Serial.begin(115200);
  while (!Serial) {
    ; // Esperar a que se conecte el puerto serial (solo en placas nativas USB)
  }
  delay(1000);
  
  Serial.println("\n==================================================");
  Serial.println("  HERRAMIENTA DE DIAGNÓSTICO Y VALIDACIÓN NEXTION  ");
  Serial.println("==================================================");
  Serial.print("Iniciando Serial1 (Nextion) a ");
  Serial.print(NEXTION_BAUDRATE);
  Serial.println(" bps...");
  Serial.print("Pines configurados -> RX: ");
  Serial.print(NEXTION_RX_PIN);
  Serial.print(" | TX: ");
  Serial.println(NEXTION_TX_PIN);

  // Iniciar el puerto de la pantalla Nextion
  pnext.begin(NEXTION_BAUDRATE, SERIAL_8N1, NEXTION_RX_PIN, NEXTION_TX_PIN);
  
  Serial.println("\nListo. Esperando datos de la pantalla...");
  Serial.println("Presiona botones en la Nextion para verificar el envío de tramas.");
  Serial.println("--------------------------------------------------");
  Serial.println("Instrucciones para enviar comandos desde la PC:");
  Serial.println(" - Escribe un comando (ej: page 0 o tfil.txt=\"Test\") y presiona Enter.");
  Serial.println(" - El programa le agregará los tres bytes 0xFF 0xFF 0xFF automáticamente.");
  Serial.println("--------------------------------------------------\n");
}

// Variables para el parser de eventos táctiles
uint8_t nexBuffer[7];
uint8_t nexIndex = 0;

// Variables para acumular comandos de la consola de PC
String comandoPC = "";

void loop() {
  // --- 1. LEER DATOS QUE ENVIÓ LA NEXTION AL ESP32 ---
  while (pnext.available()) {
    uint8_t b = pnext.read();
    
    // Imprimir el byte crudo en formato Hexadecimal para inspección profunda
    Serial.print("Byte recibido crudo: 0x");
    if (b < 0x10) Serial.print("0");
    Serial.print(b, HEX);
    
    // Si es un caracter ASCII imprimible, mostrarlo para facilitar lectura
    if (b >= 32 && b <= 126) {
      Serial.print(" ('");
      Serial.print((char)b);
      Serial.print("')");
    }
    Serial.println();

    // Procesar la trama de eventos táctiles (Nextion Touch Event)
    // Estructura esperada: [0x65] [PageID] [ComponentID] [Event] [0xFF] [0xFF] [0xFF]
    if (nexIndex == 0) {
      if (b == 0x65) {
        nexBuffer[nexIndex++] = b;
        Serial.println(" -> Detectado inicio de trama Touch Event (0x65)");
      }
    } else {
      nexBuffer[nexIndex++] = b;

      // Si hemos acumulado 7 bytes, validamos la trama
      if (nexIndex == 7) {
        // Verificar si termina con el terminador estándar de Nextion (0xFF 0xFF 0xFF)
        if (nexBuffer[4] == 0xFF && nexBuffer[5] == 0xFF && nexBuffer[6] == 0xFF) {
          uint8_t pageId = nexBuffer[1];
          uint8_t componentId = nexBuffer[2];
          uint8_t eventType = nexBuffer[3]; // 0x00 = Release (Soltar), 0x01 = Press (Presionar)

          Serial.println("\n>>> TRAMA NEXTION DECODIFICADA CON ÉXITO <<<");
          Serial.print("  - ID de Página: "); Serial.println(pageId);
          Serial.print("  - ID de Componente: "); Serial.print(componentId);
          Serial.print(" (Hex: 0x"); Serial.print(componentId, HEX); Serial.println(")");
          Serial.print("  - Tipo de Evento: "); 
          if (eventType == 0x01) {
            Serial.println("1 (Touch Press / Presionar)");
          } else if (eventType == 0x00) {
            Serial.println("0 (Touch Release / Soltar)");
          } else {
            Serial.print(eventType); Serial.println(" (Desconocido)");
          }

          // Validación específica contra el firmware principal (Componente ID 18 / 0x12)
          if (componentId == 0x12) {
            Serial.println("  [! SUCCESS !] ¡Este es el Componente ID 18 (0x12) que busca el firmware principal!");
            if (eventType == 0x01) {
              Serial.println("  [! SUCCESS !] Evento es 'Press' (0x01). ¡Esta pulsación llamaría a NUEVO_ARCHIVO_SD()!");
            } else {
              Serial.println("  [ ADVERTENCIA ] El evento es 'Release' (0x00). El firmware principal busca un 'Press' (0x01).");
            }
          } else {
            Serial.print("  [ INFORMACIÓN ] Este componente (ID ");
            Serial.print(componentId);
            Serial.println(") no coincide con el ID 18 (0x12) que activa la creación del archivo SD.");
          }
          Serial.println("--------------------------------------------\n");

        } else {
          // El terminador no es 0xFF 0xFF 0xFF. Informar sobre desalineación.
          Serial.println("\n>>> ERROR: Trama desalineada o corrupta <<<");
          Serial.print("  Bytes acumulados: ");
          for (int i = 0; i < 7; i++) {
            Serial.print("0x");
            if (nexBuffer[i] < 0x10) Serial.print("0");
            Serial.print(nexBuffer[i], HEX);
            Serial.print(" ");
          }
          Serial.println("\n  Los últimos 3 bytes no son 0xFF 0xFF 0xFF. Ignorando trama.\n");
        }

        // Resetear el buffer del parser
        nexIndex = 0;
      }
    }
  }

  // --- 2. ENVIAR COMANDOS DESDE PC A LA PANTALLA ---
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {
      if (comandoPC.length() > 0) {
        // Enviar el comando ingresado
        pnext.print(comandoPC);
        
        // Adjuntar los tres bytes terminadores indispensables de Nextion
        pnext.write(0xFF);
        pnext.write(0xFF);
        pnext.write(0xFF);

        Serial.print("-> Comando enviado a Nextion: \"");
        Serial.print(comandoPC);
        Serial.println("\" (+ 0xFF 0xFF 0xFF)");
        
        // Limpiar para el siguiente comando
        comandoPC = "";
      }
    } else {
      comandoPC += c;
    }
  }
}
