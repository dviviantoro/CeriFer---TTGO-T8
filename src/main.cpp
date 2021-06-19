#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <Adafruit_ADS1X15.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <PubSubClient.h>

void initialize_sensor();
void initialize_wifi();
boolean reconnect();
void update_sensor();

struct dataSensor
{
  float ph = 0.0F;
  float temperature = 0.0F;
};

static dataSensor data;
static char payload[256];
StaticJsonDocument<256>doc;

const char ssid[] = "Dasar";
const char password[] = "liquidgas";

/*IPAddress local_ip(192,168,8,102);
IPAddress home_router(192,168,8,1);
IPAddress subnet(255,255,255,0);*/

WiFiClient net;
PubSubClient mqttClient(net);
const char mqtt_broker[] =  ""; //masukin alamat IP address gateway raspberry (misal:192.168.8.101)
const char publish_Topic[] = "cerifer/sensor/fermentasi"; 
#define CLIENT_ID  "" 
#define MQTT_USERNAME "" 
#define MQTT_PASSWORD "" 

long lastReconnectAttempt = 0;

Adafruit_ADS1115 ads;

#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature onewire_sensor(&oneWire);

long lastData = 0;

void initialize_sensor()
{

  if (!ads.begin())
  {
    Serial.println("Failed to initialize ADS.");
    while(1);
  }
  onewire_sensor.begin();

}

void initialize_wifi(){

  delay(10);

  /*if (!WiFi.config(local_ip, home_router, subnet))
  {
    Serial.println("Fail to configure STA");
  }*/
  
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  Serial.println();
  Serial.print("Connecting WiFi to: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  /*while(WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin(ssid, password);
    Serial.print("Attempting WiFi connection .....");
    
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WiFi connected");
      Serial.println("\nWiFi RSSI: ");
      Serial.println(WiFi.RSSI());
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
    }

    else
    {
      Serial.print("Failed to connect to WiFi");
      Serial.println(", Try again in 5 seconds");
      delay(5000);
    }
    
  }*/

  while (true)
  {
    delay(10);
    Serial.print(".");
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WiFi Connected");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());
      Serial.print("RSSI: ");
      Serial.println(WiFi.RSSI());
      break;
    }
    
  }

}

boolean reconnect(){

  Serial.println("Attempting to connect MQTT");

  if (mqttClient.connect(CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD))
  {
    Serial.println("Connected to MQTT broker");
  }

  return mqttClient.connected();
}


void update_sensor()
{

  int16_t adc0;
  float voltage = 0.0F;
  float pH = 0.0F;

  adc0 = ads.readADC_SingleEnded(0);
  voltage = ads.computeVolts(adc0);

  int16_t count = 0;
  while (count < 57)
  {
    pH += (-voltage*5.8582) + 15.8019; //for sensor1
    //pH += (-voltage*5.7870) + 15.7077; //for sensor2
    //pH += (-voltage*5.7803) + 15.6965; //for sensor3
    //pH += (-voltage*5.7870) + 15.6915; //for sensor4
    //pH += (-voltage*5.7937) + 15.7213; //for sensor5
    count++;
    delay(10); 
  }

  data.ph = pH/57;
  
  onewire_sensor.requestTemperaturesByIndex(0);
  float temperature = onewire_sensor.getTempCByIndex(0);
  data.temperature = temperature;

  doc["ph"] = data.ph;
  doc["temperature"] = data.temperature;
  serializeJsonPretty(doc, payload);
  Serial.println(payload);

}

void setup() {

  Serial.begin(9600);
  initialize_sensor();
  initialize_wifi();
  mqttClient.setServer(mqtt_broker, 1883); 
  lastReconnectAttempt = 0;
  
}

void loop() {

  if (WiFi.status() != WL_CONNECTED)
  {
    initialize_wifi();
  }

  if (WiFi.status() == WL_CONNECTED && !mqttClient.connected())
  {
    long now = millis();

    if (now - lastReconnectAttempt > 2000)
    {
      lastReconnectAttempt = now;

      if (reconnect())
      {
        lastReconnectAttempt = 0;
      }
      
    }

  }

  else
  {
    mqttClient.loop();
  }

  long now = millis();
  if (now - lastData > 60000) //update data sensor tiap 1 menit ke gateway raspberry
  {
    lastData = now;
    update_sensor();
  }
  
}