#include <stdint.h>
#include <stddef.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "pwm.h"

static const char *TAG = "pwm-mario";

#define BUZZ_GPIO    GPIO_NUM_18
#define TICK_MS      25
#define GAP_TICKS    1                      // 25 ms silence between notes
#define PAUSE_TICKS  (2000 / TICK_MS)

#define REST 0
#define NOTE_E4 330
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880

typedef struct {
    uint16_t hz;
    uint16_t ms;                            // must be a multiple of TICK_MS
} note_t;

static const note_t k_mario[] = {
    // Intro
    {NOTE_E5, 125}, {NOTE_E5, 125}, {REST, 125}, {NOTE_E5, 125},
    {REST, 125}, {NOTE_C5, 125}, {NOTE_E5, 125}, {REST, 125},
    {NOTE_G5, 250}, {REST, 250},
    {NOTE_G4, 250}, {REST, 250},
    // Phrase 2
    {NOTE_C5, 250}, {REST, 175},
    {NOTE_G4, 250}, {REST, 175},
    {NOTE_E4, 250}, {REST, 125}, {REST, 125},
    {NOTE_A4, 125}, {REST, 125}, {NOTE_B4, 125}, {REST, 125},
    {NOTE_AS4, 125}, {NOTE_A4, 125}, {REST, 125},
    {NOTE_G4, 175}, {NOTE_E5, 175}, {NOTE_G5, 175},
    {NOTE_A5, 125}, {REST, 125},
    {NOTE_F5, 125}, {NOTE_G5, 125}, {REST, 125},
    {NOTE_E5, 125}, {REST, 125},
    {NOTE_C5, 125}, {NOTE_D5, 125}, {NOTE_B4, 250}, {REST, 250},
};
#define SONG_LEN (sizeof(k_mario) / sizeof(k_mario[0]))

// ---------------- FSM ----------------
typedef enum { ST_NOTE, ST_GAP, ST_PAUSE } player_state_t;

static player_state_t s_state = ST_PAUSE;
static uint16_t       s_cnt   = 1;          // first tick loads note 0
static uint8_t        s_gap   = 0;
static size_t         s_idx   = 0;

static void enter(player_state_t st, uint16_t ticks)
{
    s_state = st;
    s_cnt   = ticks;
}

static void load_next(void)
{
    if (s_idx >= SONG_LEN) {
        pwm_off();
        s_idx = 0;
        enter(ST_PAUSE, PAUSE_TICKS);
        return;
    }

    const note_t *n  = &k_mario[s_idx++];
    uint16_t ticks   = n->ms / TICK_MS;

    s_gap = (n->hz && ticks > GAP_TICKS) ? GAP_TICKS : 0;

    if (n->hz) {
        pwm_set_freq(n->hz);
        pwm_set_duty(PWM_DUTY_HALF);
    } else {
        pwm_off();
    }
    enter(ST_NOTE, ticks - s_gap);
}

static void player_tick(void)
{
    if (--s_cnt > 0) return;

    if (s_state == ST_NOTE && s_gap) {
        pwm_off();
        enter(ST_GAP, s_gap);
    } else {
        load_next();                        // NOTE (no gap), GAP, PAUSE
    }
}

// ------------- tick source -------------
static void tick_cb(void *arg) { (void)arg; player_tick(); }

static esp_timer_handle_t s_tick_timer;

void app_main(void)
{
    setup_pwm(BUZZ_GPIO);
    pwm_off();

    const esp_timer_create_args_t args = { .callback = tick_cb, .name = "mario_tick" };
    ESP_ERROR_CHECK(esp_timer_create(&args, &s_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_tick_timer, TICK_MS * 1000));

    ESP_LOGI(TAG, "==== PWM MARIO (tick %d ms) GPIO%d ====", TICK_MS, (int)BUZZ_GPIO);
}