#include "SDLogger_Template.h"
#include "driver/twai.h"
#include <VBOXSport.h>

String Version = "2.1";

// Configuración CAN (TWAI)
#define CAN_TX_PIN GPIO_NUM_4
#define CAN_RX_PIN GPIO_NUM_5
#define OBD_REQUEST_ID 0x7E0
#define OBD_REPLY_ID 0x7E8

// Crear instancia de NEXTION
#define pnext Serial1

// Configuracion SD
#define CS_PIN 15
#define NUM_VARIABLES 24
SDLogger sd;

// Pulsador
const int D_IN_1 = 14;
// volatile int estadoVariable = 0;
// volatile unsigned long tiempoActivacion = 0;
volatile int pulsador = 5;
volatile int pulsadorSD = 0;
volatile int resetPulsador = 0;
bool flagPulsador6 = false; // Flag para detectar cambio a pulsador = 6

// Definir encabezados como constantes (ahorra RAM)
const char *ENCABEZADOS[NUM_VARIABLES] = {
    "Hora",                 // 1
    "Satelites",            // 2
    "Latitud",              // 3
    "Longitud",             // 4
    "Velocidad_GPS",        // 5
    "Velocidad_Moto",       // 6
    "Altitud",              // 7
    "Accel_X",              // 8
    "Accel_Y",              // 9
    "RPM",                  // 10
    "TPS",                  // 11
    "Temperatura_Motor",    // 12
    "Carga_Motor",          // 13
    "Calidad",              // 14
    "Bateria",              // 15
    "Distancia",            // 16
    "Pulsador",             // 17
    "Distancia_Parcial",    // 18
    "Promedio_Velocidad",   // 19
    "Promedio_Aceleracion", // 20
    "Consumo_L_H",          // 21
    "Consumo_Acumulado_L",  // 22
    "KmL_Instantaneo",      // 23
    "KmL_Promedio"          // 24
};

// Variables CAN_OBD--------------------
int dim_PID;
int timeoutCAN = 20; // ms
bool flagRead = false;
int LOAD_PTC = 0, ECT = 0, RPM = 0, VSS = 0, IAT = 0, TP = 0, BARO = 0;
float LTF_Trim = 0.0, MAP = 0.0, CMV = 0;

// Calibración Física de la Moto (Yamaha FINN 114 cc)
const float CILINDRADA_L = 0.114;
const float EFICIENCIA_V = 80.0;

// Variables globales para almacenamiento de consumo
float consumoLh = 0.0;
float litrosConsumidos = 0.0;
float kmLInstantaneo = 0.0;
float kmLPromedio = 0.0;
unsigned long ultimoMensajeECU = 0;

// PIDs activos a consultar (incluyendo MAP 0x0B y IAT 0x0F)
int PIDs[] = {0x04, 0x0B, 0x0C, 0x0D, 0x0F, 0x11};

// Variables
// GPS
double lat = 0.0, lon = 0.0, vel = 0.0, alt = 0.0, accX = 0.0, accY = 0.0,
       hdop = 0.0, dism = 0.0, diskm = 0.0;
int sat = 0, bat = 0, calidad_num = 0, batPic = 0;
uint8_t h = 0, m = 0, s = 0;
String timegps = "";
String archivo = "";

// Variables para cálculos cuando pulsador = 6
double distanciaParcial = 0.0;      // Acumulador de distancia parcial
double sumaVelocidades = 0.0;       // Suma de velocidades para promedio
double sumaAceleraciones = 0.0;     // Suma de aceleraciones para promedio
unsigned long contadorMuestras = 0; // Contador de muestras para promedios
double promedioVelocidad = 0.0;     // Promedio de velocidad
double promedioAceleracion = 0.0;   // Promedio de aceleración
double distanciaAnterior = 0.0;     // Para calcular diferencia de distancia

// Esperar hasta completar exactamente 100 ms (sin usar delay para no bloquear
// interrupciones)
const unsigned long INTERVALO_MS = 100;

