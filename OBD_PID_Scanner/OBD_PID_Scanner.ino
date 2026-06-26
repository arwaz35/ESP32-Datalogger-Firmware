#include "driver/twai.h"

// Configuración CAN (TWAI) - Mismos pines del firmware principal
#define CAN_TX_PIN GPIO_NUM_4
#define CAN_RX_PIN GPIO_NUM_5

#define OBD_REQUEST_ID 0x7E0
#define OBD_REPLY_ID 0x7E8
#define TIMEOUT_CAN_MS 150 // Tiempo de espera para respuesta de la ECU

// Estructura para definir los PIDs a escanear
struct OBD_PID {
  uint8_t code;
  const char* name;
  const char* desc;
};

// Lista de PIDs propuestos para el análisis de consumo de combustible
OBD_PID pids_to_scan[] = {
  {0x0B, "MAP", "Intake Manifold Absolute Pressure (Presión Colector)"},
  {0x0C, "RPM", "Engine RPM (Revoluciones por Minuto)"},
  {0x0D, "VSS", "Vehicle Speed Sensor (Velocidad Moto)"},
  {0x0F, "IAT", "Intake Air Temperature (Temperatura Admisión)"},
  {0x10, "MAF", "Mass Air Flow (Flujo de Masa de Aire)"},
  {0x5E, "Fuel Rate", "Engine Fuel Rate (Consumo Directo)"}
};

const int NUM_PIDS = sizeof(pids_to_scan) / sizeof(pids_to_scan[0]);

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println("\n==============================================");
  Serial.println("         OBD-II PID SCANNER PARA MOTO         ");
  Serial.println("==============================================");
  Serial.println("Iniciando CAN TWAI...");

  // Configuración del driver CAN a 500 Kbps (estándar OBD-II CAN)
  twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    Serial.println("Driver CAN instalado.");
  } else {
    Serial.println("Error al instalar driver CAN. Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  if (twai_start() == ESP_OK) {
    Serial.println("Driver CAN iniciado correctamente.");
  } else {
    Serial.println("Error al iniciar driver CAN. Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("\nConecta el dispositivo a la moto, enciende el switch (ignition ON)");
  Serial.println("y observa los resultados del escaneo.");
  Serial.println("==============================================");
}

void loop() {
  Serial.println("\n--- Iniciando escaneo de PIDs de combustible ---");
  int disponibles = 0;

  for (int i = 0; i < NUM_PIDS; i++) {
    bool respondio = false;
    uint8_t valA = 0;
    uint8_t valB = 0;

    // 1. Configurar trama de petición OBD-II
    twai_message_t message_req;
    message_req.identifier = OBD_REQUEST_ID;
    message_req.extd = 0;
    message_req.rtr = 0;
    message_req.data_length_code = 8;
    
    message_req.data[0] = 0x02;               // 2 bytes de datos adicionales
    message_req.data[1] = 0x01;               // Servicio 01 (Mostrar datos actuales)
    message_req.data[2] = pids_to_scan[i].code; // PID a escanear
    for (int k = 3; k < 8; k++) {
      message_req.data[k] = 0x00;             // Cero en los bytes restantes
    }

    // Limpiar mensajes anteriores residuales del buffer CAN
    twai_message_t temp_msg;
    while (twai_receive(&temp_msg, 0) == ESP_OK) {
      // Vaciar buffer
    }

    // 2. Transmitir petición
    if (twai_transmit(&message_req, pdMS_TO_TICKS(10)) != ESP_OK) {
      Serial.printf("[PID 0x%02X] %-10s -> ERROR de transmision CAN\n", pids_to_scan[i].code, pids_to_scan[i].name);
      continue;
    }

    // 3. Esperar respuesta con timeout
    unsigned long start_time = millis();
    twai_message_t message_resp;

    while (millis() - start_time < TIMEOUT_CAN_MS) {
      if (twai_receive(&message_resp, pdMS_TO_TICKS(5)) == ESP_OK) {
        // Verificar que provenga de la ECU (0x7E8) y sea respuesta del servicio 01 (0x41)
        if (message_resp.identifier == OBD_REPLY_ID && message_resp.data[1] == 0x41) {
          // Verificar que corresponda al PID consultado
          if (message_resp.data[2] == pids_to_scan[i].code) {
            respondio = true;
            valA = message_resp.data[3];
            valB = message_resp.data[4];
            break;
          }
        }
      }
    }

    // 4. Reportar resultados del PID
    if (respondio) {
      disponibles++;
      Serial.printf("[PID 0x%02X] %-10s -> [DISPONIBLE] - Datos recibidos: [0x%02X 0x%02X] ", 
                    pids_to_scan[i].code, pids_to_scan[i].name, valA, valB);
      
      // Decodificación preliminar de prueba para asegurar que el dato sea coherente
      switch (pids_to_scan[i].code) {
        case 0x0B: // MAP
          Serial.printf("(Presion: %d kPa)\n", valA);
          break;
        case 0x0C: // RPM
          Serial.printf("(RPM: %d rpm)\n", ((256 * valA) + valB) / 4);
          break;
        case 0x0D: // VSS
          Serial.printf("(Velocidad: %d km/h)\n", valA);
          break;
        case 0x0F: // IAT
          Serial.printf("(Temp Aire: %d C)\n", valA - 40);
          break;
        case 0x10: // MAF
          Serial.printf("(MAF: %.2f g/s)\n", ((256.0 * valA) + valB) / 100.0);
          break;
        case 0x5E: // Fuel Rate
          Serial.printf("(Fuel Rate: %.2f L/h)\n", ((256.0 * valA) + valB) / 20.0);
          break;
        default:
          Serial.println();
          break;
      }
    } else {
      Serial.printf("[PID 0x%02X] %-10s -> NO DISPONIBLE o sin respuesta (ECU apagada/no soportado)\n", 
                    pids_to_scan[i].code, pids_to_scan[i].name);
    }
    
    // Pequeño delay entre consultas para no saturar el bus CAN
    delay(50);
  }

  Serial.printf("\n---> Resumen: %d de %d PIDs respondieron correctamente.\n", disponibles, NUM_PIDS);
  Serial.println("==================================================================");
  
  // Esperar 4 segundos antes del siguiente ciclo de escaneo
  delay(4000);
}
