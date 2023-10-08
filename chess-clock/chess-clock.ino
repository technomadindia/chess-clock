#include <TM1637Display.h>

// hardware pin connections
#define W_TIMER_CLK_PIN 5
#define W_TIMER_DIO_PIN 4
#define B_TIMER_CLK_PIN 6
#define B_TIMER_DIO_PIN 7
#define WHITE_MOVE_PIN 14
#define BLACK_MOVE_PIN 15
#define START_PAUSE_PIN 8
#define KNOB1_CLK_PIN 2
#define KNOB1_DT_PIN 11
#define KNOB1_SW_PIN 12
#define ALERT_PIN 13

// constants
enum STATES { BEGIN,
              READY,
              W_PLAYING,
              B_PLAYING,
              W_PAUSED,
              B_PAUSED,
              W_END,
              B_END,
              BASE_TIME_CONFIG,
              BONUS_TIME_CONFIG };
const uint8_t TIMER_SEG_CLEAR[] = {0x00, 0x00, 0x00, 0x00};
const uint8_t TIMER_SEG_LOSE[] = {
    SEG_D | SEG_E | SEG_F,
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
    SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G};
const uint8_t TIMER_SEG_END[] = {
    SEG_A | SEG_D | SEG_E | SEG_F | SEG_G,
    SEG_C | SEG_E | SEG_G,
    SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,
    0x00};
const int ALERT_DURATION = 1000;
const long BASE_TIME_OPTIONS[] = { // all time measured in milliseconds
    60000, 120000, 180000, 300000, 600000, 900000,
    1200000, 1800000, 2700000, 3600000, 4500000, 5400000};
const int NUM_BASE_TIME_OPTIONS = 12;
const long BONUS_TIME_OPTIONS[] = {
    0, 1000, 2000, 3000, 5000, 10000, 15000,
    20000, 30000, 45000, 60000, 75000};
const int NUM_BONUS_TIME_OPTIONS = 12;

// data
STATES state_machine = STATES::BEGIN;
int selected_base_time_pos = 4;
int selected_bonus_time_pos = 0;
long selected_base_time = BASE_TIME_OPTIONS[selected_base_time_pos];
long selected_bonus_time = BONUS_TIME_OPTIONS[selected_bonus_time_pos];
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
long alert_time = 0;

// input states
int start_pause_button_state = LOW;
int start_pause_button_prev_state = LOW;
int white_move_button_state = LOW;
int black_move_button_state = LOW;
int mode_change_button_state = LOW;
int mode_change_button_prev_state = LOW;
int config_knob_clk_state = LOW;
int config_knob_dt_state = LOW;

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
    // attachInterrupt(digitalPinToInterrupt(WHITE_MOVE_PIN), white_move_button_event, RISING);
    pinMode(BLACK_MOVE_PIN, INPUT);
    // attachInterrupt(digitalPinToInterrupt(BLACK_MOVE_PIN), black_move_button_event, RISING);
    pinMode(START_PAUSE_PIN, INPUT);
    pinMode(KNOB1_SW_PIN, INPUT);
    pinMode(KNOB1_CLK_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(KNOB1_CLK_PIN), knob1_clk_event, RISING);
    pinMode(KNOB1_DT_PIN, INPUT);
    pinMode(ALERT_PIN, OUTPUT);
}

// ISR for white 'move complete' button
/* void white_move_button_event() {
    if (STATES::W_PLAYING == state_machine) {
        white_move_button_state = HIGH;
    }
} */

// ISR for black 'move complete' button
/* void black_move_button_event() {
    if (STATES::B_PLAYING == state_machine) {
        black_move_button_state = HIGH;
    }
} */