// Crear instancia de VBOX
VBOXSport vbox;

// Configuración
// const char* VBOX_NAME = "VBSport 07019094"; // Usado anteriormente

// Variables TIEMPO
unsigned long tiempoInicio = 0, beforeTime = 0, tiempoFinal = 0,
              tiempoTotal = 0;

// unsigned long tiempoantes=0, tiempodespues=0, tiempototal=0;

void actualizarTexto(String objeto, String valor, String unidad = "") {
  pnext.print(objeto + ".txt=\"" + valor + unidad + "\"\xFF\xFF\xFF");
  // Uso:
  // actualizarTexto("t0", String(bat), "%");
  // actualizarTexto("t1", String(temperatura, 1), "°C");
  // actualizarTexto("t2", String(voltaje, 2), "V");
  // actualizarTexto("t3", String(sat));  Sin unidad
}

void setup() {
  // Iniciar Puerto Serial
  Serial.begin(115200);
  delay(1000);
  Serial.println("Serial iniciado");
  delay(1000);

  // Configurar el pin 34 como entrada
  pinMode(D_IN_1, INPUT);

  // Configuracion CAN--------------------
  // Configuracion CAN (TWAI)--------------------
  Serial.println("Iniciando CAN TWAI...");
  twai_general_config_t g_config =
      TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
  twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
  twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
    Serial.println("Driver CAN instalado");
  } else {
    Serial.println("Fallo al instalar driver CAN");
  }

  if (twai_start() == ESP_OK) {
    Serial.println("Driver CAN iniciado");
  } else {
    Serial.println("Fallo al iniciar driver CAN");
  }

  dim_PID = sizeof(PIDs) / sizeof(PIDs[0]); // Cantidad de PIDs a consultar

  // Iniciar Pantalla Nextion
  pnext.begin(115200, SERIAL_8N1, 35,
              32); //(baud, config=SERIAL_8N1, rxPin=txPNEXT, txPin=rxPNEXT)
  delay(1000);
  pnext.print("page 0");
  pnext.print("\xFF\xFF\xFF");
  pnext.print("page 0");
  pnext.print("\xFF\xFF\xFF");
  Serial.println("Pantalla iniciada");
  actualizarTexto("tver", Version);
  delay(3000);
  pnext.print("page 1");
  pnext.print("\xFF\xFF\xFF");

  // Iniciar Bluetooth y conexion a VBOX
  // if(vbox.begin(VBOX_NAME)){
  uint8_t macVBOX[] = {0xC4, 0xD3, 0x6A, 0xE3, 0x03, 0x13};
  if (vbox.begin(macVBOX)) {
    Serial.println("¡Conectado exitosamente!");
    // Configurar filtro de velocidad mínima para cálculo de distancia
    vbox.setMinSpeedKmh(0.5); // Solo contará distancia si velocidad >= 0.5 km/h
    vbox.setSpeedClamp(
        0.5); // Reportará 0 km/h si la velocidad detectada es < 0.5 km/h
    // Serial.println("Filtro de velocidad configurado: 0.5 km/h");
  } else {
    Serial.println("Error de conexión");
  }

  // Iniciar SD
  if (!sd.begin(CS_PIN, 10)) {
    Serial.println("Error al inicializar SD");
    while (1)
      delay(1000);
  } else {
    Serial.println("SD iniciada");
  }
  delay(500);

  // Tomar nombre del archivo y pantalla
  archivo = sd.getFileName();
  pnext.print("tfil.txt=\"" + archivo + "\"\xFF\xFF\xFF");

  // Escribir encabezados usando el array
  sd.startLine();
  for (int i = 0; i < NUM_VARIABLES; i++) {
    sd.addValue(ENCABEZADOS[i]);
  }
  sd.endLine();
  sd.flush();

  Serial.println("Encabezados escritos!");
  Serial.println("Iniciando grabación...");
  ultimoMensajeECU = millis();
}

