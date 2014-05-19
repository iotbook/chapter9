var mqtt = require('mqtt');
var b = require('bonescript');

var arduinoError = false;
var piError = false;
var bbbError = false;

function bbbOnMessage(topic, message, packet) {
    if (topic.indexOf("arduino") > -1) {
        console.log("arduino");
        var val = message.match(/\d+/)[0];
        console.log(val);
        if (val < 500) {
            arduinoError = true;
        } else {
            arduinoError = false;
        }
    } else if (topic.indexOf("pi") > -1) {
        console.log("pi");
        var val = message.match(/\d+/)[0];
        console.log(val);
        if (val == 1) {
            piError = true;
        } else {
            piError = false;
        }   
    }
}


var client = mqtt.connect('mqtt://<user>:<password>@q.m2m.io?clientId=iotbook-bbb');

// setup LED GPIO pins
var greenLed = "P9_15";
var redLed = "P9_11";
b.pinMode(greenLed, b.OUTPUT);
b.pinMode(redLed, b.OUTPUT);
b.digitalWrite(redLed, b.LOW);
b.digitalWrite(greenLed, b.HIGH);
// setup analog pin for light reading
var lightPin = "P9_40";
var lightReading = 0.0;

// subscribe to the Arduino and Pi topics
// add a handler for the incomming message (only needs to be added once)
client.subscribe(<project>/apress/arduino', function() {
  client.on('message', bbbOnMessage);
});
// subscribe to the Raspberry Pi topic
client.subscribe(<project>/apress/pi');

setInterval(function(){
  console.log('Reading light level');
  b.analogRead(lightPin, function(x){lightReading=x.value;});
  
  console.log(lightReading);
  lightReading = Math.round(lightReading*100);
  if (lightReading < 5) {
      bbbError = true;
  } else {
      bbbError = false;
  }
  client.publish(<project>/apress/bbb', '{"l":' + lightReading + '}');
}, 2000);

setInterval(function(){
    if (arduinoError || piError || bbbError) {
        b.digitalWrite(redLed, b.HIGH);
        b.digitalWrite(greenLed, b.LOW);
    } else {
        b.digitalWrite(redLed, b.LOW);
        b.digitalWrite(greenLed, b.HIGH);
    }
}, 1000);
