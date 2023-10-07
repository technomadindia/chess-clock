#include <TM1637Display.h>

// hardware pin connections
#define W_TIMER_CLK_PIN 5
#define W_TIMER_DIO_PIN 4
#define B_TIMER_CLK_PIN 6
#define B_TIMER_DIO_PIN 7
#define WHITE_MOVE_PIN 2
#define BLACK_MOVE_PIN 3
#define START_PAUSE_PIN 8
#define KNOB1_ENC_A_PIN 14
#define KNOB1_ENC_B_PIN 15
#define KNOB1_SW_PIN 12
#define ALERT_PIN 13

// constants
enum STATES { BEGIN,
              READY,
              W_PLAYING,
              B_PLAYING,
              PAUSED,
              END,
              PLAY_CONFIG,
              BONUS_CONFIG };
const uint8_t TIMER_SEG_CLEAR[] = {0x00, 0x00, 0x00, 0x00};

// data
STATES state_machine = STATES::BEGIN;
long sel_play_time = 600000; // milli seconds
long sel_bonus_time = 10000;
long current_time = 0;
long w_total_time = 0;
long b_total_time = 0;
long bonus_time = 0;
long w_time = 0;
long b_time = 0;
uint8_t w_moves = 0;
uint8_t b_moves = 0;
long w_diff = 0;
long b_diff = 0;
long pause_time = 0;
long play_time = 0;

// input states
int start_pause_button_state = LOW;
int white_move_button_state = LOW;
int black_move_button_state = LOW;
int mode_change_button_state = LOW;

// create module object instances
TM1637Display w_timer_display(W_TIMER_CLK_PIN, W_TIMER_DIO_PIN);
TM1637Display b_timer_display(B_TIMER_CLK_PIN, B_TIMER_DIO_PIN);
uint8_t w_timer_data[] = {0xff, 0xff, 0xff, 0xff};
uint8_t b_timer_data[] = {0xff, 0xff, 0xff, 0xff};

// the setup function runs once when you press reset or power the board
void setup() {
    // initialize LED segment display
    w_timer_display.setBrightness(0x08);
    w_timer_display.setSegments(TIMER_SEG_CLEAR);
    b_timer_display.setBrightness(0x08);
    b_timer_display.setSegments(TIMER_SEG_CLEAR);

    // initialize I/O pins
    pinMode(WHITE_MOVE_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(WHITE_MOVE_PIN), white_move_button_event, RISING);
    pinMode(BLACK_MOVE_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(BLACK_MOVE_PIN), black_move_button_event, RISING);
    pinMode(START_PAUSE_PIN, INPUT);
    pinMode(KNOB1_SW_PIN, INPUT);
    pinMode(KNOB1_ENC_A_PIN, INPUT);
    pinMode(KNOB1_ENC_B_PIN, INPUT);
    pinMode(ALERT_PIN, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
    start_pause_button_state = digitalRead(START_PAUSE_PIN);
    mode_change_button_state = digitalRead(KNOB1_SW_PIN);

    switch (state_machine) {
    default:
    case STATES::BEGIN:
        initialize_timers();
        change_state_to(STATES::READY);
        break;

    case STATES::READY:
        update_w_timer_display();
        update_b_timer_display();

        if (HIGH == start_pause_button_state) {
            change_state_to(STATES::W_PLAYING);
        }
        break;

    case STATES::W_PLAYING:
        current_time = millis();
        if (HIGH == white_move_button_state) {
            b_diff += current_time - play_time;
            play_time = current_time;
            // w_total_time += bonus_time;
            w_time = w_total_time - (current_time - w_diff);
            change_state_to(STATES::B_PLAYING);
            white_move_button_state = LOW;
        } else {
            w_time = w_total_time - (current_time - w_diff);
        }
        update_w_timer_display();
        break;

    case STATES::B_PLAYING:
        current_time = millis();
        if (HIGH == black_move_button_state) {
            w_diff += current_time - play_time;
            play_time = current_time;
            // b_total_time += bonus_time;
            b_time = b_total_time - (current_time - b_diff);
            change_state_to(STATES::W_PLAYING);
            black_move_button_state = LOW;
        } else {
            b_time = b_total_time - (current_time - b_diff);
        }
        update_b_timer_display();
        break;

    case STATES::PAUSED:
        break;

    case STATES::END:
        break;
    }

    delay(100);
}

// change STATE MACHINE states
void change_state_to(STATES final_state) {
    state_machine = final_state;
}

// reset and initialize timers
void initialize_timers() {
    // set time limits
    w_total_time = sel_play_time;
    b_total_time = sel_play_time;
    bonus_time = sel_bonus_time;
    w_time = w_total_time;
    b_time = b_total_time;
    w_moves = 0;
    b_moves = 0;

    // init timers
    current_time = millis();
    w_diff = current_time;
    b_diff = current_time;
    play_time = current_time;
    pause_time = 0;
}

// convert time (in ms) to 4 digit BCD
void convert_time_to_clock(long time, uint8_t clock[]) {
    if (time < 0) {
        time = 0;
    }

    long minute = (time / 1000) / 60;
    long second = (time / 1000) % 60;

    if (minute > 99) { // TODO: fix this temporary hack
        minute = 99;
    }

    clock[0] = minute / 10;
    clock[1] = minute % 10;
    clock[2] = second / 10;
    clock[3] = second % 10;
}

// update white remaining time on display
void update_w_timer_display() {
    convert_time_to_clock(w_time, w_timer_data);

    // convert BCD to 7 segment digits
    w_timer_data[0] = w_timer_display.encodeDigit(w_timer_data[0]);
    w_timer_data[1] = w_timer_display.encodeDigit(w_timer_data[1]) | 0x80;
    // w_timer_data[1] = w_timer_display.encodeDigit(w_timer_data[1]);
    w_timer_data[2] = w_timer_display.encodeDigit(w_timer_data[2]);
    w_timer_data[3] = w_timer_display.encodeDigit(w_timer_data[3]);

    // refresh LED segment display
    w_timer_display.setSegments(w_timer_data, 4, 0);
}

// update black remaining time on display
void update_b_timer_display() {
    convert_time_to_clock(b_time, b_timer_data);

    // convert BCD to 7 segment digits
    b_timer_data[0] = b_timer_display.encodeDigit(b_timer_data[0]);
    b_timer_data[1] = b_timer_display.encodeDigit(b_timer_data[1]) | 0x80;
    // b_timer_data[1] = b_timer_display.encodeDigit(b_timer_data[1]);
    b_timer_data[2] = b_timer_display.encodeDigit(b_timer_data[2]);
    b_timer_data[3] = b_timer_display.encodeDigit(b_timer_data[3]);

    // refresh LED segment display
    b_timer_display.setSegments(b_timer_data, 4, 0);
}

// ISR for white 'move complete' button
void white_move_button_event() {
    white_move_button_state = HIGH;
}

// ISR for black 'move complete' button
void black_move_button_event() {
    black_move_button_state = HIGH;
}