void CALCULOS() {
  // Detectar cambio a pulsador = 6 (solo resetear una vez)
  if (pulsador == 6 && !flagPulsador6) {
    // Primera vez que pulsador llega a 6, resetear acumuladores
    distanciaParcial = 0.0;
    sumaVelocidades = 0.0;
    sumaAceleraciones = 0.0;
    contadorMuestras = 0;
    promedioVelocidad = 0.0;
    promedioAceleracion = 0.0;
    distanciaAnterior = dism; // Establecer el punto de partida
    litrosConsumidos = 0.0;   // Resetear consumo acumulado del viaje
    flagPulsador6 = true;     // Marcar que ya se procesó el cambio
    Serial.println("Cálculos reseteados - Pulsador = 6");
    return; // Salir sin acumular en este ciclo (acabamos de resetear)
  }

  // Resetear el flag cuando pulsador vuelve a un estado diferente de 6
  if (pulsador != 6 && flagPulsador6) {
    flagPulsador6 = false; // Preparar para la próxima pulsación
  }

  // Acumular datos siempre (tanto si pulsador == 6 como si != 6)
  // Solo no acumulamos en el ciclo donde se reseteó (return arriba)
  distanciaParcial = dism - distanciaAnterior;

  // Acumular velocidades y aceleraciones para promedios
  sumaVelocidades += vel;
  sumaAceleraciones += accX;
  contadorMuestras++;

  // Calcular promedios
  if (contadorMuestras > 0) {
    promedioVelocidad = sumaVelocidades / contadorMuestras;
    promedioAceleracion = sumaAceleraciones / contadorMuestras;
  }

  // --- CÁLCULO DE CONSUMO DE COMBUSTIBLE (Speed-Density) ---
  if (RPM > 0 && MAP > 0) {
    float tempKelvin = IAT + 273.15;
    consumoLh =
        (RPM * MAP * CILINDRADA_L * EFICIENCIA_V) / (10400.0 * tempKelvin);

    // Integración en ciclo de 100ms (0.1 segundos): L/h / 36000
    litrosConsumidos += (consumoLh / 36000.0);
  } else {
    consumoLh = 0.0;
  }

  // Consumo instantáneo en km/L (usando velocidad GPS del VBOX)
  if (consumoLh > 0.01) {
    kmLInstantaneo = vel / consumoLh;
  } else {
    kmLInstantaneo = 0.0;
  }

  // Consumo promedio del trayecto en km/L
  if (litrosConsumidos > 0.0001) {
    kmLPromedio = (distanciaParcial / 1000.0) / litrosConsumidos;
  } else {
    kmLPromedio = 0.0;
  }
}

