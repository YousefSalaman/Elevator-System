#ifndef FSM_H
#define FSM_H

#include <stdint.h>
#include <stdlib.h>

#ifndef _cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define INVALID_STATE 256  // ID of an invalid state that cannot be reached


/* State machine type and helper types */

typedef void (*run_state_cb)(void *);        // State process function
typedef uint8_t (*change_state_cb)(void *);  // State transition function

// State object
typedef struct
{
    run_state_cb run;        // Callback that runs the current state
    change_state_cb change;  // Callback that transitions between states

} state_t;

// Finite State Machine (FSM) object
typedef struct
{
    state_t ** states;    // List of states
    uint8_t state_cnt;    // Amount of states in the state machine
    state_t * curr_state; // Current active state in the state machine

} fsm_t;


// FSM function prototypes

void run_fsm(fsm_t * fsm, void * args);
fsm_t create_fsm(uint8_t state_cnt, state_t ** states);
void add_state(fsm_t * fsm, state_t * state, uint8_t id);

#define deinit_fsm(fsm) free((fsm)->states)

#ifdef __cplusplus
}
#endif

#endif