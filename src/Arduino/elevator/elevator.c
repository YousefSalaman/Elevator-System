#include <string.h>
#include <stdlib.h>

#include "elevator.h"


/* Private elevator constants */

// Elevator capacity states

#define FULL_CAP    0
#define PARTIAL_CAP 1


/* Elevator function prototypes*/

static uint8_t find_requested_floors(elevator_t * car);
static car_attrs_t init_misc_elevator_attrs(uint8_t floor_count, uint8_t capacity);


/* Public system elevator functions */


/**System elevator functions
 * 
 * These functions are meant to be used by the elevator manager to
 * administer the elevator objects.
*/

// Constructor for an elevator object
elevator_t init_elevator(uint8_t floor_count, uint8_t max_temp, uint8_t min_temp, uint8_t capacity, uint16_t weight)
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

    car.behavior.curr_state = IDLE;  // Start with the idle state (Change to init later)

    return car;
}


// Destructor for an elevator object
void deinit_elevator(elevator_t * car)
{
    free(car->attrs.person_pool[0].item);  // Free up person item array address
    free(car->attrs.person_pool);
    free(car->attrs.pressed_floors);
}


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
    uint8_t floor = car->state.floor;

    if (floor && floor < car->limits.floor)
    {
        uint8_t temp = attrs[0];
        uint8_t weight = attrs[1];

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

        // Full capacity has been reached, so alert the elevator manager
        if (car->attrs.riders == NULL)
        {
            schedule_normal_task(UPDATE_CAR_CAPACITY_STATUS, (uint8_t []){FULL_CAP}, 1);
        }
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

    // Tell the elevator manager this elevator is available in terms of capacity
    if (car->attrs.riders != NULL)
    {
        schedule_normal_task(UPDATE_CAR_CAPACITY_STATUS, (uint8_t []){PARTIAL_CAP}, 1);
    }
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
void request_floor(elevator_t * car, uint8_t * floor)
{
    if (*floor && *floor <= car->limits.floor)
    {
        // Assign an empty rider if one hasn't been placed already on requested floor
        if (car->attrs.pressed_floors[*floor - 1] == NULL)
        {
            person_t * rider = &car->attrs.riders->item;

            rider->weight = 0; 

            move_to_front(&car->attrs.pressed_floors[*floor - 1], &car->attrs.riders);
        }

        // If no floor is currently requested, then assign the requested
        // floor as the next floor the elevator should go to
        if (!car->attrs.next_floor)
        {
            car->attrs.next_floor = *floor;
        }
    }
}


/* Private system elevator functions */


// Initialize miscellaneous attributes for the elevator object
static car_attrs_t init_misc_elevator_attrs(uint8_t floor_count, uint8_t capacity)
{
    car_attrs_t car_attrs = 
    {
        .move = STOP,
        .init_time = 0,
        .riders = NULL,
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
    
    free(car_attrs.riders);
    free(car_attrs.pressed_floors);

    return car_attrs;
}


// Finds a requested floor in the current direction
static uint8_t find_requested_floors(elevator_t * car)
{
    switch (car->attrs.move)
    {
        case UP:  // Verify the upper floors for any requests
    
            for (uint8_t floor = car->state.floor; floor < car->limits.floor; floor++)
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


/**Direct elevator functions
 * 
 * These functions are meant to be called only through another device
 * connected to the Arduino. In other words, these functions will be
 * saved in the elevator command table.
*/

// Get the light state for an elevator
uint8_t * get_light_state(elevator_t * car)
{
    return &car->state.is_light_on;
}


// Get the door state for an elevator
uint8_t * get_door_state(elevator_t * car)
{
    return &car->state.is_door_open;
}


// Get which floor the elevator is currently on
uint8_t * get_floor(elevator_t * car)
{
    return &car->state.floor;
}


// Get the temperature in elevator's car
uint8_t * get_temp(elevator_t * car)
{
    return &car->state.temp;
}


// Get the weight of the load in the elevator's car
uint8_t * get_weight(elevator_t * car)
{
    return &car->state.weight;
}


// Set the light on the elevator to a specific state
void set_light_state(elevator_t * car, uint8_t * state)
{
    car->state.is_light_on = (*state > 0);
}


// Set the door of the elevator to a specific state
void set_door_state(elevator_t * car, uint8_t * state)
{
    car->state.is_door_open = (*state > 0);
}


// Set the elevator to a specfic floor
void set_floor(elevator_t * car, uint8_t * floor)
{
    if (*floor && *floor <= car->limits.floor){
        car->state.floor = *floor;
    }
}


// Set the elevator's car to a specific temperature
void set_temperature(elevator_t * car, uint8_t * temp)
{
    car->state.temp = *temp;
}


// Set the load in the elevator to a specific weight
void set_weight(elevator_t * car, uint8_t * weight)
{
    car->state.weight = *weight;
}