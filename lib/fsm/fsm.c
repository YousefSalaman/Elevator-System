#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "fsm.h"


/* Public FSM functions */


// Add a state to a state machine
void add_state(fsm_t * fsm, state_t * state, uint8_t id)
{
    if (id < fsm->state_cnt && fsm->states[id] == NULL)
    {
        fsm->states[id] = state;
    }
    else 
    {
        printf("The id %u cannot be or already has been used to register a state\n", id);
    }
}


// Create a state machine
fsm_t create_fsm(uint8_t state_cnt)
{
    fsm_t fsm;

    fsm.state_cnt = state_cnt;
    fsm.curr_state = INVALID_STATE;
    fsm.states = malloc(sizeof(state_t *) * state_cnt);

    if (fsm.states != NULL)
    {
        for (uint8_t i = 0; i < state_cnt; i++)
        {
            fsm.states[i] = NULL;
        }
    }

    return fsm;
}


// Uninitialiazed a state machine
void deinit_fsm(fsm_t * fsm)
{
    if (fsm->states != NULL)
    {
        state_t * temp_state;
        state_t * state = *fsm->states;

        for (uint8_t i = 0; i < fsm->state_cnt; state = temp_state)
        {
            temp_state = fsm->states[++i];  // Set temp_state as next element in array
            if (state != NULL)
            {
                free(state);
            }
        }

    }
}


/**Run the state machine
 * 
 * This will run the currently stored state and select what will be 
 * the next state. If no state is found with the extracte id, then
 * the FSM's current state will be set to NULL.
*/
void run_fsm(fsm_t * fsm, void * args)
{
    if (fsm->curr_state != INVALID_STATE)
    {
        state_t * curr_state = fsm->states[fsm->curr_state];

        curr_state->run(args);  // Run current state

        // Get the state to be ran in next iteration
        uint8_t id;
        fsm->curr_state = ((id = curr_state->change(args)) < fsm->state_cnt)? id: INVALID_STATE;
    }
}