// ISR for 'knob1 rotate' event
void knob1_clk_event() {
    if (STATES::BASE_TIME_CONFIG == state_machine) {
        config_knob_clk_state = digitalRead(KNOB1_CLK_PIN);
        config_knob_dt_state = digitalRead(KNOB1_DT_PIN);
        if (config_knob_dt_state != config_knob_clk_state) {
            selected_base_time_pos++;
        } else {
            selected_base_time_pos--;
        }
    } else if (STATES::BONUS_TIME_CONFIG == state_machine) {
        config_knob_clk_state = digitalRead(KNOB1_CLK_PIN);
        config_knob_dt_state = digitalRead(KNOB1_DT_PIN);
        if (config_knob_dt_state != config_knob_clk_state) {
            selected_bonus_time_pos++;
        } else {
            selected_bonus_time_pos--;
        }
    }
}

// the loop function runs over and over again forever
void loop() {
    // capture external input status
    start_pause_button_state = digitalRead(START_PAUSE_PIN);
    white_move_button_state = digitalRead(WHITE_MOVE_PIN);
    black_move_button_state = digitalRead(BLACK_MOVE_PIN);
    mode_change_button_state = digitalRead(KNOB1_SW_PIN);

    switch (state_machine) {
    default:
    case STATES::BEGIN:
        init_time_limits();
        change_state_to(STATES::READY);
        break;

    case STATES::READY:
        update_w_timer_display(w_time);
        update_b_timer_display(b_time);

        // start the game
        if (HIGH == start_pause_button_state && LOW == start_pause_button_prev_state) {
            init_timers();
            change_state_to(STATES::W_PLAYING);
        }

        // change mode
        if (HIGH == mode_change_button_state && LOW == mode_change_button_prev_state) {
            update_w_timer_display(selected_base_time);
            update_b_timer_display(selected_bonus_time);
            change_state_to(STATES::BASE_TIME_CONFIG);
        }
        break;

    case STATES::W_PLAYING:
        current_time = millis();
        // complete white move
        if (HIGH == white_move_button_state) {
            b_diff += current_time - play_time;
            play_time = current_time;
            // w_total_time += bonus_time;
            w_time = w_total_time - (current_time - w_diff);
            change_state_to(STATES::B_PLAYING);
            // white_move_button_state = LOW;
        } else {
            w_time = w_total_time - (current_time - w_diff);
        }
        update_w_timer_display(w_time);

        // pause game at white move
        if (HIGH == start_pause_button_state && LOW == start_pause_button_prev_state) {
            pause_time = millis();
            change_state_to(STATES::W_PAUSED);
        }

        // white timeout => black wins
        if (w_time <= 0) {
            alert_start();
            change_state_to(STATES::W_END);
        }
        break;

    case STATES::B_PLAYING:
        current_time = millis();
        // complete black move
        if (HIGH == black_move_button_state) {
            w_diff += current_time - play_time;
            play_time = current_time;
            // b_total_time += bonus_time;
            b_time = b_total_time - (current_time - b_diff);
            change_state_to(STATES::W_PLAYING);
            // black_move_button_state = LOW;
        } else {
            b_time = b_total_time - (current_time - b_diff);
        }
        update_b_timer_display(b_time);

        // pause game at black move
        if (HIGH == start_pause_button_state && LOW == start_pause_button_prev_state) {
            pause_time = millis();
            change_state_to(STATES::B_PAUSED);
        }

        // black timeout => white wins
        if (b_time <= 0) {
            alert_start();
            change_state_to(STATES::B_END);
        }
        break;

    case STATES::W_PAUSED:
        // unpause white move
        if (HIGH == start_pause_button_state && LOW == start_pause_button_prev_state) {
            current_time = millis();
            w_diff += current_time - pause_time;
            change_state_to(STATES::W_PLAYING);
        }
        break;

    case STATES::B_PAUSED:
        // unpause black move
        if (HIGH == start_pause_button_state && LOW == start_pause_button_prev_state) {
            current_time = millis();
            b_diff += current_time - pause_time;
            change_state_to(STATES::B_PLAYING);
        }
        break;

    case STATES::W_END:
        // print white lose message
        w_timer_display.setSegments(TIMER_SEG_END);
        alert_stop();
        break;

    case STATES::B_END:
        // print black lose message
        b_timer_display.setSegments(TIMER_SEG_END);
        alert_stop();
        break;

    case STATES::BASE_TIME_CONFIG:
        if (selected_base_time_pos >= NUM_BASE_TIME_OPTIONS) {
            selected_base_time_pos %= NUM_BASE_TIME_OPTIONS;
        } else if (selected_base_time_pos < 0) {
            selected_base_time_pos += NUM_BASE_TIME_OPTIONS;
        }

        selected_base_time = BASE_TIME_OPTIONS[selected_base_time_pos];
        update_w_timer_display(selected_base_time);

        if (HIGH == mode_change_button_state && LOW == mode_change_button_prev_state) {
            change_state_to(STATES::BONUS_TIME_CONFIG);
        }
        break;

    case STATES::BONUS_TIME_CONFIG:
        if (selected_bonus_time_pos >= NUM_BONUS_TIME_OPTIONS) {
            selected_bonus_time_pos %= NUM_BONUS_TIME_OPTIONS;
        } else if (selected_bonus_time_pos < 0) {
            selected_bonus_time_pos += NUM_BONUS_TIME_OPTIONS;
        }

        selected_bonus_time = BONUS_TIME_OPTIONS[selected_bonus_time_pos];
        update_b_timer_display(selected_bonus_time);

        if (HIGH == mode_change_button_state && LOW == mode_change_button_prev_state) {
            init_time_limits();
            change_state_to(STATES::READY);
        }
        break;
    }

    // save current input status to detect change
    start_pause_button_prev_state = start_pause_button_state;
    mode_change_button_prev_state = mode_change_button_state;

    // rate limit iterations
    delay(100);
}

