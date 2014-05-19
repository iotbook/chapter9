#include <SPI.h>
#include <PubSubClient.h>
#include <WiFi.h>

#define STATE_GOOD 0
#define STATE_ERROR 1

#define  USER_ID   "<user ID>"
#define  TOKEN     "<token>"

// interval to read "sensor" and publish value
#define PUBLISH_INTERVAL_MS  1000

#define MQTT_SERVER "q.m2m.io"

char ssid[] = "<SSID>";     // your network SSID (name) 
char pass[] = "<passphrase>";  // your network password
int status = WL_IDLE_STATUS;  // the Wifi radio's status

WiFiClient wifiClient;
PubSubClient iotBookClient(MQTT_SERVER, 1883, iotBookCallback, wifiClient);

// define pins
int potPin = A0;
int greenLedPin = 3;
int redLedPin = 8;

// reserve space for building MQTT message payloads
char msg[50];

// Variables to hold the state of each of these devices.
int arduinoState = STATE_GOOD;
int bbbState = STATE_GOOD;
int piState = STATE_GOOD;

void setup()
{
  Serial.begin(9600);
  
  delay(3000);
  
  // setup I/O pins
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  digitalWrite(greenLedPin, HIGH);
  digitalWrite(redLedPin, LOW);
      
  // connect to WiFi access point
  if (connectToWiFi() == false) {
    while(true);
  }
  
  // connect MQTT client
  iotBookClient.connect("iot", USER_ID, TOKEN);
  delay(500);
  iotBookClient.subscribe("<mydomain>/apress/pi");
  iotBookClient.subscribe("<mydomain>/apress/bbb");
}

void loop()
{
  // read potentiomenter "sensor"
  int potValue = analogRead(potPin);
  
  // set arduino status
  if (potValue < 500) {
    arduinoState = STATE_ERROR;
  } else {
    arduinoState = STATE_GOOD;
  }
  
  // publish value to the cloud over MQTT
  String sensorMsg = "{\"pot\":";
  sensorMsg.concat(potValue);
  sensorMsg.concat("}");
  Serial.println(sensorMsg);
  sensorMsg.toCharArray(msg, sensorMsg.length()+1);
  iotBookClient.publish("<mydomain>/apress/arduino", msg);
  
  updateLEDs();
  
  // give MQTT client a chance to process
  iotBookClient.loop();
  
  // delay
  delay(PUBLISH_INTERVAL_MS);
}

void iotBookCallback(char* topic, byte* payload, unsigned int length) {
  // message received on subscribed topic
  // parse message to see if we need to act upon it
  // note: Topic recognition (using single character) and
  //       integer parsing logic (subtract 48) is quite naive.

  // first look at topic to determine what logic we need on the payload (is this a pi or bbb?)
  //   possible topics:
  //      - BeagleBone Black:  <domain>/apress/bbb
  //      - Raspberry Pi:      <domain>/apress/pi
  if (topic[16] == 'p') {
    Serial.println("Pi");
    // Message came in on Raspberry Pi topic
    // Message payload format is:  {"b":<0 or 1>}
    int val = payload[5]-48;
    Serial.println(val);
    
    // Now compare to see if Pi is GOOD or ERROR
    // piState is "error" if the value is 1 (button pushed).  Otherwise it is "good".
    if (val == 1) {
      piState = STATE_ERROR;
    } else {
      piState = STATE_GOOD;
    }
  } else if (topic[16] == 'b') {
    Serial.println("BBB");
    // Message came in on BeagleBone Black topic
    // Message payload format is:  {"l":<float>}
    // Parse out integer (truncated) value for comparison later
    int val = 0;
    int i = 5;
    while (payload[i] != '}') {
      val = val*10;
      val = val + (payload[i]-48);
      i++;
    }
    Serial.println(val);
    
    // Now compare to see if BBB is GOOD or ERROR
    // bbbState is "error" if the value is < 5.  Otherwise it is "good".
    if (val < 5) {
      bbbState = STATE_ERROR;
    } else {
      bbbState = STATE_GOOD;
    }
  }
}

boolean connectToWiFi() {
  Serial.print("Attempting to connect to WiFi...");
  status = WiFi.begin(ssid, pass);

  if (status != WL_CONNECTED) {
    Serial.print("ERROR: couldn't get a wifi connection.");
    return false;
  } else {
    Serial.print("connected.");
    return true;
  }
}

void updateLEDs() {
  //   if any of the devices are in an errant state, light the red LED
  //   otherwise light the green one
  if (arduinoState == STATE_ERROR || bbbState == STATE_ERROR || piState == STATE_ERROR) {
    digitalWrite(greenLedPin, LOW);
    digitalWrite(redLedPin, HIGH);
  } else {
    digitalWrite(greenLedPin, HIGH);
    digitalWrite(redLedPin, LOW);
  }
}
