#include <DHT_Async.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* Uncomment according to your sensortype. */
#define DHT_SENSOR_TYPE DHT_TYPE_11
//#define DHT_SENSOR_TYPE DHT_TYPE_21
//#define DHT_SENSOR_TYPE DHT_TYPE_22

static const int DHT_SENSOR_PIN = 2;
DHT_Async dht_sensor(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

float temperature = 0;
float humidity = 0;

void setup() {
  
  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);

  display.setCursor(40, 30); // Posición del texto en la pantalla (0, 0)
  display.print("ARMSYS.TECH");
   display.display(); 
    delay(2000);  // Espera 2 segundos entre lecturas
  display.clearDisplay();
  // Display static text
}
/*
 * Poll for a measurement, keeping the state machine alive.  Returns
 * true if a measurement is available.
 */
static bool measure_environment(float *temperature, float *humidity) {
    static unsigned long measurement_timestamp = millis();
    /* Measure once every four seconds. */
    if (millis() - measurement_timestamp > 4000ul) {
        if (dht_sensor.measure(temperature, humidity)) {
            measurement_timestamp = millis();
            return (true);
        }
    }
    return (false);
}
void loop() {
  delay(2000);  // Espera 2 segundos entre lecturas
  display.clearDisplay();

  display.setCursor(0, 0); // Posición del texto en la pantalla (0, 0)
  display.print("Humedad: ");
   display.setCursor(0, 20); // Posición del texto en la pantalla (0, 0)
  display.print("Temperatura: ");
  
    if (measure_environment(&temperature, &humidity)) {
        display.setCursor(90,0); // Posición del texto en la pantalla (0, 0)
        display.print(humidity);
        display.print(" %");  
        display.setCursor(90, 20); // Posición del texto en la pantalla (0, 0)
        display.print(temperature);
        display.print(" C");
    }
    else{
       display.setCursor(90,0); // Posición del texto en la pantalla (0, 0)
        display.print("NaN");
        display.print(" %");  
        display.setCursor(90, 20); // Posición del texto en la pantalla (0, 0)
        display.print("NaN");
        display.print(" C");
    }
    
    display.display(); 
}
