/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
//#include "DHT.h"
//#define DHTPIN D1     // what digital pin we're connected to
//#define DHTTYPE DHT11   // DHT 11

const char* ssid     = "Tommy";
const char* password = "weloveyouall";
const char* host ="api.thingspeak.com";
const char* apiKey = "GIC6ZEWS8L1XVDDG";
// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "CF 05 98 89 CA FF 8E D8 5E 5C E0 C2 E4 F7 E6 C3 C7 50 DD 5C";

#include <NewPing.h>
#define TRIGGER_PIN1 5 // D2  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN1 4//    D3  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define TRIGGER_PIN2 14 // D2  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN2 16//    D3  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 200 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

NewPing sonar1(TRIGGER_PIN1, ECHO_PIN1, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
NewPing sonar2(TRIGGER_PIN2, ECHO_PIN2, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

const char* mqtt_server = "192.168.1.196";
WiFiClient espClient;
PubSubClient mqttClient(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  char* message="";
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    message = message + (char)payload[i];
  }
  Serial.print(message);
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      mqttClient.publish("tommy/test", "hello world");
      // ... and resubscribe
      mqttClient.subscribe("tommy/weather/cc");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  
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
  mqttClient.setServer(mqtt_server, 1883);
  mqttClient.setCallback(callback);
}



void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }
  mqttClient.loop();


  Serial.print("connecting to ");
  Serial.println(host);

   
  // Use WiFiClient class to create TCP connections
   WiFiClientSecure client;
  const int httpPort = 443;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }
  
  // We now create a URI for the request
  int distance1= sonar1.ping_cm();
  delay(500);
  int distance2= sonar2.ping_cm();
  delay(500);
  //publish to mqtt
  String  mmm = String(distance1) + "," + String(distance2);
  mmm.toCharArray(msg, 50) ;
  mqttClient.publish("tommy/garage/doors", msg);

  
  String url = "https://api.thingspeak.com/update?api_key=";
  url += apiKey;
  url += "&field1=";
  url += String(distance1);
  url += "&field2=";
  url += String(distance2);
  
  Serial.print("Requesting URL: ");
 Serial.println(url);
  
  // This will send the request to the server
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: api.thingspeak.com" + "\r\n" + 
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 20000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }
  
  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);

     Serial.print("Distance: "+ String(distance1) + ", " + String(distance2) );
  //Serial.print(sonar1.ping_cm()); // Send ping, get distance in cm and print result (0 = outside set distance range)
  Serial.println("cm");
  }
  
  Serial.println();
  Serial.println("closing connection");
  delay(10000);
}

