/*
 * My ESP8266 Mosquitto MQTT Publish-Subscribe
 * Based on Thomas Varnish (https://github.com/tvarnish)
 * PubSubClient API: https://pubsubclient.knolleary.net/api.html
 * 
 */
 
#include <Bounce2.h> // Used for "debouncing" the pushbutton
#include <ESP8266WiFi.h> // Enables the ESP8266 to connect to the local network (via WiFi)
#include <PubSubClient.h> // Allows us to connect to, and publish to the MQTT broker
#include "credentials.h"

// Input/Output
const int ledPin = LED_BUILTIN; 
  // Built-in led to show button pressed and controlled by any message beginning 
  // with '1' pubished to 'inTopic' (Note: Any other message turns LED off)
const int buttonPin = 13; 
  // Used to publish message from ESP -> Connect button to pin #13

// Serial
// Baud rate must match serial monitor for debugging
const int baud_rate = 115200;

// WiFi
// Variables set in "credentials.h"
const char* wifi_ssid = WIFI_SSID;
const char* wifi_password = WIFI_PASSWORD;
const char* wifi_hostname = WIFI_HOSTNAME;

// MQTT
// Variables set in "credentials.h"
const char* mqtt_server = MQTT_SERVER;
const char* mqtt_in_topic = MQTT_IN_TOPIC;
const char* mqtt_out_topic = MQTT_OUT_TOPIC;
const char* mqtt_username = MQTT_USERNAME;
const char* mqtt_password = MQTT_PASSWORD;
const char* mqtt_clientID = MQTT_CLIENT_ID; 

long lastMsg = 0;
char msg[50];
int value = 0;

// Initialise the Pushbutton Bouncer object
Bounce bouncer = Bounce();

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
PubSubClient client(mqtt_server, 1883, wifiClient); 

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
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void setup_wifi() {

  Serial.begin(baud_rate);

  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  // Rename WiFi hostname
  WiFi.hostname(wifi_hostname);
  Serial.printf("Wi-Fi hostname: %s\n", wifi_hostname);

  // Set WiFi to station
  WiFi.mode(WIFI_STA);
  

  // Connect to the WiFi
  WiFi.begin(wifi_ssid, wifi_password);

  // Wait until the connection has been confirmed before continuing
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  // Debugging - Output the IP Address of the ESP8266
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); 
  Serial.println();
}

void setup_mqtt() {
  // Connect to MQTT Broker
  // client.connect returns a boolean value to let us know if the connection was successful.
  // If the connection is failing, make sure you are using the correct MQTT Username and Password (Setup Earlier in the Instructable)
  if (client.connect(mqtt_clientID, mqtt_username, mqtt_password)) {
    Serial.println("Connected to MQTT Broker!");
    Serial.print("MQTT Broker IP: ");
    Serial.println(mqtt_server);
    Serial.print("MQTT Client ID: ");
    Serial.println(mqtt_clientID);
    Serial.println();
    client.setCallback(callback);
    client.publish(mqtt_out_topic, "connected");
    client.subscribe(mqtt_in_topic);
  }
  else {
    Serial.println("Connection to MQTT Broker failed...");
  }
}

// SETUP =======>
void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(buttonPin, INPUT);

  // Switch the on-board LED off to start with
  digitalWrite(ledPin, HIGH);

  // Setup pushbutton Bouncer object
  bouncer.attach(buttonPin);
  bouncer.interval(5);

  setup_wifi();
  setup_mqtt();
}

void button() {
  // Update button state
  // This needs to be called so that the Bouncer object can check if the button has been pressed
  bouncer.update();

  if (bouncer.rose()) {
    // Turn light on when button is pressed down
    // (i.e. if the state of the button rose from 0 to 1 (not pressed to pressed))
    digitalWrite(ledPin, LOW);

    // PUBLISH to the MQTT Broker (topic = mqtt_out_topic, defined at the beginning)
    // Here, "Button pressed!" is the Payload, but this could be changed to a sensor reading, for example.
    if (client.publish(mqtt_out_topic, "Button pressed!")) {
      Serial.println("Button pushed and message sent!");
    }
    // Again, client.publish will return a boolean value depending on whether it succeded or not.
    // If the message failed to send, we will try again, as the connection may have broken.
    else {
      Serial.println("Message failed to send. Reconnecting to MQTT Broker and trying again");
      client.connect(mqtt_clientID, mqtt_username, mqtt_password);
      delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
      client.publish(mqtt_out_topic, "Button pressed!");
    }
  }
  else if (bouncer.fell()) {
    // Turn light off when button is released
    // i.e. if state goes from high (1) to low (0) (pressed to not pressed)
    digitalWrite(ledPin, HIGH);
  }
}

void reconnect() {
  // Loops until reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT reconnection...");
    if (client.connect(mqtt_clientID)) {
      Serial.println("connected");
      client.publish(mqtt_out_topic, "reconnected");
      // resubsciribe:
      client.subscribe(mqtt_in_topic);
    } else {
      Serial.print("MQTT connection failed, Error code: ");
      Serial.println(client.state());
      Serial.println("  ...will try again after 5 seconds.");
      delay(5000);
    }
  }
}

// LOOP =======>
void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  
  button();
  
  client.loop(); // Refresh MQTT connection & allows processing of incoming messages from 'PubSubClient.h'
}
