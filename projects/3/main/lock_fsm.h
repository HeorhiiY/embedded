#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LOCK_CODE_LEN      4
#define LOCK_MAX_ATTEMPTS  3

typedef enum {
    LOCK_STATE_OPEN,
    LOCK_STATE_LOCKED,
    LOCK_STATE_BLOCKED,
} lock_state_t;

typedef enum {
    LOCK_EVT_DIGIT,     /* payload: digit 0..9 */
    LOCK_EVT_LOCK,      /* close the safe (only meaningful in OPEN) */
    LOCK_EVT_UNBLOCK,   /* timeout expired / admin reset (only meaningful in BLOCKED) */
    LOCK_EVT_RESET,     /* drop a partial entry: no transition, attempts untouched */
} lock_event_type_t;

typedef struct {
    lock_event_type_t type;
    uint8_t digit;      /* valid only for LOCK_EVT_DIGIT */
} lock_event_t;

typedef struct {
    lock_state_t state;
    uint8_t correct_code[LOCK_CODE_LEN];
    uint8_t current_attempt[LOCK_CODE_LEN];
    uint8_t digits_entered;     /* number of valid digits in current_attempt */
    uint8_t failed_attempts;
} lock_fsm_t;

void lock_fsm_init(lock_fsm_t *fsm,
                   const uint8_t code[LOCK_CODE_LEN],
                   lock_state_t initial_state);

/* Processes one event, returns the state after the event. */
lock_state_t lock_fsm_handle_event(lock_fsm_t *fsm, const lock_event_t *evt);

const char *lock_state_name(lock_state_t state);

#ifdef __cplusplus
}
#endif
