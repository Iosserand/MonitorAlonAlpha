#include <Adafruit_GFX.h>     // Adafruit core graphics library
#include <Adafruit_ST7789.h>  // Adafruit hardware-specific library for ST7789
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


// ST7789 TFT module connections
#define TFT_DC    5     // TFT DC  pin is connected to NodeMCU pin D1 (GPIO5)
#define TFT_RST   4     // TFT RST pin is connected to NodeMCU pin D2 (GPIO4)
#define TFT_CS    15     // TFT CS  pin is connected to NodeMCU pin D8 (GPIO15)
#define SCR_WD   240
#define SCR_HT   240   
#define TEMP_X 20
#define TEMP_Y 85 

// NTC Thermostor connections
#define ThermistorPin A0  // Pino A0 para leitura analógica

// RELAY connections
#define buttonPin 2 /// simulaçao
#define relayPin  0

// initialize ST7789 TFT library with hardware SPI module
// SCK (CLK) ---> NodeMCU pin D5 (GPIO14)
// MOSI(DIN) ---> NodeMCU pin D7 (GPIO13)
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Variavéis de Temperatura
int Vo;
float R1 = 10000;
float logR2, R2, T;
float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
float previousTemp = -100.0;
float tempC = 0;
float minTemp = 200;
float maxTemp = 0;

// Update these with values suitable for your network.
const char* ssid = "YMSHOPFLOOR";
const char* password = "Kr4sn@ya7v3zDa";
const char* mqtt_server = "172.21.68.55";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
String mesg;
int value = 0;

void setup() {
  pinMode(relayPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 3377);
  client.setCallback(callback);

  Serial.begin(9600);
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
 
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  delay(1000);

  Vo = analogRead(ThermistorPin);
  R2 = R1 * (1023.0 / (float)Vo - 1.0);
  logR2 = log(R2);
  T = (1.0 / (c1 + c2*logR2 + c3*logR2*logR2*logR2));
  T = T - 285.15;
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
 
  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "RMTZ2000 Conectado... #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("Monitor_Alon_Alpha/Temperatura", msg);
  }


  ligarRelay();


}

void deletePreviousTemp()
{
  tft.setCursor(TEMP_X, TEMP_Y);
  tft.setTextSize(5);
  tft.setTextColor(ST77XX_BLACK);
  tft.println(previousTemp,1);
}

void printTemp()
{
  tft.setCursor(TEMP_X, TEMP_Y);
  tft.setTextSize(5);
  tft.setTextColor(ST77XX_WHITE);
  tft.println(tempC,1);
}

void printMinTemp()
{
  tft.setCursor(10, 210);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.println(minTemp,1);
}

void printMaxTemp()
{
  tft.setCursor(150, 210);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.println(maxTemp,1);
}

void deleteMaxTemp()
{
  tft.setCursor(150, 210);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_BLACK);
  tft.println(maxTemp,1);
}

void deleteMinTemp()
{
  tft.setCursor(10, 210);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_BLACK);
  tft.println(minTemp,1);
}

void printMinTempDegreesSymbol()
{
  tft.setCursor(70, 210);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.println((char)247 );
  tft.setCursor(85, 210);
  tft.println("C");
}

void printMaxTempDegreesSymbol()
{
  tft.setCursor(210, 210);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_RED);
  tft.println((char)247 );
  tft.setCursor(225, 210);
  tft.println("C");
}

void ligarRelay()
{
  int state_button = digitalRead(buttonPin);
  int state_relay = digitalRead(relayPin);
  if (state_button == LOW) {
    digitalWrite(relayPin, LOW);
  } else {
    digitalWrite(relayPin, HIGH);
  }
  // Serial.print("Button[D4] = ");
  // Serial.print(state_button);
  // Serial.println();
  // Serial.print("Rele[D3] = ");
  // Serial.print(state_relay);
  // Serial.println();
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
      client.publish("Monitor_Alon_Alpha/Temperatura", "MQTT Server is Conected...");
      // ... and resubscribe
      client.subscribe("Monitor_Alon_Alpha/Temperatura");
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