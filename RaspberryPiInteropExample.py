#!/usr/bin/python
import paho.mqtt.client as mqtt
from time import sleep
import RPi.GPIO as GPIO
import re
  
user = '<user ID>'
key = '<token>'

arduinoError = False
bbbError = False
piError = False

def updateLEDs():
  if arduinoError == True or bbbError == True or piError == True:
    GPIO.output(GREEN_LED, False)
    GPIO.output(RED_LED, True)
  else:
    GPIO.output(GREEN_LED, True)
    GPIO.output(RED_LED, False)

def on_message(client, obj, msg):
  # a message was received on a subscribed topic
  #   first look at topic to determine what logic we need on the payload (is this an arduino or bbb?)
  #   possible topics:
  #     - BeagleBone Black:  <project>/apress/bbb
  #     - Arduino:                   <project>/apress/arduino
  global arduinoError
  global bbbError
  if msg.topic == "<project>/apress/bbb":
    # parse with regex to find number, store in val
    val = int(re.search(r'\d+', msg.payload).group())
    print str(val)
    if val < 5:
      bbbError = True
    else:
      bbbError = False
  elif msg.topic == "<project>/apress/arduino":
    # parse with regex to find number, store in val
    val = int(re.search(r'\d+', msg.payload).group())
    print str(val)
    if val < 500:
      arduinoError = True
    else:
      arduinoError = False
   
# setup GPIO pins
GPIO.setmode(GPIO.BCM)
GREEN_LED = 24 
RED_LED = 23
BUTTON_PIN = 25
GPIO.setup(GREEN_LED, GPIO.OUT)
GPIO.setup(RED_LED, GPIO.OUT)
GPIO.output(GREEN_LED, True)
GPIO.output(RED_LED, False)
GPIO.setup(BUTTON_PIN, GPIO.IN)

# create MQTT client
client = mqtt.Client("iotbook-pi")
client.on_message = on_message
client.username_pw_set(user, key)

# connect to 2lemetry platform
client.connect("q.m2m.io", 1883, 60)
client.loop()

# subscribe to device topics
client.subscribe("<project>/apress/arduino", 0)
client.subscribe("<project>/apress/bbb", 0)
client.loop()

try:
  while(True):
    if (GPIO.input(BUTTON_PIN) == True):
      pub_str = "{\"b\":1}"
      piError = True
    else:
      pub_str = "{\"b\":0}"
      piError = False
    print pub_str
    client.publish("<project>/apress/pi", pub_str)
    client.loop()
    updateLEDs()
    sleep(2)
finally:
  # disconnect client
  client.disconnect()
  # cleanup GPIO
  GPIO.cleanup()