void loop() {
  tiempoInicio = millis();

  // Leer pulsador de forma robusta por software (Polling a 10 Hz)
  if (digitalRead(D_IN_1) == LOW) {
    pulsador = 6;
    pulsadorSD = 100;
    resetPulsador = 0;
  }

  vbox.update();

  // Eliminamos el return para que el ciclo siga corriendo a 10 Hz leyendo CAN y
  // actualizando pantalla if (!vbox.isConnected()) {
  //   Serial.println("Desconectado...");
  //   delay(1000);
  //   return;
  // }
  // tiempoFinal = millis();
  // tiempoTotal = tiempoFinal - tiempoInicio;
  // Serial.println("Tiempo Update: " + String(tiempoTotal) + " ms");

  READ_CAN();
  // tiempoFinal = millis();
  // tiempoTotal = tiempoFinal - tiempoInicio;
  // Serial.println("Tiempo Read CAN: " + String(tiempoTotal) + " ms");

  GPS();
  // tiempoFinal = millis();
  // tiempoTotal = tiempoFinal - tiempoInicio;
  // Serial.println("Tiempo GPS: " + String(tiempoTotal) + " ms");

  CALCULOS();
  // tiempoFinal = millis();
  // tiempoTotal = tiempoFinal - tiempoInicio;
  // Serial.println("Tiempo Calculos: " + String(tiempoTotal) + " ms");

  MICRO_SD();
  // tiempoFinal = millis();
  // tiempoTotal = tiempoFinal - tiempoInicio;
  // Serial.println("Tiempo Micro SD: " + String(tiempoTotal) + " ms");

  PANTALLA();
  // tiempoFinal = millis();
  // tiempoTotal = tiempoFinal - tiempoInicio;
  // Serial.println("Tiempo Pantalla: " + String(tiempoTotal) + " ms");

  resetPulsador++;

  while (millis() - tiempoInicio < INTERVALO_MS) {
    // Esperar sin hacer nada (compatible con interrupciones)
  }

  if (resetPulsador >= 20) {
    resetPulsador = 0;
    pulsador = 5;
  }

  tiempoFinal = millis();
  tiempoTotal = tiempoFinal - tiempoInicio;
  // Serial.println("Tiempo total: " + String(tiempoTotal) + " ms");
  // Serial.println(" ");

  // Serial.println("Consumo: " + String(consumoLh, 2) + " L/h | " +
  //                "Consumo Acumulado: " + String(litrosConsumidos, 2) + " L |
  //                " + "Km/L Instantáneo: " + String(kmLInstantaneo, 2) + "
  //                km/L | " + "Km/L Promedio: " + String(kmLPromedio, 2) + "
  //                km/L");

  // // Verificar si el ciclo tardó más de lo esperado
  // unsigned long tiempoTotal = millis() - tiempoInicio;
  // if (tiempoTotal > INTERVALO_MS) {
  //   Serial.println("ADVERTENCIA: El ciclo tardó " + String(tiempoTotal) +
  //   ms (>100 ms)");
  // }
}

void READ_CAN() {
  twai_message_t message_req;
  message_req.identifier = OBD_REQUEST_ID;
  message_req.extd = 0;
  message_req.rtr = 0;
  message_req.data_length_code = 8;
  // Rellenar ceros fijos
  for (int k = 3; k < 8; k++)
    message_req.data[k] = 0x00;

  for (int i = 0; i < dim_PID; i++) {
    // Configurar trama de petición
    message_req.data[0] = 0x02;    // bytes adicionales
    message_req.data[1] = 0x01;    // Servicio
    message_req.data[2] = PIDs[i]; // PID actual

    // Enviar petición
    if (twai_transmit(&message_req, pdMS_TO_TICKS(5)) != ESP_OK) {
      // Si falla envio, saltamos al siguiente
      continue;
    }

    beforeTime = millis();
    flagRead = false;

    // Esperar respuesta
    twai_message_t message_resp;
    while (millis() - beforeTime < timeoutCAN) {
      if (twai_receive(&message_resp, pdMS_TO_TICKS(2)) == ESP_OK) {
        // Verificar si es respuesta OBD (0x7E8) y Servicio correcto (0x41)
        if (message_resp.identifier == OBD_REPLY_ID &&
            message_resp.data[1] == 0x41) {
          // Verificar que el PID coincida con el solicitado (o parsear lo que
          // llegue)
          if (message_resp.data[2] == PIDs[i]) {
            switch (message_resp.data[2]) {
            case 0x04: // Calculated Load
              LOAD_PTC = message_resp.data[3] * 100 / 255;
              flagRead = true;
              break;
            case 0x0B: // MAP (Manifold Absolute Pressure)
              MAP = message_resp.data[3];
              flagRead = true;
              break;
            case 0x0C: // RPM
              RPM = ((256 * message_resp.data[3]) + message_resp.data[4]) / 4;
              flagRead = true;
              break;
            case 0x0D: // VSS
              VSS = message_resp.data[3];
              flagRead = true;
              break;
            case 0x0F: // IAT (Intake Air Temperature)
              IAT = message_resp.data[3] - 40;
              flagRead = true;
              break;
            case 0x11: // TP
              TP = message_resp.data[3] * 100 / 255;
              flagRead = true;
              break;
            }
          }
        }
      }
      if (flagRead) {
        ultimoMensajeECU = millis();
        break; // Salir del while si ya leímos
      }
    }
  }

  // Verificar timeout de comunicación con la ECU (moto apagada / switch
  // cerrado)
  if (millis() - ultimoMensajeECU > 2000) {
    RPM = 0;
    VSS = 0;
    LOAD_PTC = 0;
    TP = 0;
    MAP = 0.0;
    IAT = 0;
    consumoLh = 0.0;
    kmLInstantaneo = 0.0;
  }
}

