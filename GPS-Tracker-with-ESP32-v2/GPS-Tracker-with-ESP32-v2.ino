
#include <WiFi.h>
#include <PubSubClient.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>

// Datos de la conexxion a wifi
const char* ssid = "GoLocAut";
const char* password = "123456789";
const char* mqtt_server = "broker.hivemq.com";
const unsigned int writeInterval = 2000; // Intervalo para la recoleccion de datos del GPS

// Pines de esp32 para la recespcion de los datos del modulo gps
static const int RXD2 = 16, TXD2 = 17;
static const uint32_t GPSBaud = 9600;

// Variables del modulo GPS
WiFiClient goClient;
PubSubClient client(goClient);
TinyGPSPlus gps;
SoftwareSerial gpsSerial(RXD2, TXD2);

// Variables globales para alamcenar la latitud y longitud
float latitude, longitude;

void setup() {
  Serial.begin(115200);
  setup_wifi();
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  gpsSerial.begin(GPSBaud);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
//MQTT Callback
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  
}
//MQTT Reconected
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("GoLocAutClient")) {
      Serial.println("MQTT client connected");
      // Enciende el led cuando esta conectado
      digitalWrite(LED_BUILTIN, HIGH);
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      //Se apaga el led cuando esta descoectado
      digitalWrite(LED_BUILTIN, LOW);
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Inicia el procesoso de gps
  while(gpsSerial.available() > 0)
    if (gps.encode(gpsSerial.read()))
        gpsInfo();
    if (millis() > 5000 && gps.charsProcessed() < 10)
    {
      Serial.println(F("GPS no detectado"));
      while(true);
    }
}


void gpsInfo(){
  String Datos; // Almacenar los datos obteniddo del gps
  if(gps.location.isValid()){
        latitude = (gps.location.lat());
        longitude = (gps.location.lng());

        StaticJsonDocument<300> docEnvio;

        docEnvio["name"] = "GoLocAut";
        docEnvio["lat"] = latitude;
        docEnvio["lon"] = longitude; 

        serializeJson(docEnvio, Datos);
        Serial.println(Datos);

        client.publish("GoLocAut/GPSTracker", (char*)Datos.c_str());
        delay(writeInterval);
        
    }
    else{
        Serial.println("INVALIDO (Obteniendo datos)...");
      } 
}
