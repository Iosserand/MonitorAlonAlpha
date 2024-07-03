#include <Adafruit_GFX.h>     // Adafruit core graphics library
#include <Adafruit_ST7789.h>  // Adafruit hardware-specific library for ST7789
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>


// ST7789 TFT module connections
#define TFT_DC    5     // TFT DC  pin is connected to NodeMCU pin D1 (GPIO5)
#define TFT_RST   4     // TFT RST pin is connected to NodeMCU pin D2 (GPIO4)
#define TFT_CS    15     // TFT CS  pin is connected to NodeMCU pin D8 (GPIO15)
#define SCR_WD   240
#define SCR_HT   240   
#define TEMP_X 20
#define TEMP_Y 85 

// DHT11 connections
#define DHTPIN 2  // pin is connected to NodeMCU pin D4 (GPIO2)
#define DHTTYPE    DHT11     // DHT 11
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

// RELAY connections
#define relayPin  0
bool relayState = true;  // Estado atual do relé

// initialize ST7789 TFT library with hardware SPI module
// SCK (CLK) ---> NodeMCU pin D5 (GPIO14)
// MOSI(DIN) ---> NodeMCU pin D7 (GPIO13)
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);


//Variables for display
float previousTemp = -100.0;
float tempC = 0;
float minTemp = 200;
float maxTemp = 0;
float nominalTemperature = 25.0;  // Temperatura nominal em Celsius
float upperThreshold = nominalTemperature + 2.0;  // Limite superior
float lowerThreshold = nominalTemperature - 2.0;  // Limite inferior


// Update these with values suitable for your network.
const char* ssid = "YMSHOPFLOOR";
const char* password = "Kr4sn@ya7v3zDa";
const char* mqtt_server = "172.21.68.55";
const int mqtt_port = 3377;


WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  pinMode(relayPin, OUTPUT);
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output

  Serial.begin(9600);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  // client.setCallback(callback);

  Serial.println("Starting up ...");
  // if the display has CS pin try with SPI_MODE0
  tft.init(SCR_WD, SCR_HT, SPI_MODE2);    // init ST7789 display 240x240 pixel
  Serial.println(F("Initialized"));
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);        
  tft.setTextColor(ST77XX_RED);  
  tft.setTextSize(3);           
  tft.println(" Temperatura");
  tft.setCursor(160, TEMP_Y);
  tft.setTextSize(5);
  tft.setTextColor(ST77XX_WHITE);
  tft.println((char)247 );
  tft.setCursor(200, TEMP_Y);
  tft.println("C");

  tft.setCursor(40, 180);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.println("MIN");
  printMinTempDegreesSymbol();
  printMaxTempDegreesSymbol();

  tft.setCursor(170, 180);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED); 
  tft.println("MAX");

  dht.begin();
  sensor_t sensor;
  // delayMS = sensor.min_delay / 1000;
  delayMS = 1000;
 
}


void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  delay(delayMS);

  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Erro ao ler a temperatura!"));
  } else {
    Serial.print(F("Temperatura: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));

    tempC = event.temperature;

    // Publicar temperatura no tópico MQTT
    char tempString[8];
    dtostrf(event.temperature, 1, 2, tempString);
    // Adicionar "°C" ao final da string
    strcat(tempString, " °C");
    client.publish("Monitor_Alon_Alpha/RMTZ2000/temperatura", tempString);
  }

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Erro ao ler a umidade!"));
  } else {
    Serial.print(F("Umidade: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));

    // Publicar umidade no tópico MQTT
    char humString[8];
    dtostrf(event.relative_humidity, 1, 2, humString);
    strcat(humString, " %");
    client.publish("Monitor_Alon_Alpha/RMTZ2000/umidade", humString);
  }
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
 
  // Controle do relé baseado na temperatura
  if (tempC >= upperThreshold && !relayState) {
    digitalWrite(relayPin, LOW);  // Liga o relé
    relayState = true;
  } else if (tempC <= lowerThreshold && relayState) {
    digitalWrite(relayPin, HIGH);   // Desliga o relé
    relayState = false;
  }

  // int estado_rele = digitalRead(relayPin);
  // Serial.print("D3 = ");
  // Serial.print(estado_rele);
  // Serial.println();


}

void deletePreviousTemp()
{
  tft.setCursor(TEMP_X, TEMP_Y);
  tft.setTextSize(5);
  tft.setTextColor(ST77XX_BLACK);
  tft.println(previousTemp,1);
  delay(100);
}

void printTemp()
{
  tft.setCursor(TEMP_X, TEMP_Y);
  tft.setTextSize(5);
  tft.setTextColor(ST77XX_WHITE);
  tft.println(tempC,1);
  delay(100);
  previousTemp = tempC;
}

void printMinTemp()
{
  tft.setCursor(10, 210);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.println(minTemp,1);
  delay(100);
}

void printMaxTemp()
{
  tft.setCursor(150, 210);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.println(maxTemp,1);
  delay(100);
}

void deleteMaxTemp()
{
  tft.setCursor(150, 210);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_BLACK);
  tft.println(maxTemp,1);
  delay(100);
}

void deleteMinTemp()
{
  tft.setCursor(10, 210);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_BLACK);
  tft.println(minTemp,1);
  delay(100);
}

void printMinTempDegreesSymbol()
{
  tft.setCursor(70, 210);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.println((char)247 );
  tft.setCursor(85, 210);
  tft.println("C");
  delay(100);
}

void printMaxTempDegreesSymbol()
{
  tft.setCursor(210, 210);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.println((char)247 );
  tft.setCursor(225, 210);
  tft.println("C");
  delay(100);
}


void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("Monitor_Alon_Alpha/RMTZ2000", "MQTT Server is Conected...");
      // ... and resubscribe
      client.subscribe("Monitor_Alon_Alpha/RMTZ2000");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
// end of code.