#include <string.h>
#include <stdlib.h>

#include "elevator.h"


/* Private elevator constants */

/* Elevator function prototypes*/

static uint8_t find_requested_floors(elevator_t * car);
static car_attrs_t init_misc_elevator_attrs(uint8_t floor_count, uint8_t capacity);


/* Public system elevator functions */

// Set up attributes for elevator object
void set_elevator_attrs(uint8_t car_index, uint8_t floor_count, uint8_t max_temp, uint8_t min_temp, uint8_t capacity, uint16_t weight)
{
    elevator_t * car_p = get_elevator(car_index);

    if (car_p != NULL)
    {
        // Limits for the elevator's car
        car_limits_t car_limits = 
        {
            .floor = floor_count,
            .h_temp = max_temp,
            .l_temp = min_temp,
            .weight = weight
        };

        // Set the elevator's initial state
        car_state_t car_state = 
        {
            .temp = ROOM_TEMPERATURE, 
            .floor = NULL_FLOOR, 
            .weight = ZERO_WEIGHT, 
            .is_light_on = LIGHTS_ON,
            .is_door_open =  CLOSE_DOOR
        };

        // Set up elevator object
        elevator_t car = 
        {
            .attrs = init_misc_elevator_attrs(floor_count, capacity),
            .state = car_state,
            .behavior = create_fsm(ELEVATOR_STATE_CNT, elevator_states),
            .limits = car_limits
        };

        car.behavior.curr_state = elevator_states[START];  // Start with the idle state (Change to init later)

        // Send initialized data to the computer as priority tasks
        // The update_attr methods schedule a normal task instead of a priority one, so
        // That's why this is done manually here
        uint8_t * weight_p = (uint8_t *) &weight;

        /* TODO: Add an option to the update functions to send them as a normal or priority task 
        You can do this by using schedule_task directly instead of schedule_normal_task */
        // schedule_priority_task(UPDATE_FLOOR, ((uint8_t []){car_index, UPDATE_FLOOR, car.state.floor}), 3);
        // schedule_priority_task(UPDATE_TEMPERATURE, ((uint8_t []){car_index, UPDATE_TEMPERATURE, car.state.temp}), 3);
        // schedule_priority_task(UPDATE_DOOR_STATUS, ((uint8_t []){car_index, car.state.is_door_open}), 2);
        // schedule_priority_task(UPDATE_LIGHT_STATUS, ((uint8_t []){car_index, car.state.is_light_on}), 2);
        // schedule_priority_task(UPDATE_WEIGHT, ((uint8_t []){car_index, weight_p[0], weight_p[1]}), 3);
        // schedule_priority_task(UPDATE_CAPACITY, ((uint8_t []){car_index, car.attrs.riders != NULL}), 2);
        // schedule_priority_task(UPDATE_TEMPERATURE, ((uint8_t []){car_index, car.state.temp}), 2);
        // schedule_priority_task(UPDATE_MOVEMENT_STATE, ((uint8_t []){car_index, car.attrs.move}), 2);
        
        memcpy(car_p, &car, sizeof(elevator_t));
    }
}


// Destructor for an elevator object
void deinit_elevator(elevator_t * car)
{
    free(car->attrs.person_pool[0].item);  // Free up person item array address
    free(car->attrs.person_pool);
    free(car->attrs.pressed_floors);
}


/**Manager elevator functions
 * 
 * These functions are meant to be used by the elevator manager to
 * administer the elevator objects or by other parts of the Arduino
 * code to perform state-specific actions.
*/


/**A person enters an elevator
 * 
 * In here, the information related to a specific person entering the
 * elevator is stored and the elevator's state is updated with this 
 * new information. 
 * 
 * This is done to keep track of the values thatcare changing in the
 * elevator, so when a person "exits" the elevator, the values related
 * to that person are also substracted.
 * 
 * The "attrs" argument is assumed to have the following information
 * in the order that is presented:
 * 
 * - temp (1 byte): The temperature increase in the car due to this
 *      entering the elevator.
 * 
 * - weight (1 byte): The person's weight.
 * 
 * This function assumes the manager requested this floor beforehand
 * and that this function will be called when the elevator reaches
 * said floor.
*/
void enter_elevator(elevator_t * car, uint8_t * attrs)
{
    if (car->state.floor && car->state.floor <= car->limits.floor)
    {
        uint8_t temp = attrs[0];
        uint8_t weight = attrs[1];

        uint8_t floor = car->state.floor - 1;

        if (front_rider_is_not_empty(car, floor))  // If rider is not a placeholder to indicate this floor was requested
        {
            move_to_front(&car->attrs.pressed_floors[floor], &car->attrs.riders);  // Move rider to appropiate floor stack
        }

       person_t * rider = car->attrs.pressed_floors[floor]->item;

        // Update person information
        rider->temp = temp;
        rider->weight = weight;

        // Update elevator's state
        car->state.temp += temp;
        car->state.weight += weight;

        // Send new states to manager
        update_elevator_temp(car);
        update_elevator_weight(car);
        update_elevator_capacity(car);
    }
}