void GPS() {
  if (!vbox.isConnected()) {
    timegps = "--:--:--";
    sat = 0;
    hdop = 0.0;
    lat = 0.0;
    lon = 0.0;
    vel = 0.0;
    // dism y diskm no se enceran a 0 intencionalmente para no dañar tu cálculo
    // actual de distancia parcial.
    accX = 0.0;
    accY = 0.0;
    alt = 0.0;
    bat = 0;
    batPic = 0;      // Icono pbat vacío cuando no hay conexión
    calidad_num = 7; // Icono psig tachado (7) cuando no hay señal o conexión
    return;
  }

  vbox.getLocalTime(h, m, s); // Por defecto UTC-5
  timegps = String(h) + ":" + String(m) + ":" + String(s);
  sat = vbox.satellites();
  hdop = vbox.hdop();
  lat = vbox.latitude();
  lon = vbox.longitude();
  vel = vbox.speedKmh();
  dism = vbox.distanceMeters();
  diskm = dism / 1000;
  accX = vbox.accelLongG();
  accY = vbox.accelLatG();
  alt = vbox.heightM();
  bat = vbox.batteryMinutes();
  if (bat >= 500) {
    bat = 100;
  } else {
    bat = (bat * 100 / 500);
  }
  batPic = bat / 20;
  if (batPic > 4)
    batPic = 4;

  // Obtener calidad numérica del HDOP desde la librería
  String calidad_str = vbox.hdopQuality();
  if (calidad_str == "Excelente") {
    calidad_num = 11;
  } else if (calidad_str == "Bueno") {
    calidad_num = 10;
  } else if (calidad_str == "Moderado") {
    calidad_num = 9;
  } else if (calidad_str == "Pobre") {
    calidad_num = 8;
  }
}

void MICRO_SD() {
  sd.startLine();
  sd.addValue(timegps);     // 1   Hora
  sd.addValue(sat, 0);      // 2   Satelites
  sd.addValue(lat, 6);      // 3   Latitud
  sd.addValue(lon, 6);      // 4   Longitud
  sd.addValue(vel, 2);      // 5   Velocidad
  sd.addValue(VSS, 0);      // 6   Velocidad Moto
  sd.addValue(alt, 0);      // 7   Altura
  sd.addValue(accX, 3);     // 8   Aceleracion X
  sd.addValue(accY, 3);     // 9   Aceleracion Y
  sd.addValue(RPM, 0);      // 10  RPM
  sd.addValue(TP, 0);       // 11  TPS
  sd.addValue(ECT, 0);      // 12  Temperatura Motor
  sd.addValue(LOAD_PTC, 0); // 13  Carga Motor
  sd.addValue(hdop, 2);     // 14  Calidad
  sd.addValue(bat, 0);      // 15  Bateria
  sd.addValue(dism, 2);     // 16  Distancia
  // sd.addValue(pulsador, 0);             //17  Pulsador

  static unsigned long lastPulsadorTime = 0;
  int logPulsador = 0;

  if (pulsadorSD == 100) {
    if (millis() - lastPulsadorTime > 2000) {
      logPulsador = 100;
      lastPulsadorTime = millis();
    }
    pulsadorSD = 0;
  }
  sd.addValue(logPulsador, 0);         // 17  Pulsador
  sd.addValue(distanciaParcial, 2);    // 18  Distancia Parcial
  sd.addValue(promedioVelocidad, 2);   // 19  Promedio Velocidad
  sd.addValue(promedioAceleracion, 3); // 20  Promedio Aceleracion
  sd.addValue(consumoLh, 2);           // 21  Consumo L/H
  sd.addValue(litrosConsumidos, 4);    // 22  Consumo Acumulado L
  sd.addValue(kmLInstantaneo, 2);      // 23  Km/L Instantáneo
  sd.addValue(kmLPromedio, 2);         // 24  Km/L Promedio
  sd.endLine();

  static int count = 0;
  if (++count >= 10) {
    sd.flush();
    count = 0;
    // Serial.println("Guardado en SD");
  }
}

