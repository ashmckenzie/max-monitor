#include "DHT.h"
#include "Timer.h"
#include <RFM69.h>
#include <SPI.h>

#define SERIAL_BAUD   115200

// Timer setup
//
Timer t;

#define DELAY          500

#define MOVEMENT_PIN   13

// Radio setup
//
#define NODE_ID        3
#define NODE_NAME      "max-room"
#define NETWORK_ID     100
#define GATEWAY_ID     1
#define ENCRYPT_KEY    "a7bd91kaxlchdk36"
#define FREQUENCY      RF69_433MHZ
#define ACK_TIME       30
#define LIGHT_PIN      9

RFM69 radio;
int TRANSMIT_PERIOD =  300;

char serial_message[1024];

void read_movement(void *context) {
  if (digitalRead(MOVEMENT_PIN) == HIGH) {
    send_data("movement:true");
  }
}

void send_data(char *message) {
  byte buffLen;
  char buff[60];

  sprintf(buff, "%s:%s", NODE_NAME, message);
  buffLen = strlen(buff);
  print_and_blink_light(buff);
  
  radio.send(GATEWAY_ID, buff, buffLen);
  delay(TRANSMIT_PERIOD);
}

void radio_ack_check(void *context) {
  if (radio.receiveDone()) {
   // Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");

    for (byte i = 0; i < radio.DATALEN; i++) {
      Serial.print((char)radio.DATA[i]);
    }

    //Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI);Serial.print("]");

    //if (radio.ACK_REQUESTED) {
    //  radio.sendACK();
    //  Serial.print(" - ACK sent");
    //  delay(10);
    //}

    blink_light(LIGHT_PIN, 5);
    blank_line();
  }
}

void setup_radio() {
  radio.initialize(FREQUENCY, NODE_ID, NETWORK_ID);
  radio.encrypt(ENCRYPT_KEY);
}

void blink_light(byte pin  , int delay_ms) {
  pinMode(pin, OUTPUT);
  digitalWrite(pin, HIGH);

  delay(delay_ms);
  digitalWrite(pin, LOW);
}

void print_and_blink_light(char *message) {
  blink_light(LIGHT_PIN, 5);
  debug_print(message);
}

void blank_line() {
  Serial.println();
}

void debug_print(char *message) {
  char buff[1024];

  sprintf(buff, "DEBUG: %s", message);
  Serial.println(buff);
}

void setup() {
  
  Serial.begin(SERIAL_BAUD);
  
  pinMode(MOVEMENT_PIN, INPUT);

  // Radio
  //
  setup_radio();

  t.every(DELAY, read_movement, (void*)0);

  // Radio ACK's
  //
  t.every(1, radio_ack_check, (void*)0);
}

void loop() {

  t.update();
}

