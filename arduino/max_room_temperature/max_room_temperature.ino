#include "DHT.h"
#include "Timer.h"
#include <RFM69.h>
#include <SPI.h>

#define SERIAL_BAUD   115200

// Timer setup
//
Timer t;

// Radio setup
//
#define NODE_ID        2
#define NODE_NAME      "max-room-temp"
#define NETWORK_ID     100
#define GATEWAY_ID     1
#define ENCRYPT_KEY    "a7bd91kaxlchdk36"
#define FREQUENCY      RF69_433MHZ
#define ACK_TIME       30
#define LIGHT_PIN      9

RFM69 radio;
int TRANSMIT_PERIOD =  300;

// Temperature & Humidity setup
//
#define TEMP_SENSOR_DELAY  5000
#define TEMP_SENSOR_PIN   3
#define TEMP_SENSOR_TYPE  DHT22

DHT dht(TEMP_SENSOR_PIN, TEMP_SENSOR_TYPE);

// Accelerometer setup
//
const int x = A0;
const int y = A1;
const int z = A2;

int x_min, x_max, x_val;
int y_min, y_max, y_val;
int z_min, z_max, z_val;

int tolerance = 6;
boolean calibrated = false;

char serial_message[1024];

void calibrate_accelerometer() {

  calibrated = false;

  x_val = analogRead(x);
  x_min = x_val;
  x_max = x_val;

  y_val = analogRead(y);
  y_min = y_val;
  y_max = y_val;

  z_val = analogRead(z);
  z_min = z_val;
  z_max = z_val;

  for (int i = 0; i < 50; i++){

    // Calibrate X Values
    x_val = analogRead(x);

    if (x_val > x_max){
      x_max = x_val;
    } else if (x_val < x_min){
      x_min=x_val;
    }

    // Calibrate Y Values
    y_val = analogRead(y);

    if (y_val>y_max){
      y_max=y_val;
    } else if (y_val < y_min){
      y_min=y_val;
    }

    // Calibrate Z Values
    z_val = analogRead(z);

    if (z_val>z_max){
      z_max=z_val;
    } else if (z_val < z_min){
      z_min=z_val;
    }

    delay(10);
  }

  debug_print("accelerometer:calibrated");

  calibrated = true;
}

int check_motion() {

  int score = 0;

  x_val = analogRead(x);
  y_val = analogRead(y);
  z_val = analogRead(z);

  if (x_val > (x_max + tolerance) || x_val < (x_min - tolerance)) {
    score = score + x_val;
    sprintf(serial_message, "accelerometer:x value=%d", x_val);
    debug_print(serial_message);
  }

  if (y_val > (y_max + tolerance) || y_val < (y_min - tolerance)) {
    score = score + y_val;
    sprintf(serial_message, "accelerometer:y value=%d", y_val);
    debug_print(serial_message);
  }

  if (z_val > (z_max+tolerance) || z_val < (z_min - tolerance)) {
    score = score + z_val;
    sprintf(serial_message, "accelerometer:z value=%d", z_val);
    debug_print(serial_message);
  }

  return score;
}

void read_accelerometer(void *context) {

  if (calibrated) {
    int motionScore = check_motion();

    if (motionScore > 0) {
      sprintf(serial_message, "accelerometer:%d", motionScore);
      debug_print(serial_message);
      calibrate_accelerometer();
    }
  }
}

void read_temperature_and_humidity(void *context) {
  float offset = 0.0;
  //float offset = 1.955;

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  t = t - offset;

  static char temperature[6];
  static char humidity[6];
  char buff[60];

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(t) || isnan(h)) {
    debug_print("error!Failed to read from temperature sensor");
  } else {
    // Temperature & Humidity
    dtostrf(t, 0, 2, temperature);
    dtostrf(h, 0, 2, humidity);
    sprintf(buff, "temperature:%s,humidity:%s", temperature, humidity);
    send_data(buff);
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

    for (byte i = 0; i < radio.DATALEN; i++) {
      Serial.print((char)radio.DATA[i]);
    }

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

  // Radio
  //
  setup_radio();

  // Accelerometer
  //
  // calibrate_accelerometer();
  // t.every(1, read_accelerometer, (void*)0);

  // Temperature & Humidity
  //
  dht.begin();
  t.every(TEMP_SENSOR_DELAY, read_temperature_and_humidity, (void*)0);

  // Radio ACK's
  //
  t.every(1, radio_ack_check, (void*)0);
}

void loop() {

  t.update();
}

