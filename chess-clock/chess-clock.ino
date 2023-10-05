#include <TM1637Display.h>

// hardware pin connections
const int W_TIMER_CLK_PIN = 5;
const int W_TIMER_DIO_PIN = 4;
const int B_TIMER_CLK_PIN = 6;
const int B_TIMER_DIO_PIN = 7;
const int WHITE_MOVE_PIN = 2;
const int BLACK_MOVE_PIN = 3;
const int START_PAUSE_PIN = 8;
const int KNOB1_ENC_A_PIN = 14;
const int KNOB1_ENC_B_PIN = 15;
const int KNOB1_SW_PIN = 12;
const int BUZZER_PIN = 13;

// data

const uint8_t TIMER_SEG_CLEAR[] = {0x00, 0x00, 0x00, 0x00};
uint8_t w_timer_data[] = {0xff, 0xff, 0xff, 0xff};
uint8_t b_timer_data[] = {0xff, 0xff, 0xff, 0xff};

// create module object instances
TM1637Display w_timer_display(W_TIMER_CLK_PIN, W_TIMER_DIO_PIN);
TM1637Display b_timer_display(B_TIMER_CLK_PIN, B_TIMER_DIO_PIN);

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  w_timer_display.setBrightness(0x0f);
  w_timer_display.setSegments(w_timer_data);
  b_timer_display.setBrightness(0x08);
  b_timer_display.setSegments(b_timer_data);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
}
