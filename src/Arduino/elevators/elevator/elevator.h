#ifndef ELEVATOR_OBJ_H
#define ELEVATOR_OBJ_H

#include <stdint.h>

#ifndef _cplusplus
#include <stdbool.h>
#endif

#include <fsm.h>
#include <list.h>


#ifdef __cplusplus
extern "C" {
#endif

/* Elevator constants */

#define ELEVATOR_STATE_CNT 4


/**Movement types
 * 
 * These are the types of movements the elevator can take. This
 * is used as a convinient measure to state the elevator's 
 * direction lock.
*/
enum movement
{
    STOP,
    DOWN,
    UP
};


/**Door status
 * 
 * These are the door positions the elevator's door can possibly
 * take.
*/
enum door_status
{
    CLOSE_DOOR,
    OPEN_DOOR
};


/**Light status
 * 
 * Possible states the light can take.
*/
enum light_status
{
    LIGHTS_OFF,
    LIGHTS_ON
};


// An elevator's initial state parameters
enum init_state
{
    NULL_FLOOR = 0,
    ROOM_TEMPERATURE = 68,
    ZERO_WEIGHT = 0,
    NO_PEOPLE = 0,
};


enum action_state
{
    END,
    START
};


enum elevator_states
{
    IDLE,
    MOVING,
    EMERGENCY,
    MAINTENANCE
};

/* Elevator user-defined types */

/**Person object
 * 
 * This object holds a information about a person that enters the
 * elevator. It is used to keep track of the changes in the state
 * of the elevator car itself. When a person enters or leaves the
 * elevator car, this will be referenced to change the weight and
 * temperature of the elevator.
*/
typedef struct
{
    uint8_t temp;     // Temperature change the person added
    uint8_t weight;   // Weight of the person

} person_t;


// Current state of the elevator
typedef struct
{
    uint8_t temp;          // Current temperature
    uint8_t floor;         // Current floor
    uint16_t weight;       // Current weight
    uint8_t headcount;     // Current people count
    uint8_t is_light_on;   // Current light state
    uint8_t is_door_open;  // Current door state

} car_state_t;


// Limits of the elevator
typedef struct
{
    const uint8_t floor;     // Maximum floor the elevator can reach
    const uint8_t h_temp;    // Maximum temperature the cab can reach
    const uint8_t l_temp;    // Minimum temperature the cab can reach
    const uint16_t weight;   // Maximum weight the elevator can handle
    const uint8_t capacity;  // Maximum people capacity

} car_limits_t;


// Convinient attributes
typedef struct
{
    uint8_t move;             // States current movement
    uint64_t init_time;       // Initial time marker for an elevator operation
    uint8_t next_floor;       // Next floor the elevator must go to
    bool action_started;      // Indicate whether or not an action has started
    bool* pressed_floors;     // 
    list_node_t ** riders;    // Array that lists stores people info for each floor
    bool maintenance_needed;  // Elevator needs maintenance

} car_attrs_t;


/**Elevator object
 * 
 * This is an elevator object. It contains the behavior, quantitative
 * and qualitative descriptions, and the limits the elevator can reach.
*/
typedef struct
{
    fsm_t behavior;       // Behavior of the elevator
    car_state_t state;    // State of the elevator
    car_attrs_t attrs;    // Additional attributes of the elevator
    car_limits_t limits;  // Limits of the elevator

} elevator_t;


/* Elevator variables */

extern state_t idle;
extern state_t moving;
extern state_t emergency;
extern state_t maintenance;

/* Elevator methods */

// Assign the direction the elevator should take to reach the next floor
#define assign_elevator_direction(car) car->attrs.move = (car->attrs.next_floor > car->state.floor)? UP: DOWN

/**Check if elevator was initialized correctly
 * 
 * An elevator is said to have been initialized correctly if the 
 * state in the state machine and the rider list array were 
 * allocated in memory (i.e., they are not NULL.) An elevator
 * that was not initialized correctly will be called a null
 * elevator.
*/
#define elevator_is_null(car) (car->attrs.riders == NULL) || (car->behavior == NULL) || (car->attrs.pressed_buttons == NULL)


/**See what floor was requested
 * 
 * This verifies if the the floor was requested by a person. If no
 * person has requested, this will yield false (i.e., the rider's
 * list for that floor is empty.) Otherwise, if a person has requested
 * this floor, then it will give out a true value (i.e., the rider's
 * list for that floor will have at least one member.)
*/
#define floor_was_requested(car, floor) get_list_length(car->attrs.riders[floor]) != 0


// Set the elevator to move on a specific direction
#define set_elevator_movement(car, move) car->attrs.move = move


// Shorthand for running an elevator's state machine
#define run_elevator(car) car->behavior.run_FSM(&car->behavior, car)


// Shorthand for verifying if elevator is currently performing an action
#define elevator_performing_action(car) car->attrs.action_started


// Specify if the elevator is performing an action or not
#define set_elevator_action(car, action) car->attrs.action_started = action

// Verify if the elevator is within its limits
#define elevator_within_limits(car) (car->state.weight < car->limits.weight) && (car->limits.l_temp < car->state.temp) && (car->state.temp < car->limits.h_temp)


uint8_t * get_temp(elevator_t * car);
uint8_t * get_floor(elevator_t * car);
uint8_t * get_weight(elevator_t * car);
uint8_t * get_door_state(elevator_t * car);
uint8_t * get_light_state(elevator_t * car);

void set_floor(elevator_t * car, uint8_t * floor);
void set_weight(elevator_t * car, uint8_t * weight);
void set_door_state(elevator_t * car, uint8_t * state);
void set_temperature(elevator_t * car, uint8_t * temp);
void set_light_state(elevator_t * car, uint8_t * state);

void exit_elevator(elevator_t * car);
void move_elevator(elevator_t * car);
void deinit_elevator(elevator_t * car);
uint8_t find_next_floor(elevator_t * car);
elevator_t init_elevator(uint8_t * limits);
void enter_elevator(elevator_t * car, uint8_t * attrs);

#ifdef __cplusplus
}
#endif

#endif