/**People exit the elevator
 * 
 * Given a floor for an elevator, its respective floor list will be
 * checked to see if any person has requested that floor (i.e., if
 * the floor list contains people/not empty.) The information stored
 * in these lists will be substracted from the current state of the
 * elevator.
*/
void exit_elevator(elevator_t * car)
{
    uint8_t floor = car->state.floor - 1;

    for (list_iterator(car->attrs.pressed_floors[floor], person_node))
    {
        person_t * person = person_node->item;

        // Prevent arithmetic overflow of the elevator parameters
        if (car->state.temp - person->temp > car->state.temp || car->state.weight - person->weight > car->state.weight)
        {
            // Reset parameters to readjust internal state
            car->state.temp = 0;
            car->state.weight = 0;

            break;
        }

        // Update parameters in system
        car->state.temp -= person->temp;
        car->state.weight -= person->weight;
    }

    // Remove people from elevator at current floor
    while(car->attrs.pressed_floors[floor] != NULL)
    {
        move_to_front(&car->attrs.riders, &car->attrs.pressed_floors[floor]);
    }

    // Send updated states to manager
    update_elevator_temp(car);
    update_elevator_weight(car);
    update_elevator_capacity(car);
}


/**Move the elevator
 * 
 * This function will move the elevator based on the direction it's
 * currently moving.
*/
void move_elevator(elevator_t * car)
{
    uint8_t movement = car->attrs.move;

    if (movement == UP && car->state.floor < car->limits.floor)
    {
        car->state.floor++;
    }
    else if (movement == DOWN && NULL_FLOOR < car->state.floor - 1)
    {
        car->state.floor--;
    }

    update_elevator_floor(car);
}


// Find the next floor the elevator should go to
uint8_t find_next_floor(elevator_t * car)
{
    uint8_t floor  = find_requested_floors(car);  // Find a requested floor in the current direction

    // Find a requested floor in the opposite direction if no floors were
    // found in the current direction.
    if (!floor)
    {
        uint8_t move = car->attrs.move;

        // This searches the opposite direction
        car->attrs.move = (car->attrs.move == UP)? DOWN: UP;
        floor = find_requested_floors(car);

        car->attrs.move = move;  // Reset back to original direction
    }

    return floor;
}


/**Request the elevator to go a floor
 * 
 * This function assumes the elevator manager checked if the
 * elevator has not reached full capacity. A rider from the 
 * unassigned rider stack will be turned to a NULL rider and
 * placed into that floor it was requested to later find this
 * requested floor.
*/ 
void request_elevator(elevator_t * car, uint8_t * p_floor)
{
    if (*p_floor && *p_floor <= car->limits.floor)
    {
        uint8_t floor = *p_floor - 1;

        // Assign an empty rider if one hasn't been placed already on requested floor
        if (car->attrs.pressed_floors[floor] == NULL)
        {
            person_t * rider = car->attrs.riders->item;

            rider->weight = 0; 

            move_to_front(&car->attrs.pressed_floors[floor], &car->attrs.riders);
        }

        // If no floor is currently requested, then assign the requested
        // floor as the next floor the elevator should go to
        if (!car->attrs.next_floor)
        {
            car->attrs.next_floor = *p_floor;
            car->attrs.move = (car->state.floor > car->attrs.next_floor)? DOWN: UP;
            update_elevator_movement_state(car);
        }
    }
}


/**Elevator object functions
 * 
 * These functions are meant to directly control the states of 
 * the elevator functions and will most likely be called through
 * another device connected to the Arduino.
*/

