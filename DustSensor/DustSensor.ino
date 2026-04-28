#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Calibration Constants
#define COV_RATIO         0.2   // ug/m3 / mv
#define NO_DUST_VOLTAGE   400   // mv (Adjust this based on your Serial Monitor)
#define SYS_VOLTAGE       5000  

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int iled = 7;
const int vout = A0;

float density, voltage;
int adcvalue;

// Improved Filter
int Filter(int m) {
  static int _buff[10], sum;
  static bool first = true;
  if(first) {
    for(int i=0; i<10; i++) _buff[i] = m;
    sum = m * 10;
    first = false;
  }
  sum -= _buff[0];
  for(int i=0; i<9; i++) _buff[i] = _buff[i+1];
  _buff[9] = m;
  sum += _buff[9]; // Corrected line
  return sum / 10;
}

void setup() {
  pinMode(iled, OUTPUT);
  digitalWrite(iled, LOW); 
  
  Serial.begin(9600);
  Serial.println("System Start...");

  Wire.begin();
  Wire.setWireTimeout(3000, true); 

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("OLED Fail"));
  }
  
  display.clearDisplay();
  display.display();
}

void loop() {
  // 1. READ SENSOR
  digitalWrite(iled, HIGH);
  delayMicroseconds(280);
  int rawAdc = analogRead(vout);
  delayMicroseconds(40);
  digitalWrite(iled, LOW);
  
  // 2. FILTER
  adcvalue = Filter(rawAdc);

  // 3. MATH
  // (adcvalue / 1024.0 * 5000.0) is the voltage at the Nano pin
  // Multiplying by 11 accounts for the Waveshare board's internal divider
  voltage = (adcvalue * (SYS_VOLTAGE / 1024.0)) * 11.0;

  if (voltage > NO_DUST_VOLTAGE) {
    density = (voltage - NO_DUST_VOLTAGE) * COV_RATIO;
  } else {
    density = 0;
  }

  // 4. DEBUG
  Serial.print("Raw:"); Serial.print(rawAdc);
  Serial.print(" | Volt:"); Serial.print(voltage);
  Serial.print(" | Dust:"); Serial.println(density);

  // 5. OLED (No clearDisplay() to prevent flashing)
  display.setCursor(0, 10);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
  display.println("AIR QUALITY:      ");
  
  display.setCursor(0, 30);
  display.setTextSize(2);
  if (millis() < 3000) {
    display.print("Warm up...");
  } else {
    display.print(density, 1);
    display.print(" ug/m3 ");
  }
  display.display();
  
  delay(1000); 
}