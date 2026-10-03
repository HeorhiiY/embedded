#include "lock_fsm.h"

#include <stdbool.h>
#include <string.h>

/* Compares all digits regardless of where the first mismatch is. */
static bool code_matches(const lock_fsm_t *fsm)
{
    uint8_t diff = 0;
    for (int i = 0; i < LOCK_CODE_LEN; i++) {
        diff |= fsm->current_attempt[i] ^ fsm->correct_code[i];
    }
    return diff == 0;
}

/* Drops a partial entry. Wipes the digits too, so a half-typed code is not
 * left sitting in RAM. Leaves state and failed_attempts alone. */
static void clear_entry(lock_fsm_t *fsm)
{
    memset(fsm->current_attempt, 0, LOCK_CODE_LEN);
    fsm->digits_entered = 0;
}

static void handle_digit_in_locked(lock_fsm_t *fsm, uint8_t digit)
{
    fsm->current_attempt[fsm->digits_entered++] = digit;

    if (fsm->digits_entered < LOCK_CODE_LEN) {
        return;                                 /* still entering */
    }

    /* 4th digit: evaluate first, the entry is cleared below either way */
    if (code_matches(fsm)) {
        fsm->failed_attempts = 0;
        fsm->state = LOCK_STATE_OPEN;
    } else if (++fsm->failed_attempts >= LOCK_MAX_ATTEMPTS) {
        fsm->state = LOCK_STATE_BLOCKED;
    }
    /* else: wrong code, attempts left -> stay LOCKED */

    clear_entry(fsm);                           /* always start fresh */
}

void lock_fsm_init(lock_fsm_t *fsm,
                   const uint8_t code[LOCK_CODE_LEN],
                   lock_state_t initial_state)
{
    memset(fsm, 0, sizeof(*fsm));
    memcpy(fsm->correct_code, code, LOCK_CODE_LEN);
    fsm->state = initial_state;
}

lock_state_t lock_fsm_handle_event(lock_fsm_t *fsm, const lock_event_t *evt)
{
    /* State-independent: valid everywhere, never transitions. Handled here so
     * it cannot be forgotten when a new state is added below. */
    if (evt->type == LOCK_EVT_RESET) {
        clear_entry(fsm);
        return fsm->state;
    }

    switch (fsm->state) {   /* no default: -Wswitch flags unhandled states */

    case LOCK_STATE_LOCKED:
        if (evt->type == LOCK_EVT_DIGIT && evt->digit <= 9) {
            handle_digit_in_locked(fsm, evt->digit);
        }
        break;

    case LOCK_STATE_OPEN:
        if (evt->type == LOCK_EVT_LOCK) {
            fsm->state = LOCK_STATE_LOCKED;
        }
        break;

    case LOCK_STATE_BLOCKED:
        if (evt->type == LOCK_EVT_UNBLOCK) {
            fsm->failed_attempts = 0;
            fsm->state = LOCK_STATE_LOCKED;
        }
        break;
    }

    return fsm->state;
}

const char *lock_state_name(lock_state_t state)
{
    switch (state) {
    case LOCK_STATE_OPEN:    return "OPEN";
    case LOCK_STATE_LOCKED:  return "LOCKED";
    case LOCK_STATE_BLOCKED: return "BLOCKED";
    }
    return "?";
}
