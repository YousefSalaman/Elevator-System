#include <Arduino.h>

#include "elevator.h"


/* Elevator behavior constants */

#define ELEVATOR_MOVE_TIME 3000  // Time the elevator takes to move from floor to another


enum idle_stages
{
    CLOSE_DOOR_TIME = 4000,
    LIGHTS_OFF_TIME = 10000
};


// state_t prototypes

static void idle_run(void * args);
static void moving_run(void * args);
static void emergency_run(void * args);

static uint8_t idle_change(void * args);
static uint8_t moving_change(void * args);
static uint8_t emergency_change(void * args);
static uint8_t maintenance_change(void * args);


// State for elevators

static state_t idle = {.run = idle_run, .change = idle_change};
static state_t moving = {.run = moving_run, .change = moving_change};
static state_t emergency = {.run = emergency_run, .change = emergency_change};
static state_t maintenance = {.run = emergency_run, .change = maintenance_change};

state_t * elevator_states[] = {&idle, &moving, &emergency, &maintenance};

// Elevator state functions

/* Emergency state functions */

static void emergency_run(void * args)
{
    uint8_t car_index = *((uint8_t *) args);
    elevator_t * car = get_elevator(car_index);

    if (!car->attrs.action_started)
    {
        car->state.is_door_open = OPEN_DOOR;
        car->state.is_light_on = LIGHTS_ON;
        car->attrs.action_started = START_ACTION;

        // Send updated states to manager
        update_elevator_door_status(car_index, car);
        update_elevator_light_status(car_index, car);
    }
}


static uint8_t emergency_change(void * args)
{
    uint8_t car_index = *((uint8_t *) args);
    elevator_t * car = get_elevator(car_index);

    if (elevator_within_limits(car))
    {
        car->attrs.action_started = END_ACTION;
        return IDLE;
    }
    return EMERGENCY;
}


/* Maintenance state functionS */

static uint8_t maintenance_change(void * args)
{
    uint8_t car_index = *((uint8_t *) args);
    elevator_t * car = get_elevator(car_index);

    if (!car->attrs.maintenance_needed)
    {
        car->attrs.action_started = END_ACTION;
        return IDLE;
    }
    return MAINTENANCE;
}


/* Moving state functions */

static void moving_run(void * args)
{
    uint8_t car_index = *((uint8_t *) args);
    elevator_t * car = get_elevator(car_index);

    if (!car->attrs.action_started)
    {
        car->attrs.action_started = START_ACTION;
        car->attrs.init_time = millis();
        assign_elevator_direction(car);
    }

    // Move from one floor to another
    else if (millis() - car->attrs.init_time > ELEVATOR_MOVE_TIME)
    {
        move_elevator(car, car_index);
        if (car->attrs.next_floor == car->state.floor)
        {
            car->attrs.next_floor = find_next_floor(car);
            car->attrs.move = STOP;  
            exit_elevator(car, car_index);  // Get people out of the elevator

            update_elevator_movement_state(car_index, car);
        }
        car->attrs.action_started = END_ACTION;
    }
}


static uint8_t moving_change(void * args)
{
    uint8_t car_index = *((uint8_t *) args);
    elevator_t * car = get_elevator(car_index);

    if (!elevator_within_limits(car))
    {
        return EMERGENCY;
    }

    if (car->attrs.maintenance_needed)
    {
        return MAINTENANCE;
    }

    if (car->attrs.move == STOP)
    {
        return IDLE;
    }

    return MOVING;
}


/* Idle state functions */

static void idle_run(void * args)
{
    uint8_t car_index = *((uint8_t *) args);
    elevator_t * car = get_elevator(car_index);

    if (!car->attrs.action_started)
    {
        car->state.is_door_open = OPEN_DOOR;
        car->state.is_light_on = LIGHTS_ON;
        car->attrs.action_started = START_ACTION;
        car->attrs.init_time = millis();

        // Send updated states to manager
        update_elevator_door_status(car_index, car);
        update_elevator_light_status(car_index, car);
    }

    // Verify timed events (closing door and turning lights off)
    else if (car->state.is_door_open || car->state.is_light_on)
    {
        bool event_timer_passed = (car->state.is_door_open)? millis() - car->attrs.init_time > CLOSE_DOOR_TIME: millis() - car->attrs.init_time > LIGHTS_OFF_TIME;
    
        // Toggle states after enough time passed
        if (event_timer_passed)
        {
            if (car->state.is_door_open)
            {
                car->state.is_door_open = CLOSE_DOOR;
                update_elevator_door_status(car_index, car);
            }
            else
            {
                car->state.is_light_on = LIGHTS_OFF;
                update_elevator_light_status(car_index, car);
            }
        } 
    }        
}


static uint8_t idle_change(void * args)
{
    elevator_t * car = args;
    uint8_t next_state = IDLE;

    if (!elevator_within_limits(car))
    {
        next_state = EMERGENCY;
    }
    else if (car->attrs.maintenance_needed)
    {
        next_state = MAINTENANCE;
    }
    else if (car->attrs.next_floor && !car->state.is_door_open)
    {
        next_state = MOVING;
    }
    
    // If the next state is not idle, then end the idle action
    if (next_state)
    {
        car->attrs.action_started = END_ACTION;
    }
    
    return next_state;
}