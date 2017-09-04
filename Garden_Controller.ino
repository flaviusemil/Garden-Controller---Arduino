#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include <PubSubClient.h>

// #define DEVELOP_MODE true

const char* ssid = "UEO";
const char* password = "";

const char* mqtt_server = "192.168.10.215";

const int redPin = 14;
const int enablePin = 12;
const int greenPin = 13;
const int bluePin = 15;

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
String fullMessage = "";
String command = "";
String parameter = "";

bool setParam = false;

void setup() {
  Serial.begin(115200);

  Serial.print("Chip ID: ");
  Serial.println(ESP.getChipId(), HEX);
  Serial.print("Flash ID: ");
  Serial.println(ESP.getFlashChipId(), HEX);
  Serial.print("Flash size: ");
  Serial.println(ESP.getFlashChipSize());

  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // #if(!DEVELOP_MODE)
  //   ArduinoOTA.setPassword((const char *)"orionpass");
  // #endif
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();

  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(enablePin, OUTPUT);
  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);

  digitalWrite(enablePin, HIGH);
  digitalWrite(redPin, HIGH);
  digitalWrite(bluePin, HIGH);
  digitalWrite(greenPin, HIGH);
  
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  Serial.print("Running at ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println("MHz");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  fullMessage = "";
  command = "";
  parameter = "";
  
  for (int i = 0; i < length; i++) {
    fullMessage.concat((char)payload[i]);

    if (setParam) {
      if ((char)payload[i] != ' ')
        parameter.concat((char)payload[i]);
    } else {
      if ((char)payload[i] != ' ')
        command.concat((char)payload[i]);
    }

    if ((char)payload[i] == ' ') {
      setParam = true;
    }
  }

  Serial.println(fullMessage);
  Serial.print("Command: ");
  Serial.println(command);
  Serial.print("Parameter: ");
  Serial.println(parameter);
  setParam = false;

  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);
  } else if ((char)payload[0] == '0') {
    digitalWrite(BUILTIN_LED, HIGH);
  }

  if (fullMessage == "reset" || fullMessage == "restart" || fullMessage == "reboot") {
    Serial.println("Rebooting on command!");
    ESP.restart();
  }

  if (fullMessage == "red") {
    Serial.println("Toggaling the red led!");
    toggleLed(redPin);
  }

  if (fullMessage == "green") {
    Serial.println("Toggaling the green led!");
    toggleLed(greenPin);
  }

  if (fullMessage == "blue") {
    Serial.println("Toggaling the blue led!");
    toggleLed(bluePin);
  }

  if (fullMessage == "toggleLed") {
    Serial.println("Toggleing LED!");
    toggleLed(enablePin);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
	  client.publish("outTopic", "hello world");
	  client.subscribe("admin/commands");
      client.subscribe("inTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void toggleLed(int pinNumber) {
  digitalWrite(pinNumber, !digitalRead(pinNumber));
}

void loop() {
  ArduinoOTA.handle();

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, 75, "hello world #%ld", value);
    Serial.print("Publish fullMessage: ");
    Serial.println(msg);
	client.publish("outTopic", msg);

	Serial.println();
  }
}

byte unsetBit(byte reg, int bit) {
  return reg & ~(1 << bit);
}

byte unsetBitWithMask(byte reg, byte mask) {
  return reg & ~(mask);
}

byte setBit(byte reg, int bit) {
  return reg | (1 << bit);
}

byte setBitWithMask(byte reg, byte mask) {
  return reg | mask;
}