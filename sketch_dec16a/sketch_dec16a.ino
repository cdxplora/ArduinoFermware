#include <Arduino_MKRIoTCarrier.h>
#include <ArduinoMqttClient.h> // The library to use MQTT protocol
#include <WiFiNINA.h>
#include <ArduinoJson.h> // ArduinoJson library for handling data
#include "arduino_secrets.h" // WiFi credentials (SSID and password)

MKRIoTCarrier carrier;      // Object that manages MKR IoT Carrier functionalities
char payload[256];          // Buffer to hold the payload message
char ssid[] = SECRET_SSID;  // Network SSID (name)
char pass[] = SECRET_PASS;  // Network password
int status = WL_IDLE_STATUS; // WiFi radio's initial status

// MQTT broker details
const char broker[] = "test.mosquitto.org";   // Address of the MQTT broker
int port = 1883;                              // MQTT broker port
const char topic[] = "Karelia_IoT_Project_Team2_2024"; // MQTT topic to publish data
const char topicSubscribe[] = "Karelia_IoT_Project_Team2_2024send"; // MQTT topic for subscribing

WiFiClient wifiClient;          // WiFi connection object
MqttClient mqttClient(wifiClient); // MQTT client object

const int relayPin = 7; // Digital pin for the relay

void setup() {
  // Initialize serial communication
  Serial.begin(9600);
  while (!Serial);

  // Attempt to connect to the WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    delay(10000); // Wait 10 seconds before retrying
  }

  // Print network information
  Serial.println("Connected to the network");
  Serial.println("----------------------------------------");
  printData();
  Serial.println("----------------------------------------");

  // Initialize the MKR IoT Carrier
  carrier.withCase();
  carrier.begin();

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW); // Start with relay OFF

  // Connect to the MQTT broker
  connectToBroker();

  // Subscribe to the topic
  mqttClient.onMessage(onMessageReceived); // Set the callback function
  mqttClient.subscribe(topicSubscribe);
  Serial.print("Subscribed to topic: ");
  Serial.println(topicSubscribe);
}

void loop() {
  // Ensure the MQTT client stays connected
  if (!mqttClient.connected()) {
    connectToBroker();
  }

  mqttClient.poll(); // Process incoming messages

  // Read sensor data from the carrier
  float temperature = carrier.Env.readTemperature();
  float humidity = carrier.Env.readHumidity();
  float pressure = carrier.Pressure.readPressure();

  // Generate a JSON payload containing sensor data
  generateJsonMessage(temperature, humidity, pressure);

  // Publish the data to the MQTT topic
  mqttClient.beginMessage(topic);
  mqttClient.print(payload);
  mqttClient.endMessage();

  Serial.println("Sensor data sent to MQTT broker");
  delay(10000); // Send data every 10 seconds
}

void connectToBroker() {
  Serial.print("Connecting to MQTT broker: ");
  Serial.println(broker);
  while (!mqttClient.connect(broker, port)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected to MQTT broker");
}

void generateJsonMessage(float temperature, float humidity, float pressure) {
  // Create a JSON document
  StaticJsonDocument<200> jsonDoc;
  IPAddress ip = WiFi.localIP(); // Get the device's IP address

  // Add sensor and device data to the JSON document
  jsonDoc["Device_IP"] = ip.toString();
  jsonDoc["Temperature"] = temperature;
  jsonDoc["Humidity"] = humidity;
  jsonDoc["Pressure"] = pressure;

  // Serialize the JSON document to the payload buffer
  serializeJson(jsonDoc, payload);

  // Print the JSON payload for debugging
  Serial.println("Generated JSON payload:");
  Serial.println(payload);
}

void printData() {
  // Print network and device information
  IPAddress ip = WiFi.localIP();
  Serial.println("Board Information:");
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.println("Network Information:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI): ");
  Serial.println(rssi);
}

void onMessageReceived(int messageSize) {
  char message[256];                 // Buffer to hold the incoming message
  mqttClient.read((uint8_t*)message, messageSize); // Read the message
  message[messageSize] = '\0';       // Null-terminate the string

  Serial.println("Message received:");
  Serial.println(message);           // Print the message for debugging

  // Parse the received JSON message
  StaticJsonDocument<200> jsonDoc;   // Create a JSON document to store parsed data
  DeserializationError error = deserializeJson(jsonDoc, message);

  if (error) {
    Serial.print("Error parsing JSON: ");
    Serial.println(error.c_str());   // Print the error if JSON parsing fails
    return;
  }

  // Extract specific data from the JSON
  const char* heatLevel = jsonDoc["Heat"]; // Extract the "Heat" value

  // Print the extracted data
  Serial.print("Heat Level: ");
  Serial.println(heatLevel);

  // Handle the extracted data (custom logic)
  if (strcmp(heatLevel, "Very_high") == 0) {
    Serial.println("Heat level is very high! Turning relay ON and updating display.");
    digitalWrite(relayPin, HIGH); // Turn the relay ON

    // Update the display for high heat
    carrier.display.fillScreen(0xF800); // Red background
    carrier.display.setCursor(60, 100);
    carrier.display.setTextColor(0xFFFF);
    carrier.display.setTextSize(2);
    carrier.display.print("Heat Level: VERY HIGH");
  } else {
    Serial.println("Heat level is normal. Turning relay OFF and updating display.");
    digitalWrite(relayPin, LOW); // Turn the relay OFF

    // Update the display for normal heat
    carrier.display.fillScreen(0x07E0); // Green background
    carrier.display.setCursor(60, 100);
    carrier.display.setTextColor(0xFFFF);
    carrier.display.setTextSize(2);
    carrier.display.print("Heat Level: NORMAL");
  }
}
