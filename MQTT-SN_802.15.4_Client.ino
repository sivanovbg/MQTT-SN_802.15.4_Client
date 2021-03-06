/*
MQTT-SN Client implementation based on MRF24J40 and Arduino

This is an MQTT-SN Client implementation based on MRF24J40 and Arduino.

The implementation is based ot MQTT-SN Specification Version 1.2.
Arduino board used in the sample schetch is Arduino Nano.
Any compatible board with ATmega328 could be used instead.
A level shifting is required is certain cases.

Version Beta 1 (VB1)

This is a very early implementation. Part of the whole message list according specs is supported so far.
Topics should be predefined on the client and gateway sides
and are fixed to 2 positions alpha-numeric string.

Messages supported:

CONNECT
CONNACK
PINGREQ
PINGRESP
PUBLISH (both directions)
SUBSCRIBE
SUBACK

* This example makes the Client to subscribe
* for topic "s2" through the MQTT-SN Gateway.
* 
* The client reacts with switching pin_led (D4) to 1 or 0
* according to the payload data received within topic "t2"
* received thriugh the MQTT-SN Gateway.

*/

#include <SPI.h>
#include <mrf24j.h>   // *** Please use the modified library found within the same repo on GitHub ***

const int pin_reset = 6;
const int pin_cs = 10;
const int pin_interrupt = 2;

const int pin_led = 4;

Mrf24j mrf(pin_reset, pin_cs, pin_interrupt);

long last_time, last_ping, last_pingresp, last_pub;
long tx_interval = 5000;
long ping_interval = 8000;
long pub_interval = 60000;
long timeout_interval = 60000;

boolean message_received = false;
boolean node_connected = false;
boolean connection_timeout = true;
boolean node_subscribed = false;
char rx_buffer[127];
char tx_buffer[127];
uint8_t rx_len;

unsigned long current_time;

// Change 0x44 and 0x96 with the 802.15.4 short address bytes of your Client (this node)

uint8_t CONNECT_MSG[] = { 0x0A, 0x04, 0x00, 0x01, 0x00, 0x10, 0xFF, 0xFF, 0x44, 0x96 };

uint8_t PINGREQ_MSG[] = { 0x0A, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x44, 0x96 };

// Several sample messages used within the current implementation set up

uint8_t PUBLISH_MSGON[] = { 0x0B, 0x0C, 0x00, 't', '2', 0x44, 0x96, ' ', 'R', 'E', 'D' };
uint8_t PUBLISH_MSGOFF[] = { 0x0B, 0x0C, 0x00, 't', '2', 0x44, 0x96, 'D', 'a', 'r', 'k' };

uint8_t SUBSCRIBE_MSG[] = { 0x07, 0x12, 0x00, 0x44, 0x96, 's', '2' };

typedef struct
{
  uint8_t Length;
  uint8_t MsgType;
  uint8_t Var[127];
} Message;

Message Msg;

#define OWN_ADDRESS 0x4496    // 802.15.4 short address of the Client (this node)
#define GW_ADDRESS 0x8266    // 802.15.4 short address of the Gateway

#define CONNACK   0x05
#define PINGRESP  0x17
#define PUBLISH   0x0C
#define PUBACK    0x0D
#define SUBACK    0x13

void setup() {
  Serial.begin(115200);

  Serial.println("MQTT-SN Client implementation");
  Serial.println("=============================");
  
  mrf.reset();
  mrf.init();
  
  mrf.set_pan(0xcfce); // pan ID = 0xABCD // Enter your 802.15.4 PAN ID here
  mrf.address16_write(OWN_ADDRESS);

  Serial.println("Wireless module set up done.");
  
  attachInterrupt(0, interrupt_routine, CHANGE);
  last_time = millis();
  interrupts();

  current_time = millis();

  last_time = current_time + tx_interval + 1;
  last_ping = current_time + ping_interval + 1;
  last_pingresp = current_time + timeout_interval + 1;
  last_pub = current_time + pub_interval + 1;

}

void interrupt_routine() {
    mrf.interrupt_handler(); // mrf24 object interrupt routine

    pinMode(pin_led,OUTPUT);
}