void NUEVO_ARCHIVO_SD() {
  Serial.println("Cerrando archivo actual y creando uno nuevo...");

  // 1. Cerrar el archivo actual
  sd.close();
  delay(100);

  // 2. Volver a llamar a begin() de la SD. La librería automáticamente buscará
  // el siguiente número de archivo disponible y lo creará.
  if (!sd.begin(CS_PIN, 10)) {
    Serial.println("Error al crear el nuevo archivo SD.");
    return;
  }

  // 3. Tomar el nombre del nuevo archivo y actualizar en la pantalla
  archivo = sd.getFileName();
  pnext.print("tfil.txt=\"" + archivo + "\"\xFF\xFF\xFF");

  // 4. Escribir los encabezados en el nuevo archivo
  sd.startLine();
  for (int i = 0; i < NUM_VARIABLES; i++) {
    sd.addValue(ENCABEZADOS[i]);
  }
  sd.endLine();
  sd.flush();

  Serial.println("¡Nuevo archivo creado exitosamente: " + archivo + "!");
}

void PANTALLA() {
  // --- 1. PROCESAR COMANDOS ENTRANTES DE NEXTION (SIN DELAYS) ---
  static uint8_t nexBuffer[7];
  static uint8_t nexIndex = 0;

  while (pnext.available()) {
    uint8_t b = pnext.read();

    // Si el buffer está vacío, esperamos iniciar con 0x65 (Touch Event)
    if (nexIndex == 0) {
      if (b == 0x65) {
        nexBuffer[nexIndex++] = b;
      }
    } else {
      // Guardar el resto de los bytes de la trama
      nexBuffer[nexIndex++] = b;

      // La trama del Touch Event de Nextion siempre tiene exactamente 7 bytes:
      // [0x65] [PageID] [ComponentID] [Event] [0xFF] [0xFF] [0xFF]
      if (nexIndex == 7) {
        // Verificar si los últimos 3 bytes son el terminador 0xFF 0xFF 0xFF
        if (nexBuffer[4] == 0xFF && nexBuffer[5] == 0xFF &&
            nexBuffer[6] == 0xFF) {
          uint8_t pageId = nexBuffer[1];
          uint8_t componentId = nexBuffer[2];
          uint8_t eventType = nexBuffer[3]; // 0=Release, 1=Press
          // Serial.println("Page ID: " + String(pageId) +
          //                ", Component ID: " + String(componentId) +
          //                ", Event Type: " + String(eventType));

          // Si es el componente ID 15 (0x0F hex) y es un Press (0x01)
          // Si es el componente ID 18 (0x12 hex) y es un Press (0x01)
          if (componentId == 0x12 && eventType == 0x01) {
            Serial.println(
                "¡Botón de Nuevo Archivo presionado en la pantalla (ID 18)!");
            NUEVO_ARCHIVO_SD();
          }
        } else {
          // Si el terminador no coincide, algo salió mal (desincronización).
          // Imprimimos para depurar opcionalmente o ignoramos.
        }

        // Sin importar si fue exitoso o no, reseteamos el buffer para la
        // próxima instrucción
        nexIndex = 0;
      }
    }
  }

  // --- 2. ENVIAR DATOS A NEXTION ---
  static int sat_anterior = -1; // Inicializar con valor inválido
  if (sat != sat_anterior) {
    actualizarTexto("tsat", String(sat));
    sat_anterior = sat; // Actualizar valor anterior
  }
  // actualizarTexto("tsat", String(sat));
  static String timegps_anterior = ""; // Inicializar con valor inválido
  if (timegps != timegps_anterior) {
    actualizarTexto("ttim", timegps);
    timegps_anterior = timegps; // Actualizar valor anterior
  }
  // actualizarTexto("ttim", timegps);
  // Flag de actualización: solo enviar si cambia el valor
  static int calidad_anterior = -1; // Inicializar con valor inválido
  if (calidad_num != calidad_anterior) {
    pnext.print("psig.pic=");
    pnext.print(calidad_num);
    pnext.print("\xFF\xFF\xFF");
    calidad_anterior = calidad_num; // Actualizar valor anterior
  }
  // Flag de actualización para batería
  static int batPic_anterior = -1;
  if (batPic != batPic_anterior) {
    pnext.print("pbat.pic=");
    pnext.print(batPic);
    pnext.print("\xFF\xFF\xFF");
    batPic_anterior = batPic;
  }
  // Aqui el boton
  static int pulsador_anterior = -1;
  if (pulsador != pulsador_anterior) {
    pnext.print("pbut.pic=");
    pnext.print(pulsador);
    pnext.print("\xFF\xFF\xFF");
    pulsador_anterior = pulsador;
  }

  // Aqui el texto TPS
  //  actualizarTexto("ttps", String(TP));

  // Aqui el texto de rpm con filtro de cambio
  static int rpm_anterior = -1;
  if (RPM != rpm_anterior) {
    actualizarTexto("trpm", String(RPM));
    rpm_anterior = RPM;
  }

  // Aqui la barra de progreso de TPS
  //  pnext.print("jtps.val=");
  //  pnext.print(TP);
  //  pnext.print("\xFF\xFF\xFF");

  // Aqui el texto de velocidad con filtro (cambio mayor a 0.1)
  static float vel_anterior = -1.0;
  if (abs(vel - vel_anterior) > 0.1) {
    actualizarTexto("tspe", String(vel, 1));
    vel_anterior = vel;
  }

  // Aqui el texto de distancia con filtro (cambio mayor a 0.01 km = 10m)
  static float todo_anterior = -1.0;
  if (abs(diskm - todo_anterior) > 0.01) {
    actualizarTexto("todo", String(diskm, 2));
    todo_anterior = diskm;
  }

  // Aqui el texto de distancia parcial con filtro (cambio mayor a 1.0 m, se
  // muestra con 0 decimales)
  static double ttri_anterior = -1.0;
  if (abs(distanciaParcial - ttri_anterior) > 1.0) {
    actualizarTexto("ttri", String(distanciaParcial, 0));
    ttri_anterior = distanciaParcial;
  }

  // Aqui el texto de temperatura
  // actualizarTexto("tetemp", String(VSS));

  // Aqui el texto de carga de motor
  // actualizarTexto("tload", String(LOAD_PTC));

  // --- ACTUALIZACIÓN DE PANTALLA NEXTION CON DATOS DE CONSUMO ---
  // 1. Consumo Instantáneo en L/h (objeto: tcon)
  static float consumoLh_anterior = -1.0;
  if (abs(consumoLh - consumoLh_anterior) > 0.05) {
    actualizarTexto("tcon", String(consumoLh, 2), " L/h");
    consumoLh_anterior = consumoLh;
  }

  // 2. Consumo Acumulado en Litros (objeto: tlit)
  static float litros_anterior = -1.0;
  if (abs(litrosConsumidos - litros_anterior) > 0.01) {
    actualizarTexto("tlit", String(litrosConsumidos, 2), " L");
    litros_anterior = litrosConsumidos;
  }

  // 3. Consumo Promedio en km/L (objeto: tkml)
  static float kmLPromedio_anterior = -1.0;
  if (abs(kmLPromedio - kmLPromedio_anterior) > 0.1) {
    actualizarTexto("tkml", String(kmLPromedio, 1), " km/L");
    kmLPromedio_anterior = kmLPromedio;
  }
}