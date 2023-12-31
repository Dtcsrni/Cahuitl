/*
-------------------------------------
-    Erick Renato Vega Cerón        -
- Maestría en Internet de las Cosas -
-            UAEH                   -
-    https://github.com/Dtcsrni     -
-------------------------------------
----------------------------------------------------------------------------------------------------------------------------------------------------
-Código para controlar un nodo de sensado y actuación donde se midan temperatura, humedad (DHT11) y presencia viva (PIR)                           -
-Los datos se muestran en la pantalla física cuando hay alguien cerca para verlo, y al no detectarse presencia humana por un tiempo considerable   -
-se apaga la pantalla para ahorrar energía y mejorar la vida de la pantalla OLED.                                                                  -
-Se publica en un broker de HiveQ los datos sensados en formato JSON y se controla el funcionamiento del ventilador con una app (IoT MQTT Panel)   -
----------------------------------------------------------------------------------------------------------------------------------------------------
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <TinyGPSPlus.h>
#include <ctime>


#define BUILTIN_LED 13
#define LED_CONEXIONES 15
#define LED_ACTUADORES 12
#define RELE 33
#define PIR 14
#define GPS_SERIAL Serial2

#define SCREEN_WIDTH 128  // Ancho de la pantalla OLED, en píxeles
#define SCREEN_HEIGHT 64  // Alto de la pantalla OLED, en píxeles


// Declaración para una pantalla SSD1306 conectada a través de I2C (pines SDA, SCL)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//////////////////Sensor DHT (Temperatura y Humedad)/////////////////////////////////////
#define DHTPIN 2       // Pin digital conectado al sensor DHT
#define DHTTYPE DHT11  // Sensor utilizado: DHT 11 (AM2302)

//Credenciales Wifi
const char* ssid = "ArmsysTech";
const char* password = "sjmahpe122512";

//Datos MQTT
//Broker Público
const char* mqtt_servidor_publico = "mqtt-dashboard.com";
//Broker Local
const char* mqtt_servidor_local = "192.168.1.70";

const char* topic_Sensores = "iot/UAEH/ErickVega/Estudio/Sensores";
const char* topic_Actuadores = "iot/UAEH/ErickVega/Estudio/Actuadores";
const String nombreClienteMQTT = "eirikr";

static const int GPS_RXPin = 15, GPS_TXPin = 16;
static const uint32_t GPSBaud = 9600;


TinyGPSPlus gps;                 // Crea una instancia de TinyGPSPlus
WiFiClient espClient;            //Definición de cliente WiFi
PubSubClient client(espClient);  //Cliente MQTT
DHT dht(DHTPIN, DHTTYPE);

////////////////////////////////////Variables/////////////////////////////////////////////////////////////////////
long ultimoMensajeEnviado = 0;
char mensaje[50];
IPAddress ip_dispositivo;
int value = 0;
bool presenciaHumanaCercana = true;
bool sensorDesconectado = false;
float temperaturaActual = 0;
float temperaturaAnterior = 0;
float humedadActual = 0;
float humedadAnterior = 0;
bool ventilador = false;
bool modoAutomatico = false;
bool cambioEstadoLEDS = false;
bool cambioEstadoPantalla = false;
bool estadoConexion_MQTT = false;
bool estadoConexion_WIFI = false;
bool estadoPantalla = false;
bool GPS_habilitado = false;
byte estado_LED_Conexion = LOW;
byte estado_LED_Actuadores = LOW;
byte estadoPrevioLEDS = LOW;
unsigned long tiempoInicial = 0;
unsigned long tiempoActual = 0;
unsigned long tiempoPrevio_LED_Conexion = 0;    //almacena la última vez que se revisó el estado del led;
unsigned long tiempoPrevio_LED_Actuadores = 0;  //almacena la última vez que se revisó el estado del led;
unsigned long tiempoPrevio_Reconexion = 0;
unsigned long tiempoPrevio_Sensado_Presencia = 0;
unsigned long tiempoPrevio_Pantalla_Descando = 0;
unsigned long tiempoPrevio_Sensado_GPS = 0;
double latitud = 0.0;
double longitud = 0.0;
unsigned long tiempoSistema = 0;

const float temperatura_activacion_automatica = 21.5;
/////////////////////////////////Intervalos de acciones periódicas////////////////////////////////////////////////////////////////////////
const int intervalo_lectura_sensores = 5000;       //Intervalo entre lecturas de sensores
const int intervalo_sensado_presencia = 5000;      //Tiempo entre sensado de presencia
const int intervalo_descanso_pantalla = 20000;     //Tiempo transcurrido entre cada descanso de la pantalla OLED
const int duracion_descanso_pantalla = 2000;       //Tiempo que la pantalla descansará antes de encender de nuevo
const int intervalo_reconexion = 4000;             //Tiempo entre intentos de conexión
const int intervalo_verificacion_conexion = 5000;  //Tiempo entre verificaciones de conexión periódicas
const int intervalo_mensajes = 5000;               //Tiempo entre mensajes de MQTT
const int intervalo_parpadeo_LED = 500;            //Tiempo entre parpadeos de led
const int duracionEncendido_LED_Conexion = 500;    //Duración de LED encendido
const int diracionEncendido_LED_Actuadores = 500;  //Duración de LED encendido
const int duracion_pantalla_encendida = 15000;     //Duración máxima de la pantalla encendida
const int intervalo_actualizacion_GPS = 5000;
/////////////////////JSON///////////////////////////////////////////////////////////////////////////////////////////////////////////////
StaticJsonDocument<16> datosActuadores;
//////////////////////////////////////////////Iconos de GUI////////////////////////////////////////////////////////////////
// 'icono_armsys', 50x50px
const unsigned char icono_armsys[350] PROGMEM = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc0, 0xff, 0xff, 0xb1, 0x8c, 0x7f, 0xff, 0xc0, 0xff, 0xff,
  0x10, 0x00, 0x3f, 0xff, 0xc0, 0xff, 0xff, 0x50, 0x00, 0x3f, 0xff, 0xc0, 0xff, 0xff, 0x90, 0x00,
  0x3f, 0xff, 0xc0, 0xff, 0xff, 0x10, 0x84, 0x3f, 0xff, 0xc0, 0xff, 0xe0, 0x00, 0x00, 0x03, 0xff,
  0xc0, 0xff, 0xc0, 0x00, 0x00, 0x03, 0xff, 0xc0, 0xff, 0x80, 0x00, 0x00, 0x01, 0xff, 0xc0, 0xff,
  0x07, 0xff, 0xff, 0xf0, 0xff, 0xc0, 0xfe, 0x0f, 0xff, 0xff, 0xf8, 0x7f, 0xc0, 0xfc, 0x1f, 0xff,
  0xff, 0xfc, 0x7f, 0xc0, 0xfc, 0x3c, 0x60, 0x60, 0x02, 0x4f, 0xc0, 0xfc, 0x7c, 0x20, 0x00, 0x02,
  0x4f, 0xc0, 0xfc, 0x7c, 0x20, 0x01, 0x02, 0x4f, 0xc0, 0xfc, 0x7c, 0x60, 0x00, 0x02, 0x0f, 0xc0,
  0x80, 0x7c, 0x60, 0x00, 0x02, 0x00, 0x40, 0x80, 0x78, 0x00, 0x00, 0x02, 0x00, 0x40, 0x80, 0x70,
  0x00, 0x40, 0x02, 0x00, 0x40, 0xf8, 0x72, 0x07, 0x0c, 0xb2, 0x4f, 0xc0, 0xc0, 0x72, 0x09, 0x0f,
  0xf2, 0x00, 0xc0, 0x80, 0x70, 0x10, 0x00, 0x02, 0x00, 0x40, 0x80, 0x70, 0x00, 0x00, 0x02, 0x00,
  0x40, 0x80, 0x78, 0x20, 0x00, 0x02, 0x00, 0x40, 0x80, 0x7c, 0x00, 0x00, 0x02, 0x40, 0xc0, 0xc4,
  0x7c, 0x00, 0x18, 0x02, 0x09, 0xc0, 0x80, 0x7c, 0x00, 0x18, 0x02, 0x01, 0x40, 0x80, 0x78, 0x00,
  0x18, 0x02, 0x00, 0x40, 0x80, 0x70, 0x00, 0x18, 0x06, 0x00, 0x40, 0x80, 0x70, 0x00, 0x18, 0x06,
  0x40, 0xc0, 0xfc, 0x70, 0x10, 0x18, 0x0e, 0x0f, 0xc0, 0x80, 0x60, 0x00, 0x3c, 0x3e, 0x01, 0x40,
  0x80, 0x40, 0x08, 0x7e, 0x7e, 0x00, 0x40, 0x80, 0x40, 0x1c, 0xff, 0xfe, 0x00, 0x40, 0xfc, 0x40,
  0x1f, 0xff, 0xfe, 0x47, 0xc0, 0xfc, 0x40, 0x1f, 0xff, 0xfe, 0x4f, 0xc0, 0xfc, 0x00, 0x1f, 0xff,
  0xfe, 0x0f, 0xc0, 0xfc, 0x00, 0x1f, 0xff, 0xfc, 0x0f, 0xc0, 0xfc, 0x80, 0x7f, 0xff, 0xf8, 0x0f,
  0xc0, 0xfc, 0x41, 0xff, 0xff, 0xf0, 0x1f, 0xc0, 0xfe, 0x23, 0xdf, 0xff, 0x70, 0x3f, 0xc0, 0xff,
  0x90, 0x00, 0x00, 0x00, 0x7f, 0xc0, 0xff, 0x80, 0x00, 0x00, 0x00, 0xff, 0xc0, 0xff, 0xc0, 0x00,
  0x00, 0x01, 0xff, 0xc0, 0xff, 0xf9, 0x8c, 0x63, 0x03, 0xff, 0xc0, 0xff, 0xff, 0x58, 0x81, 0x3f,
  0xff, 0xc0, 0xff, 0xff, 0x50, 0x00, 0x3f, 0xff, 0xc0, 0xff, 0xff, 0x18, 0x43, 0x3f, 0xff, 0xc0,
  0xff, 0xff, 0x10, 0x84, 0x3f, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
// 'icono_iot', 20x15px
const unsigned char icono_iot[45] PROGMEM = {
  0x1f, 0x8f, 0xc0, 0x1f, 0x77, 0xc0, 0x1f, 0x8f, 0xc0, 0x1f, 0xff, 0xc0, 0x1f, 0x07, 0xc0, 0x16,
  0x7b, 0x40, 0x02, 0x8a, 0x00, 0x0a, 0xaa, 0x80, 0x02, 0x8a, 0x00, 0x16, 0xfb, 0x40, 0x1f, 0x07,
  0xc0, 0x1f, 0xff, 0xc0, 0x1f, 0x8f, 0xc0, 0x1f, 0x57, 0xc0, 0x1f, 0x8f, 0xc0
};
const unsigned char icono_GPS[30] PROGMEM = {
  // 'gps, 15x15px
  0x00, 0x00, 0x03, 0x80, 0x07, 0xc0, 0x0f, 0xe0, 0x0e, 0xe0, 0x0c, 0x60, 0x0e, 0xe0, 0x0f, 0xe0,
  0x07, 0xc0, 0x03, 0x80, 0x0d, 0xe0, 0x1c, 0x30, 0x0f, 0xe0, 0x03, 0x80, 0x00, 0x00
};
// 'icono_ventilador', 40x40px
const unsigned char icono_ventilador[30] PROGMEM = {
  // 'icono_ventilador, 15x15px
  0xfc, 0x3e, 0xe0, 0x1e, 0xc8, 0x16, 0xdc, 0x12, 0x86, 0x18, 0x82, 0x38, 0x00, 0x20, 0x00, 0x00,
  0x00, 0x80, 0x0c, 0x00, 0xb8, 0x42, 0x90, 0x7e, 0xc0, 0x32, 0xe0, 0x06, 0xf8, 0x1e
};
const unsigned char icono_automatico[30] PROGMEM = {
  // 'icono_automatico, 15x15px
  0xf9, 0xfe, 0xf1, 0xfe, 0xf1, 0xfe, 0xe3, 0xfe, 0xc3, 0xfe, 0xc0, 0x3e, 0x80, 0x3e, 0x00, 0x3e,
  0x00, 0x66, 0x00, 0xe6, 0xf0, 0xc2, 0xe1, 0xda, 0xe3, 0xda, 0xe3, 0x80, 0xe7, 0xbc
};
const unsigned char icono_wifi[30] PROGMEM = {
  // 'wifi, 16x15px
  0x00, 0x00, 0x00, 0x00, 0x07, 0xe0, 0x3f, 0xfc, 0x78, 0x1e, 0x60, 0x06, 0x01, 0x80, 0x0f, 0xf0,
  0x0c, 0x30, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};
////////////////////////////////////SETUP/////////////////////////////////////////////////////////////////////////////////////
////////////Configuración del programa, esta parte se ejecuta sólo una vez al energizarse el sistema/////////////////////
void setup() {
  //// Inicialización del programa
  // Inicia comunicación serial
  Serial.begin(2000000);
  dht.begin();  //Se inicializa el sensor DHT11
  GPS_SERIAL.begin(GPSBaud);
  /// Configuración de pines //
  pinMode(BUILTIN_LED, OUTPUT);  // Se configuran los pines de los LEDs indicadores como salida
  pinMode(LED_CONEXIONES, OUTPUT);
  pinMode(LED_ACTUADORES, OUTPUT);
  pinMode(RELE, OUTPUT);  // Inicializa el rele como output con resistencia Pull- Up interna
  pinMode(PIR, INPUT);    // Inicializa el rele como input con resistencia Pull- Up interna
  // Condiciones Iniciales //
  digitalWrite(BUILTIN_LED, LOW);  // Se inicia con los LEDs indicadores apagados
  digitalWrite(LED_CONEXIONES, LOW);
  digitalWrite(LED_ACTUADORES, LOW);
  digitalWrite(RELE, LOW);  // Se inicia con el relé apagado

  // Inicializar la pantalla SSD1306
  Serial.println("Inicializando pantalla ");
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Fallo en la asignación de SSD1306"));
    for (;;) {
      // Se repite indefinidamente si falla la inicialización de la pantalla
    }
  }
  delay(350);
  display.cp437(true);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.clearDisplay();
  ///////////////////Intro
  // Dibuja logo de ArmsysTech/
  display.drawBitmap(0, 10, icono_armsys, 50, 50, WHITE);
  colocarTextoEnPantalla(55, 10, "Erick Renato");
  colocarTextoEnPantalla(55, 25, "Vega Ceron");
  colocarTextoEnPantalla(90, 35, "IoT");
  display.display();
  delay(5000);  // Espera 5 segundos
  display.clearDisplay();
  display.display();
  // Conexion al broker MQTT
  //client.setServer(mqtt_servidor_publico, 1883);  // Se hace la conexión al servidor MQTT Público
  client.setCallback(callback);                 //Se activa la función que permite recibir mensajes de respuesta
  client.setServer(mqtt_servidor_local, 1883);  // Se hace la conexión al servidor MQTT Local

}  // Fin del void setup()
////////////////////////////////////////////////////////////
//////////////////////////Funciones///////////////////////////////////////////////////////////////////////////////////
void colocarTextoEnPantalla(byte fila, byte columna, String mensaje) {
  //Se coloca el cursor en la fila y columna indicadas
  if (presenciaHumanaCercana) {
    display.setCursor(fila, columna);
    display.print(mensaje);
  }
}
void actualizarGPS() {  //Obtiene la posición del GPS solo en el intervalo indicado
  //Si está apagado
  if (tiempoActual - tiempoPrevio_Sensado_GPS >= intervalo_actualizacion_GPS) {  //Y ya ha pasado el intervalo entre parpadeos
    obtenerPosicionTiempoGPS(latitud, longitud);                                 //Se obtiene la posición GPS y el tiempo del sistema
    tiempoPrevio_Sensado_GPS += intervalo_actualizacion_GPS;                     //Y se registra cuándo sucedió el último cambio
  }
}
////////// Función para obtener la posición GPS
void obtenerPosicionTiempoGPS(double& lat, double& lon) {
  // Limpia las variables de posición
  lat = 0.0;
  lon = 0.0;
  struct tm tm;
  const int ajusteUTC6 = -6 * 3600;  // Ajuste para UTC-6 (3600 segundos por hora, 6 horas)
  // Espera hasta que se obtenga una nueva posición válida
  while (GPS_SERIAL.available() > 0) {
    if (gps.encode(GPS_SERIAL.read())) {
      if (gps.location.isUpdated()) {
        // Actualiza las variables de posición
        lat = gps.location.lat();
        lon = gps.location.lng();
      }
    }
  }
  if (gps.date.isValid() && gps.time.isValid()) {
    // Obtener la fecha y la hora del módulo RTC del GPS
    int year = gps.date.year();
    byte month = gps.date.month();
    byte day = gps.date.day();
    byte hour = gps.time.hour();
    byte minute = gps.time.minute();
    byte second = gps.time.second();

    // Crear un objeto de tipo tm (estructura de tiempo estándar en C++)

    tm.tm_year = year - 1900;  // Ajustar el año según la convención de tm
    tm.tm_mon = month - 1;     // Ajustar el mes según la convención de tm
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tiempoSistema = mktime(&tm);  //Se unifica el struct del tiempo para convertirlo en un timestamp y enviarlo por MQTT
    // Ajustar el timestamp a la zona horaria UTC-6
    tiempoSistema = tiempoSistema + ajusteUTC6;
  }
  // Si hay una posición válida, se habilita el estado del GPS
  if (lat != 0.0 && lon != 0.0) {
    GPS_habilitado = true;
  } else {
    GPS_habilitado = false;
  }
}
/////Conmutar la activación de los LED si hay algún cambio en el estado de alguno
void switchLEDS() {
  if (cambioEstadoLEDS) {
    digitalWrite(LED_CONEXIONES, estado_LED_Conexion);
    digitalWrite(LED_ACTUADORES, estado_LED_Actuadores);
    cambioEstadoLEDS = false;
  }
}
/////Se revisa si el estado del LED Wifi ha cambiado y se actualiza acorde
void actualizarEstado_LED_Conexiones() {
  if (WiFi.status() != WL_CONNECTED) {
    if (estado_LED_Conexion == LOW) {                                            //Si está apagado
      if (tiempoActual - tiempoPrevio_LED_Conexion >= intervalo_parpadeo_LED) {  //Y ya ha pasado el intervalo entre parpadeos
        estadoPrevioLEDS = estado_LED_Conexion;
        estado_LED_Conexion = HIGH;                           //Se enciende
        cambioEstadoLEDS = true;                              //Se cambia el estado
        tiempoPrevio_LED_Conexion += intervalo_parpadeo_LED;  //Y se registra cuándo sucedió el último cambio
      }
    } else {                                                                             //Si está encendido
      if (tiempoActual - tiempoPrevio_LED_Conexion >= duracionEncendido_LED_Conexion) {  //Y ya ha cumplido la duración de encendido determinada
        estadoPrevioLEDS = estado_LED_Conexion;
        estado_LED_Conexion = LOW;                                    //Se apaga
        cambioEstadoLEDS = true;                                      //Se actualiza el estado
        tiempoPrevio_LED_Conexion += duracionEncendido_LED_Conexion;  ////Se registra el último cambio de estado
      }
    }
  } else {
    estado_LED_Conexion = LOW;
    cambioEstadoLEDS = true;
  }
}
/////Se revisa si el estado del LED indicador de actuación ha cambiado y se actualiza acorde
void actualizarestado_LED_Actuadores() {
  if (ventilador) {
    if (estado_LED_Actuadores == LOW) {                                            //Si está apagado
      if (tiempoActual - tiempoPrevio_LED_Actuadores >= intervalo_parpadeo_LED) {  //Y ya ha pasado el intervalo entre parpadeos
        estado_LED_Actuadores = HIGH;                                              //Se enciende
        cambioEstadoLEDS = true;                                                   //Se cambia el estado
        tiempoPrevio_LED_Actuadores += intervalo_parpadeo_LED;                     //Y se registra cuándo sucedió el último cambio
      }
    } else {                                                                                 //Si está encendido
      if (tiempoActual - tiempoPrevio_LED_Actuadores >= diracionEncendido_LED_Actuadores) {  //Y ya ha cumplido la duración de encendido determinada
        estado_LED_Actuadores = LOW;                                                         //Se apaga
        cambioEstadoLEDS = true;                                                             //Se actualiza el estado
        tiempoPrevio_LED_Actuadores += diracionEncendido_LED_Actuadores;                     ////Se registra el último cambio de estado
      }
    }
  } else {
    estado_LED_Actuadores = LOW;
    cambioEstadoLEDS = true;
  }
}
/////Se muestra en pantalla textos de sensación relativa con base en las mediciones
void mostrarSensacionRelativa(int temperatura, int humedad) {
  // Determinar el mensaje de confort según la temperatura
  if (temperatura <= 0) {
    colocarTextoEnPantalla(2, 50, "Congelante (!!!)");
  } else if (temperatura > 0 && temperatura <= 5) {
    colocarTextoEnPantalla(2, 50, "Muy Frio (!)");
  } else if (temperatura > 5 && temperatura <= 12) {
    colocarTextoEnPantalla(2, 50, "Frio");
  } else if (temperatura > 12 && temperatura <= 20) {
    colocarTextoEnPantalla(2, 50, "Comodo");
  } else if (temperatura > 20 && temperatura <= 25) {
    colocarTextoEnPantalla(2, 50, "Calido");
  } else if (temperatura > 25 && temperatura <= 30) {
    colocarTextoEnPantalla(2, 50, "Caluroso");
  } else if (temperatura > 30 && temperatura < 40) {
    colocarTextoEnPantalla(2, 50, "Muy caluroso (!)");
  } else if (temperatura >= 40) {
    colocarTextoEnPantalla(2, 50, "Extremo caluroso (!!!)");
  }
  // Determinar el mensaje de confort según la humedad
  if (humedad <= 30) {
    colocarTextoEnPantalla(2, 40, "Muy seco (!)");
  } else if (humedad > 30 && humedad <= 35) {
    colocarTextoEnPantalla(2, 40, "Seco");
  } else if (humedad > 35 && humedad <= 40) {
    colocarTextoEnPantalla(2, 40, "Ligeramente seco");
  } else if (humedad > 40 && humedad <= 50) {
    colocarTextoEnPantalla(2, 40, "Comodo");
  } else if (humedad > 50 && humedad <= 60) {
    colocarTextoEnPantalla(2, 40, "Ligeramente humedo");
  } else if (humedad > 60 && humedad <= 65) {
    colocarTextoEnPantalla(2, 40, "Humedo");
  } else if (humedad > 65) {
    colocarTextoEnPantalla(2, 40, "Muy humedo (!)");
  }
}


/////Función que revisará si hay conexión, en caso de que no la haya intenta conectarse hasta que establece conexión. Parpadeo de LED de Wifi para indicar el estado sin conexión
void mantenerConexionWIFI() {
  if (WiFi.status() != WL_CONNECTED) {
    display.clearDisplay();  //Se limpia la pantalla
    // Iniciar conexion WiFi
    Serial.println(">Conectando a Red WIFI: ");              //Anunciar intento de conexión WIFI a Serial
    Serial.print(ssid);                                      //Mostrar SSID a serial
    colocarTextoEnPantalla(0, 5, "Conectando a red WIFI:");  //Mostrar SSID en pantalla
    colocarTextoEnPantalla(0, 25, ssid);
    WiFi.begin(ssid, password);  // Iniciar intento de conexión con credenciales de wifi proporcionadas
    display.display();
    // Intentar hasta lograr conexión
    while (WiFi.status() != WL_CONNECTED) {
      tiempoActual = millis();  //Llevar conteo del tiempo transcurrido
      actualizarEstado_LED_Conexiones();
      switchLEDS();  //Activar o desactivar el led wifi conforme a su intervalo de parpadeo
    }
    if (WiFi.status() == WL_CONNECTED) {
      ip_dispositivo = WiFi.localIP();
      Serial.println("WiFi conectado exitosamente");  // Una vez lograda la conexión, se reporta al puerto serial el éxito
      Serial.println("IP:");                          // Y se reporta la IP
      Serial.println(ip_dispositivo);
      // Dibuja icono de wifi y muestra texto de confirmación a pantalla
      display.drawBitmap(20, 0, icono_wifi, 43, 15, WHITE);
      colocarTextoEnPantalla(10, 40, "|WiFi Conectado|");
      colocarTextoEnPantalla(10, 40, "IP: " + String(WiFi.localIP()));
      display.display();
    }
  }
}
///////Leer sensores y actualizar datos ambientales
void actualizarDatosAmbientales() {
  display.clearDisplay();
  //Se actualiza la última lectura de temperatura y humedad
  temperaturaAnterior = temperaturaActual;
  humedadAnterior = humedadActual;
  //Leer sensores
  temperaturaActual = dht.readTemperature();  //Leer y registrar temperatura
  humedadActual = dht.readHumidity();         //Leer y registrar humedad
                                              //Solo se muestra la pantalla cuando hay presencia humana cercana

  colocarTextoEnPantalla(2, 20, ">Temperatura: " + String(temperaturaActual));  //Mostrar la temperatura a pantalla
  // Mostrar humedad en la pantalla OLED
  colocarTextoEnPantalla(2, 30, ">Humedad: " + String(humedadActual) + " %");  //Mostrar la temperatura a pantalla
  // Generar el mensaje de confort y mostrarlo en la pantalla
  if (!isnan(humedadActual) || !isnan(temperaturaActual)) {
    mostrarSensacionRelativa(round(temperaturaActual), round(humedadActual));
    sensorDesconectado = false;
  } else {  //Si no se detecta el sensor, avisarlo en pantalla
    colocarTextoEnPantalla(0, 50, "x Sensor Desconectado");
    sensorDesconectado = true;
  }
}
///Preparar y publicar datos de sensores utilizando el protocolo MQTT en el servidor remoto en la nube
void publicar_datos_MQTT() {
  if (tiempoActual - ultimoMensajeEnviado >= intervalo_mensajes && !sensorDesconectado && GPS_habilitado) {  //si el último mensaje se envió hace más de 5 segundos
                                                                                                             ///Carga de datos de sensores y estado del ventilador
    StaticJsonDocument<200> datosSensores;
    // Agregar datos al objeto JSON
    datosSensores["sensor"] = "ESP32_Cahuitl_1";

    JsonObject datos = datosSensores.createNestedObject("datos");
    datos["temperatura"] = temperaturaActual;
    datos["humedad"] = humedadActual;
    datos["ventilador"] = ventilador;

    JsonObject ubicacion = datos.createNestedObject("ubicacion");
    ubicacion["latitud"] = latitud;
    ubicacion["longitud"] = longitud;

    JsonObject metadatos = datos.createNestedObject("metadatos");
    metadatos["ip"] = ip_dispositivo.toString();
    metadatos["timestamp"] = tiempoSistema;

    size_t length = measureJson(datosSensores);
    char payload[length + 1];  // +1 para el carácter nulo al final

    //Serializado de datos de carga útil
    serializeJson(datosSensores, payload, length + 1);
    //Publicación en cliente local
    client.publish(topic_Sensores, payload);
    Serial.println(payload);

    //Se registra el momento en el que se envía el mensaje como el último mensaje enviado
    ultimoMensajeEnviado = millis();
  }
}
//Función para activar el relé del ventilador
void ventilarArea(bool ventilar) {
  if (ventilar) {              //Si el comando es para ventilar
    digitalWrite(RELE, HIGH);  //Enviar señal de activado al Relé
    ventilador = true;         //y cambiar estado a activado
  } else {                     //Si el comando es para detener ventilación
    digitalWrite(RELE, LOW);   //Se envía señal de apagado al Relé
    ventilador = false;        //Se cambia el estado a desactivado
  }
}
// Funcion Callback, aqui se pueden poner acciones si se reciben mensajes MQTT
void callback(char* topic1, byte* payload, unsigned int length) {
  DynamicJsonDocument mensaje(128);      //Objeto JSON
  Serial.println("Mensaje recibido: ");  //Avisar de mensaje recibido por MQTT
  //Deserealizar el contenido del JSON
  deserializeJson(mensaje, payload);
  //Se registra el contenido del mensaje
  bool ventilador = mensaje["ventilador"];
  bool estadoAutomatico = mensaje["modoAutomatico"];
  //Se imprime a serial el contenido
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  //Si el topic es el mismo de los actuadores
  if (String(topic1) == topic_Actuadores) {
    if (ventilador) {                                         //Si el estado del ventilador es encendido
      ventilarArea(true);                                     //Se ventila el área
      if (modoAutomatico) {                                   //Si el modo automático está encendido mientras se recibe el comando directo de ventilación
        modoAutomatico = false;                               //Desactivar el modo automático
        Serial.println("Ventilador automático desactivado");  //Y avisar a serial
      }
    } else {                                                  //Si el estado del ventilador es apagado
      ventilarArea(false);                                    //Y se desactiva la ventilación
      if (modoAutomatico) {                                   //Si el modo automático está activado
        modoAutomatico = false;                               //Se desactiva
        Serial.println("Ventilador automático desactivado");  //Y se anuncia a serial
      }
    }
    if (estadoAutomatico) {   //Si se manda activar el modo automático
      if (!modoAutomatico) {  //Se activa el modo automático si no estaba activado y se avisa a serial
        modoAutomatico = true;
        Serial.println("Modo automático activado");
      }
    } else {                 //Si se manda desactivar
      if (modoAutomatico) {  //Se desactiva el modo automático y se avisa en serial
        modoAutomatico = false;
        Serial.println("Modo automático desactivado");
      }
    }
  }
}
void monitorearModoAutomatico() {

  if (modoAutomatico) {
    if (temperaturaActual > temperatura_activacion_automatica) {  //Si está activado el modo automático, revisar criterio de revisión
      ventilarArea(true);
    } else {
      ventilarArea(false);
    }
  }
}
// Función para mantener conectado el cliente al Broker MQTT
void mantenerConexionMQTT() {
  if (!client.connected()) {
    display.clearDisplay();                               //Limpiar Pantalla
    Serial.println("Intentando conexion a Broker MQTT");  //Avisar de intento de conexión a Broker
    colocarTextoEnPantalla(0, 0, ">Conectando a Broker MQTT");
    display.display();
  }
  while (!client.connected()) {  //Mientras que el cliente no se conecte
    tiempoActual = millis();     //Llevar conteo del tiempo transcurrido
    actualizarEstado_LED_Conexiones();
    switchLEDS();
    if (!client.connected()) {  //Intentar conexión con nombre de cliente
      if (millis() - tiempoPrevio_Reconexion >= intervalo_reconexion) {
        client.connect("eirikr");
        tiempoPrevio_Reconexion += intervalo_reconexion;
      }
      switch (client.connected()) {
        case true:  //En caso de que la conexión haya sido exitosa
          Serial.println("Conexion exitosa");
          if (presenciaHumanaCercana) {
            colocarTextoEnPantalla(5, 10, "Conexion con Broker MQTT Exitosa");
            display.drawBitmap(0, 30, icono_iot, 20, 15, WHITE);
            display.display();
          }

          // Suscribirse al tema de actuadores
          client.subscribe(topic_Actuadores);
          break;
        case false:                                                                                   //En caso contrario
          Serial.println("X Fallo en comunicacion con Broker, error rc= " + String(client.state()));  //Avisar de intento de conexión a Broker
          Serial.println("Reintentando en " + String(intervalo_reconexion) + " segundos");
          colocarTextoEnPantalla(0, 19, "X Fallo en comunicacion con Broker, error= " + String(client.state()));
          colocarTextoEnPantalla(0, 45, "Reintentando en " + String(intervalo_reconexion) + " segundos ");
          display.display();
          break;
      }
    }
  }
}
//////Función para mostrar y mantener íconos de conexión, el indicador de modo automático y actuadores en la barra de estado/////////////////
void refrescarBarraEstado() {
  if (presenciaHumanaCercana) {
    if (WiFi.status() == WL_CONNECTED) {                    //Si el WiFi está conectado
      display.drawBitmap(0, 0, icono_wifi, 16, 15, WHITE);  //mostrar icono de WiFi en pantalla
    }
    if (client.connected()) {  //Si el cliente está conectado al Broker MQTT
      colocarTextoEnPantalla(46, 0, "MQTT");
      display.setCursor(40, 0);  // Posición del texto en la pantalla (0, 0)
      display.write(4);          //Mostrar simbolo
    }
    if (ventilador) {                                              //Si el ventilador está activado
      display.drawBitmap(90, 0, icono_ventilador, 15, 15, WHITE);  //mostrar icono de ventilador en pantalla
    }
    if (modoAutomatico) {                                           //Si el modo automático está activado
      display.drawBitmap(106, 0, icono_automatico, 15, 15, WHITE);  //mostrar icono de ventilador en pantalla
    }
    if (GPS_habilitado) {                                   //Si el modo automático está activado
      display.drawBitmap(20, 0, icono_GPS, 15, 15, WHITE);  //mostrar icono de ventilador en pantalla
    }
    display.display();
  }
}

////////Función para sensar la presencia humana por medio del sensor pirométrico de forma periódica
void sensarPresenciaHumanaCercana() {
  if (millis() - tiempoPrevio_Sensado_Presencia >= intervalo_sensado_presencia) {
    presenciaHumanaCercana = digitalRead(PIR);  // Se revisa si hay presencia de un ser vivo cerca
    tiempoPrevio_Sensado_Presencia += intervalo_sensado_presencia;
  }
}
//Función para apagar o encender la pantalla dependiendo de su estado
void switchPantalla() {
  if (cambioEstadoPantalla) {
    if (estadoPantalla) {
      display.ssd1306_command(SSD1306_DISPLAYON);  // Encender pantalla
    } else {
      display.ssd1306_command(SSD1306_DISPLAYOFF);  // Encender pantalla
    }
    cambioEstadoPantalla = false;
  }
}
/////Se actualiza el estado de la pantalla si ha cumplido su periodo máximo encendida y debe cumplir el intervalo de descanso
void actualizarEstado_Pantalla() {
  if (!estadoPantalla) {                                                            //Si está apagado
    if (millis() - tiempoPrevio_Pantalla_Descando >= duracion_descanso_pantalla) {  //Y ya ha pasado el intervalo entre parpadeos
      estadoPantalla = true;                                                        //Se enciende
      cambioEstadoPantalla = true;                                                  //Se cambia el estado
      tiempoPrevio_Pantalla_Descando += duracion_descanso_pantalla;                 //Y se registra cuándo sucedió el último cambio
    }
  } else {                                                                           //Si está encendido
    if (millis() - tiempoPrevio_Pantalla_Descando >= duracion_pantalla_encendida) {  //Y ya ha cumplido la duración de encendido determinada
      estadoPantalla = false;                                                        //Se apaga
      cambioEstadoPantalla = true;                                                   //Se actualiza el estado
      tiempoPrevio_Pantalla_Descando += duracion_pantalla_encendida;                 ////Se registra el último cambio de estado
    }
  }
}
////////Loop principal
void loop() {
  /////Registrar tiempo actual en milisegundos desde el inicio del programa
  tiempoActual = millis();
  actualizarEstado_Pantalla();
  ///////////////////////Conectar WIFI y Broker MQTT
  mantenerConexionWIFI();
  mantenerConexionMQTT();
  sensarPresenciaHumanaCercana();
  actualizarGPS();
  actualizarEstado_LED_Conexiones();
  actualizarestado_LED_Actuadores();
  monitorearModoAutomatico();
  actualizarDatosAmbientales();  //refresca los datos del sensor DHT
  publicar_datos_MQTT();
  refrescarBarraEstado();  //Actualizar iconos de barra de estado
  switchLEDS();
  switchPantalla();
  client.loop();  // Loop de cliente MQTT

}  // Fin del void loop()