// change STATE MACHINE states
void change_state_to(STATES final_state) {
    state_machine = final_state;
}

// initialize game time limits
void init_time_limits() {
    w_total_time = selected_base_time;
    b_total_time = selected_base_time;
    bonus_time = selected_bonus_time;
    w_time = w_total_time;
    b_time = b_total_time;
    w_moves = 0;
    b_moves = 0;
}

// initialize timers
void init_timers() {
    current_time = millis();
    w_diff = current_time;
    b_diff = current_time;
    play_time = current_time;
}

// activate alert buzzer
void alert_start() {
    alert_time = millis();
    digitalWrite(ALERT_PIN, HIGH);
}

// deactivate alert buzzer after set duration
void alert_stop() {
    current_time = millis();
    if (current_time - alert_time > ALERT_DURATION) {
        alert_time = 0;
        digitalWrite(ALERT_PIN, LOW);
    }
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
void update_w_timer_display(long time) {
    convert_time_to_clock(time, w_timer_data);

    // convert BCD to 7 segment digits
    w_timer_data[0] = w_timer_display.encodeDigit(w_timer_data[0]);
    w_timer_data[1] = w_timer_display.encodeDigit(w_timer_data[1]) | 0x80;
    w_timer_data[2] = w_timer_display.encodeDigit(w_timer_data[2]);
    w_timer_data[3] = w_timer_display.encodeDigit(w_timer_data[3]);

    // refresh LED segment display
    w_timer_display.setSegments(w_timer_data, 4, 0);
}

// update black remaining time on display
void update_b_timer_display(long time) {
    convert_time_to_clock(time, b_timer_data);

    // convert BCD to 7 segment digits
    b_timer_data[0] = b_timer_display.encodeDigit(b_timer_data[0]);
    b_timer_data[1] = b_timer_display.encodeDigit(b_timer_data[1]) | 0x80;
    b_timer_data[2] = b_timer_display.encodeDigit(b_timer_data[2]);
    b_timer_data[3] = b_timer_display.encodeDigit(b_timer_data[3]);

    // refresh LED segment display
    b_timer_display.setSegments(b_timer_data, 4, 0);
}
