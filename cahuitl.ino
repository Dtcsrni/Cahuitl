/*********
Erick Renato Vega Ceron
Armsys.Tech
Proyecto para medir humedad y temperatura utilizando un sensor DHT11 y mostrar los datos junto con 
texto de confort relativo en una pantalla Oled de 128x64
*********/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

#define SCREEN_WIDTH 128 // Ancho de la pantalla OLED, en píxeles
#define SCREEN_HEIGHT 64 // Alto de la pantalla OLED, en píxeles

// Declaración para una pantalla SSD1306 conectada a través de I2C (pines SDA, SCL)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 2     // Pin digital conectado al sensor DHT

// Descomenta el tipo de sensor que estás utilizando:
#define DHTTYPE    DHT11     // DHT 11 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);

  dht.begin();

  // Inicializar la pantalla SSD1306
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Fallo en la asignación de SSD1306"));
    for (;;) {
      // Se repite indefinidamente si falla la inicialización de la pantalla
    }
  }
  delay(200); 
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(40, 30); // Posición del texto en la pantalla (0, 0)
  display.print("ARMSYS.TECH");
   display.display(); 
    delay(2000);  // Espera 2 segundos entre lecturas
  display.clearDisplay();
}

void generarMensajeConfort(int temperatura, int humedad) {
  // Establecer la posición del cursor en la pantalla para mostrar el mensaje de confort

  display.setCursor(2, 17);

  // Determinar el mensaje de confort según la temperatura
  if (  temperatura <= 0) {
    display.print("Congelante (!!!)");
  } else if (temperatura > 0 && temperatura <= 5) {
    display.print("Muy Frio (!)");
  } else if (temperatura > 5 && temperatura <= 12) {
    display.print("Frio");
  } else if (temperatura > 12 && temperatura <= 20) {
    display.print("Comodo");
  } else if (temperatura > 20 && temperatura <= 25) {
    display.print("Calido");
  } else if (temperatura > 25 && temperatura <= 30) {
    display.print("Caluroso");
  } else if (temperatura > 30 && temperatura < 40) {
    display.print("Muy caluroso (!)");
  } else if (temperatura >= 40) {
    display.print("Extremo caluroso (!!!)");
  }
  // Establecer la posición del cursor en la pantalla para mostrar el mensaje de confort de humedad

  display.setCursor(2, 28);

  // Determinar el mensaje de confort según la humedad
  if (humedad <= 30) {
    display.print("Muy seco (!)");
  } else if (humedad > 30 && humedad <= 35) {
    display.print("Seco");
  } else if (humedad > 35 && humedad <= 40) {
    display.print("Ligeramente seco");
  } else if (humedad > 40 && humedad <= 50) {
    display.print("Comodo");
  } else if (humedad > 50 && humedad <= 60) {
    display.print("Ligeramente humedo");
  } else if (humedad > 60 && humedad <= 65) {
    display.print("Humedo");
  } else if (humedad > 65) {
    display.print("Muy humedo (!)");
  }

 
}

void loop() {
  // Retardo de 5 segundos
  delay(5000);

  // Leer temperatura y humedad
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Verificar si se pudieron leer los valores correctamente
  if (isnan(h) || isnan(t)) {
    Serial.println("¡Error al leer el sensor DHT!");
  }

  // Limpiar la pantalla
  display.clearDisplay();

  // Mostrar temperatura en la pantalla OLED
 
  display.setTextSize(1);
  display.setCursor(2, 0);
  display.print("Temperatura: ");
  display.setTextSize(1);
  display.setCursor(80, 0);
  display.print(t);
  display.print(" ");
  display.cp437(true);
  display.write(167);
  display.print("C");
  // Mostrar humedad en la pantalla OLED
  display.setCursor(2, 8);
  display.print("Humedad: ");
  display.setCursor(80, 8);
  display.print(h);
  display.print(" %");

  // Generar el mensaje de confort y mostrarlo en la pantalla
if (!isnan(h) && !isnan(t)) {
  generarMensajeConfort(round(t), round(h));
    
  } else{

     display.setCursor(2, 25);
     display.print("Sin datos");

  }
  

  // Mostrar en la pantalla OLED
  display.display();
}
