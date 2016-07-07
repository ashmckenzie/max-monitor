#include <HIDSerial.h>
#include <RFM69sqwr2.h>
#include <SPI.h>
//#include <avr/sleep.h>
//#include <avr/power.h>
//#include <avr/wdt.h>
#include "Timer.h"

//volatile int f_wdt = 1;
//
//ISR(WDT_vect)
//{
//  if(f_wdt == 0)
//  {
//    f_wdt=1;
//  }
//}

HIDSerial serial;

// Timer setup
//
Timer t;

#define BUTTON        4
//#define MOVEMENT_PIN  1

// Light setup
//
#define BUZZER 9
#define GREEN  12
#define RED    8
#define BLUE   13

#define LIGHT_SENSOR A0

// Temperature sensor
//
#define TEMPERATURE_SENSOR A1

#define TOO_MUCH_LIGHT_VALUE    100
#define SUFFICIENT_LIGHT_VALUE  1

// Radio setup
//
#define NODE_ID        3
#define NODE_NAME      "max-room-light"
#define NETWORK_ID     100
#define GATEWAY_ID     1
#define ENCRYPT_KEY    "a7bd91kaxlchdk36"
#define FREQUENCY      RF69_433MHZ
#define ACK_TIME       30

RFM69 radio;
int TRANSMIT_PERIOD =  300;

//void setup_movement() {
//  pinMode(MOVEMENT_PIN, OUTPUT);
//  digitalWrite(MOVEMENT_PIN, LOW);
//}

void setup_light(int color) {
  pinMode(color, OUTPUT);
  digitalWrite(color, LOW);
}

void setup_lights() {
  setup_light(RED);
  setup_light(GREEN);
  setup_light(BLUE);
}

void setup_button() {
  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, HIGH);
}

void too_much_light() {
  debug_print("Too much light detected!");
}

void sufficient_light() {
  debug_print("Sufficient light detected!");
  notify(GREEN, -1);
  movement_detected();
}

void movement_detected() {
  char buff[60];

  sprintf(buff, "movement:true");
  //send_data(buff);

  //digitalWrite(MOVEMENT_PIN, HIGH);
  //delay(500);
  //digitalWrite(MOVEMENT_PIN, LOW);
}

void lights_are_off() {
  debug_print("Looks like the lights are off..");
}

void notify(int color, int delay_ms) {
  flash(color, delay_ms);
}

void flash(int color, int delay_ms) {
  if (delay_ms == -1) { delay_ms = 250; }
  digitalWrite(color, HIGH);
  delay(delay_ms);
  digitalWrite(color, LOW);
}

void read_temperature_value(void *context) {
  float temperature_value;
  char temperature_value_message[40];
  char temperature_value_string[10];

  temperature_value = (float)analogRead(TEMPERATURE_SENSOR);
  temperature_value = (temperature_value * 3.3 / 1024 - 0.5) / 0.01;
  temperature_value = temperature_value - 3.5;

  dtostrf(temperature_value, 2, 2, temperature_value_string);

  sprintf(temperature_value_message, "current temperature_value:%s", temperature_value_string);
  debug_print(temperature_value_message);
}

void read_light_value(void *context) {
  int light_value;
  char light_value_message[30];

  light_value = analogRead(LIGHT_SENSOR);
  sprintf(light_value_message, "current light_value:%d", light_value);
  debug_print(light_value_message);

  if (digitalRead(BUTTON) == LOW) {
    sufficient_light();
  }

  if (light_value > TOO_MUCH_LIGHT_VALUE) {
    too_much_light();
  } else if (light_value > SUFFICIENT_LIGHT_VALUE) {
    sufficient_light();
  } else if (light_value < TOO_MUCH_LIGHT_VALUE) {
    lights_are_off();
  }
}

void debug_print(char *message) {
  serial.println(message);
}

void start_up() {
  debug_print("Starting up...");
  flash(RED, -1);
  flash(GREEN, -1);
  flash(BLUE, -1);
}

void setup_radio() {
  radio.initialize(FREQUENCY, NODE_ID, NETWORK_ID);
  radio.encrypt(ENCRYPT_KEY);
}

void send_data(char *message) {
  byte buffLen;
  char buff[60];

  sprintf(buff, "%s:%s", NODE_NAME, message);
  buffLen = strlen(buff);

  debug_print(buff);
  radio.send(GATEWAY_ID, buff, buffLen);
  delay(TRANSMIT_PERIOD);
}

// void radio_ack_check(void *context) {
//   if (radio.receiveDone()) {
//     for (byte i = 0; i < radio.DATALEN; i++) {
//       serial.print((char)radio.DATA[i]);
//     }
//   }
// }

//void enter_sleep(void) {
//  set_sleep_mode(SLEEP_MODE_PWR_DOWN);   /* EDIT: could also use SLEEP_MODE_PWR_DOWN for lowest power consumption. */
//  sleep_enable();
//  /* Now enter sleep mode. */
//  sleep_mode();
//  /* The program will continue from here after the WDT timeout*/
//  sleep_disable(); /* First thing to do is disable sleep. */
//  /* Re-enable the peripherals. */
//  power_all_enable();
//}

/*-----------------------------------------------------------------------------*/

void setup() {
  serial.begin();

  //setup_radio();
  //setup_movement();
  setup_button();
  setup_lights();
  start_up();

  t.every(1000, read_light_value, (void*)0);
  t.every(3000, read_temperature_value, (void*)0);
  // t.every(1, radio_ack_check, (void*)0);

//  MCUSR &= ~(1<<WDRF);
//  WDTCSR |= (1<<WDCE) | (1<<WDE);
//  WDTCSR = 1<<WDP0 | 1<<WDP1 | 1<<WDP2;  // 2.0 seconds
//  WDTCSR |= _BV(WDIE);
}

void loop() {
  t.update();

//  if (f_wdt == 1) {
//    read_light_value(NULL);
//    read_temperature_value(NULL);
//    f_wdt = 0;
//    enter_sleep();
//  }
}

