#include <stdio.h>
#include <stdint.h>

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


/**Create a state machine
 * 
 * If no states are provided (i.e., if the argument is NULL), then
 * the code will manually allocate memory, so you can later add
 * the states.
 */ 
fsm_t create_fsm(uint8_t state_cnt, state_t ** states)
{
    fsm_t fsm;

    fsm.state_cnt = state_cnt;
    fsm.curr_state = INVALID_STATE;

    if (states == NULL)
    {
        fsm.states = malloc(sizeof(state_t *) * state_cnt);

        if (fsm.states != NULL)
        {
            for (uint8_t i = 0; i < state_cnt; i++)
            {
                fsm.states[i] = NULL;
            }
        }     
    }
    else
    {
        fsm.states = states;
    }

    return fsm;
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