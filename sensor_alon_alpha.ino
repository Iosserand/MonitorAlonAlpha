#define TFT_DC    7
#define TFT_RST   8 
#define SCR_WD   240
#define SCR_HT   240   
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Arduino_ST7789_Fast.h>
#include <DallasTemperature.h> 
#define ThermistorPin 0
#define TEMP_X 20
#define TEMP_Y 85

OneWire oneWire(ThermistorPin);
DallasTemperature sensors(&oneWire);

Arduino_ST7789 tft = Arduino_ST7789(TFT_DC, TFT_RST);
// Variavéis de Temperatura
int Vo;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
float previousTemp = -100.0;
float tempC = 0;
float minTemp = 200;
float maxTemp = 0;

// Variáveis de acionamento
int buttonPin = 2;
int relayPin = 3;


void setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(relayPin, OUTPUT);
  
  Serial.begin(9600);
  Serial.println("Starting up ...");
  tft.init(SCR_WD, SCR_HT);
  tft.fillScreen(BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(RED);
  tft.setTextSize(3);
  tft.println(" Temperatura");
  tft.setCursor(160, TEMP_Y);
  tft.setTextSize(5);
  tft.setTextColor(WHITE);  
  tft.println((char)247 );
  tft.setCursor(200, TEMP_Y);
  tft.println("C");

  tft.setCursor(40, 180);
  tft.setTextSize(2);
  tft.setTextColor(CYAN); 
  tft.println("MIN");
  printMinTempDegreesSymbol();
  printMaxTempDegreesSymbol();

  tft.setCursor(170, 180);
  tft.setTextSize(2);
  tft.setTextColor(RED); 
  tft.println("MAX");

  
}

void loop() {
  delay(1000);
  Vo = analogRead(ThermistorPin);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 332.15;
  previousTemp = tempC;
  tempC = T;
  if(tempC<minTemp)
    {
      deleteMinTemp();
      minTemp = tempC; 
    }
    if(tempC>maxTemp)
    {
      deleteMaxTemp();
      maxTemp = tempC; 
    }
    if(previousTemp!=tempC)
    {
       deletePreviousTemp();
       printTemp();
       printMinTemp();
       printMaxTemp();
    }
  if (digitalRead(buttonPin)==HIGH) {
    digitalWrite(relayPin, HIGH);
  } else {
    digitalWrite(relayPin, LOW);
  }

  ligarRelay();

}


void deletePreviousTemp()
{
  tft.setCursor(TEMP_X, TEMP_Y);
  tft.setTextSize(5);
  tft.setTextColor(BLACK);
  tft.println(previousTemp,1);
}

void printTemp()
{
  tft.setCursor(TEMP_X, TEMP_Y);
  tft.setTextSize(5);
  tft.setTextColor(WHITE);
  tft.println(tempC,1);
}

void printMinTemp()
{
  tft.setCursor(10, 210);
  tft.setTextSize(2);
  tft.setTextColor(CYAN);
  tft.println(minTemp,1);
}

void printMaxTemp()
{
  tft.setCursor(150, 210);
  tft.setTextSize(2);
  tft.setTextColor(RED);
  tft.println(maxTemp,1);
}

void deleteMaxTemp()
{
  tft.setCursor(150, 210);
  tft.setTextSize(2);
  tft.setTextColor(BLACK);
  tft.println(maxTemp,1);
}

void deleteMinTemp()
{
  tft.setCursor(10, 210);
  tft.setTextSize(2);
  tft.setTextColor(BLACK);
  tft.println(minTemp,1);
}

void printMinTempDegreesSymbol()
{
  tft.setCursor(70, 210);
  tft.setTextSize(2);
  tft.setTextColor(CYAN);
  tft.println((char)247 );
  tft.setCursor(85, 210);
  tft.println("C");
}

void printMaxTempDegreesSymbol()
{
  tft.setCursor(210, 210);
  tft.setTextSize(2);
  tft.setTextColor(RED);
  tft.println((char)247 );
  tft.setCursor(225, 210);
  tft.println("C");
}

void ligarRelay()
{
  int state_button = digitalRead(buttonPin);
  int estado_rele = digitalRead(relayPin);
  if (state_button == LOW) {
    digitalWrite(relayPin, LOW);
  } else {
    digitalWrite(relayPin, HIGH);
  }
  Serial.print("Button = ");
  Serial.print(state_button);
  Serial.println();
  Serial.print("Rele = ");
  Serial.print(estado_rele);
  Serial.println();
}