// Alert the elevator that it has been initialized in computer
void alert_elevator_init(uint8_t car_index, uint8_t * _)
{
    (get_elevator(car_index))->attrs.is_init = true;
}


// Set the light on the elevator to a specific state
void set_light_state(uint8_t car_index, uint8_t * state)
{
    elevator_t * car = get_elevator(car_index);

    car->state.is_light_on = (*state > 0);
    update_elevator_light_status(car);
}


// Set the door of the elevator to a specific state
void set_door_state(uint8_t car_index, uint8_t * state)
{
    elevator_t * car = get_elevator(car_index);

    car->state.is_door_open = (*state > 0);
    update_elevator_door_status(car);
}


// Set the elevator to a specfic floor
void set_floor(uint8_t car_index, uint8_t * floor)
{
    elevator_t * car = get_elevator(car_index);

    if (*floor && *floor <= car->limits.floor)
    {
        car->state.floor = *floor;
        update_elevator_floor(car);
    }
}


// Set the elevator's car to a specific temperature
void set_temperature(uint8_t car_index, uint8_t * temp)
{
    elevator_t * car = get_elevator(car_index);

    car->state.temp = *temp;
    update_elevator_temp(car);
}


// Set the load in the elevator to a specific weight
void set_weight(uint8_t car_index, uint8_t * weight)
{
    elevator_t * car = get_elevator(car_index);

    car->state.weight = *weight;
    update_elevator_weight(car);
}


// Set the elevator's car to a specific temperature
void set_maintanence_state(uint8_t car_index, uint8_t * status)
{
    elevator_t * car = get_elevator(car_index);

    car->attrs.maintenance_needed = *status;
    update_elevator_maintenance_status(car);
}


/* Private elevator functions */

// Initialize miscellaneous attributes for the elevator object
static car_attrs_t init_misc_elevator_attrs(uint8_t floor_count, uint8_t capacity)
{
    car_attrs_t car_attrs = 
    {
        .move = STOP,
        .init_time = 0,
        .is_init = false,
        .pressed_floors = NULL,
        .next_floor = NULL_FLOOR,
        .action_started = false,
        .maintenance_needed = false,
    };

    // Create list nodes to store passendgers for rider floor placement
    car_attrs.person_pool = malloc(sizeof(list_node_t) * capacity);
    car_attrs.riders = car_attrs.person_pool;

    if (car_attrs.person_pool == NULL)
    {
        goto handle_null_elevator;
    }

    // Create passenger stacks for each floor
    car_attrs.pressed_floors = malloc(sizeof(list_node_t *) * floor_count);

    if (car_attrs.pressed_floors == NULL)
    {
        goto handle_null_elevator;
    }

    // Set up floor stacks
    for (uint8_t i = 0; i < floor_count; i++)
    {
        car_attrs.pressed_floors[i] = NULL;
    }

    // Create person pool items to store info in
    person_t * person_array = malloc(sizeof(person_t) * capacity); 

    if (car_attrs.person_pool[0].item == NULL)
    {
        goto handle_null_elevator;
    }

    // Set up person pool stack
    for (uint8_t i = 0; i < floor_count; i++)
    {
        car_attrs.person_pool[i].item = &person_array[i];
        car_attrs.person_pool[i].next = (i < floor_count - 1)? &car_attrs.person_pool[i + 1]: NULL;
    }

    return car_attrs;


handle_null_elevator:
    
    free(car_attrs.person_pool);
    free(car_attrs.pressed_floors);

    return car_attrs;
}


// Finds a requested floor in the current direction
static uint8_t find_requested_floors(elevator_t * car)
{
    switch (car->attrs.move)
    {
        case UP:  // Verify the upper floors for any requests
    
            for (uint8_t floor = car->state.floor; floor <= car->limits.floor; floor++)
            {
                if (car->attrs.pressed_floors[floor - 1] != NULL)
                {
                    return floor;
                }
            }

            break;

        case DOWN:  // Verify the lower floors for any requests

            for (uint8_t floor = car->state.floor; NULL_FLOOR < floor; floor--)
            {
                if (car->attrs.pressed_floors[floor - 1] != NULL)
                {
                    return floor;
                }
            }

            break;
    }

    return NULL_FLOOR;  // Otherwise, no requests were found for the current direction
}