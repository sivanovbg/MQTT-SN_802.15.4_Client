# MQTT-SN_802.15.4_Client
MQTT-SN Client implementation based on MRF24J40 and Arduino

Visit project website at https://sites.google.com/view/hobbyiot/projects/mqtt-sn-802-15-4

This is an MQTT-SN Client implementation based on MRF24J40 and Arduino.

The implementation is based ot MQTT-SN Specification Version 1.2. Arduino board used in the sample schetch is Arduino Nano. Any compatible board with ATmega328 could be used instead. A level shifting is required is certain cases.

Version Beta 1 (VB1)

This is a very early implementation. Part of the whole message list according specs is supported so far. Topics should be predefined on the client and gateway sides and are fixed to 2 positions alpha-numeric string.

Messages supported:

CONNECT

CONNACK

PINGREQ

PINGRESP

PUBLISH (both directions)

SUBSCRIBE

SUBACK

