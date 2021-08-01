#ifndef fsm_t_H
#define fsm_t_H

#include <stdint.h>

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
    uint16_t curr_state;  // Current active state in the state machine

} fsm_t;


// FSM function prototypes

void deinit_fsm(fsm_t * fsm);
fsm_t create_fsm(uint8_t state_cnt);
void run_fsm(fsm_t * fsm, void * args);
void add_state(fsm_t * fsm, state_t * state, uint8_t id);

#ifdef __cplusplus
}
#endif

#endif