void loop()
{  
  int i;

  mrf.check_flags(&handle_rx, &handle_tx);

  current_time = millis();
  
  if(!node_connected)
    if (current_time - last_time > tx_interval)
    {
      last_time = current_time;
      Serial.println("Sending CONNECT...");
      mrf.send16(GW_ADDRESS,CONNECT_MSG,sizeof(CONNECT_MSG));
    }

    if(message_received)
    {
      Msg.Length = rx_buffer[0];  // Fill in the message fields
      Msg.MsgType = rx_buffer[1];
      for(i=2;i<Msg.Length;i++)
      {
        Msg.Var[i-2] = rx_buffer[i];
      }

      switch (Msg.MsgType)        // Check message type
      {
        case(CONNACK):
        {
          Serial.println("CONNACK received");
          message_received = false;
          node_connected = true;
          connection_timeout = false;
        } break;
        
        case(PINGRESP):
        {
          Serial.println("PINGRESP received");
          last_pingresp = current_time;
          message_received = false;
          connection_timeout = false;
        } break;

        case(PUBACK):
        {

        } break;

        case(SUBACK):
        {
          Serial.println("SUBACK Received.");  
          node_subscribed = true;
          message_received = false;
        } break;

        case(PUBLISH):
        {
          Serial.println("Publish message received");
          
          if(node_subscribed)
          {
            Serial.print("Processing publish message: ");
//          pubMsg.Flags = rx_buffer[2];
//          pubMsg.topic_id[0] = rx_buffer[3];
//          pubMsg.topic_id[1] = rx_buffer[4];
//          pubMsg.msg_id[0] = rx_buffer[5];
//          pubMsg.msg_id[1] = rx_buffer[6];

            for(i=6;i<Msg.Length;i++)
            {
                Serial.print(rx_buffer[i]);
            }
            Serial.println();
  
            if(rx_buffer[6] == '1')
            {
              digitalWrite(pin_led,HIGH);
              delay(100);
              mrf.send16(GW_ADDRESS,PUBLISH_MSGON,sizeof(PUBLISH_MSGON));
            }
            if(rx_buffer[6] == '0')
            {
              digitalWrite(pin_led,LOW);
              delay(100);
              mrf.send16(GW_ADDRESS,PUBLISH_MSGOFF,sizeof(PUBLISH_MSGOFF));
            }
          }
          else
          {
            Serial.println("Client is not subscribed");
          }
          message_received = false;
        } break;
      }
    }
    
    if(node_connected)
    {
      if (current_time - last_ping > ping_interval)
      {
        last_ping = current_time;
        Serial.println("Sending PINGREQ...");
        mrf.send16(GW_ADDRESS,PINGREQ_MSG,sizeof(PINGREQ_MSG));  // Send PING request
      }
    }

  current_time = millis();
  
  if(current_time - last_pingresp > timeout_interval)
  {
    last_pingresp = current_time;
    Serial.println("Connection timeout...");
    connection_timeout = true;
    node_connected = false;
    message_received = false;
    node_subscribed = false;
  }  

  if(node_connected)
    if(current_time - last_pub > pub_interval)
    {
      current_time = millis();
      last_pub = current_time;

      if(node_subscribed==false)
      {
        Serial.println("Sending SUBSCRIBE...");
        mrf.send16(GW_ADDRESS,SUBSCRIBE_MSG,sizeof(SUBSCRIBE_MSG));
      }
      
    }
    
}

void handle_rx() {
    uint8_t i;
    
    digitalWrite(LED_BUILTIN, LOW);
    delay(100);
//    Serial.println("Packet received, waiting 100 ms");
    rx_len = mrf.rx_datalength();
    for (i = 0; i < rx_len; i++)
    {
          rx_buffer[i] = mrf.get_rxinfo()->rx_data[i];
    }
//    client_address = rx_buffer[7];
    message_received = true;
    digitalWrite(LED_BUILTIN, HIGH);
}

void handle_tx() {
//    if (mrf.get_txinfo()->tx_ok) {
//        Serial.println("802.15.4 TX went ok, got ack");
//    } else {
//        Serial.print("802.15.4 TX failed after ");Serial.print(mrf.get_txinfo()->retries);Serial.println(" retries\n");
//    }
}
 
