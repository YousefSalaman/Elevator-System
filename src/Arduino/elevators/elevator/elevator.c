#include <string.h>
#include <stdlib.h>

#include <utils.h>

#include "elevator.h"



#define BIT_8_MAX 254  // Maximum value a 8 bit integer can take


// Elevator function prototypes

static uint8_t find_requested_floors(elevator_t * car);


/* Public system elevator functions */


/**System elevator functions
 * 
 * These functions are meant to be used by the elevator system. In other
 * words, do not bind these directly to the elevator command table and 
 * just call these from the elevator system code.
*/

// Constructor for an elevator object
elevator_t init_elevator(uint8_t * limits)
{
    uint16_t * temp_weight;
    memcpy(temp_weight, limits + 3, 2);

    // Limits for the elevator's car
    car_limits_t car_limits = {
        .floor = limits[0],
        .h_temp = limits[1],
        .l_temp = limits[2],
        .weight = *temp_weight,
        .capacity = limits[5]
    };

    // Set the elevator's initial state
    car_state_t car_state = {
        .temp = ROOM_TEMPERATURE, 
        .floor = NULL_FLOOR, 
        .weight = ZERO_WEIGHT, 
        .headcount = NO_PEOPLE, 
        .is_light_on = LIGHTS_ON,
        .is_door_open =  CLOSE_DOOR
    };

    // Define the additional attributes for the elevator
    car_attrs_t car_attrs = {
        .move = STOP,
        .riders = create_list_array(limits[5]),
        .init_time = 0,
        .next_floor = NULL_FLOOR,
        .action_started = false,
        .pressed_floors = malloc(sizeof(bool)*limits[5]),
        .maintenance_needed = false
    };

    // Set up elevator object
    elevator_t car = {
        .attrs = car_attrs,
        .state = car_state,
        .behavior = create_fsm(ELEVATOR_STATE_CNT),
        .limits = car_limits
    };

    // Set up behavior for the elevator
    if (car.behavior.states != NULL)
    {
        add_state(&car.behavior, &idle, IDLE);
        add_state(&car.behavior, &moving, MOVING);
        add_state(&car.behavior, &emergency, EMERGENCY);
        add_state(&car.behavior, &maintenance, MAINTENANCE);

        car.behavior.curr_state = IDLE;  // Start with the idle state (Change to init later)
    }

    return car;
}


// Destructor for an elevator object
void deinit_elevator(elevator_t * car)
{
    deinit_fsm(&car->behavior);
    erase_list_array(car->attrs.riders, car->limits.floor);

    // Free up array indicating floors that were requested
    if (car->attrs.pressed_floors != NULL)
    {
        for (uint8_t i = 0; i < car->limits.floor; i++)
        {
            free(car->attrs.pressed_floors + i);
        }       
    }
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
 * - floor (1 byte): This contains the floor the person requested.
 * 
 * - temp (1 byte): The temperature increase in the car due to this
 *      entering the elevator.
 * 
 * - weight (2 bytes): The person's weight.
*/
void enter_elevator(elevator_t * car, uint8_t * attrs)
{
    if (car->state.headcount < car->limits.capacity)
    {
        uint16_t weight;
        bool person_was_added = false;
        uint8_t floor = *attrs, temp = attrs[1];

        memcpy(&weight, attrs + 2, 2);  // Pass value of weight to proper type

        // Add person to its respective floor list and update
        // request for that floor
        if (floor && floor <= car->limits.floor)
        {
            person_was_added = add_item(&car->attrs.riders[floor - 1], &(person_t){temp, weight}, sizeof(person_t));
        }

        // Update elevator's state if person was added
        if (person_was_added)
        {
            car->state.headcount++;
            car->state.temp += temp;
            car->state.weight += weight;
            car->attrs.pressed_floors[floor - 1] = true;
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

    // Update elevator's state
    for (list_iterator(car->attrs.riders[floor], person_node))
    {
        person_t * person = person_node->item;
        car->state.headcount -= 1;
        car->state.temp -= person->temp;
        car->state.weight -= person->weight;
    }

    // Remove people from elevator at current floor
    clear_list(&car->attrs.riders[floor]); 
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
        uint8_t opposite_move = (car->attrs.move == UP)? DOWN: UP;

        // This searches the opposite direction
        car->attrs.move = opposite_move;
        floor = find_requested_floors(car);

        car->attrs.move = move;  // Reset back to original direction
    }

    return floor;
}


/* Private system elevator functions */

// Finds a requested floor in the current direction
static uint8_t find_requested_floors(elevator_t * car)
{
    switch (car->attrs.move)
    {
        case UP:  // Verify the upper floors for any requests
    
            for (uint8_t floor = car->state.floor; floor < car->limits.floor; floor++)
            {
                if (car->attrs.pressed_floors[floor - 1])
                {
                    return floor;
                }
            }

        break;

        case DOWN:  // Verify the lower floors for any requests

            for (uint8_t floor = car->state.floor; NULL_FLOOR < floor; floor--)
            {
                if (car->attrs.pressed_floors[floor - 1])
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
    static uint8_t temp_weight[2];

    memcpy(temp_weight, &(car->state.weight), 2);
    return temp_weight;
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
    uint8_t out_max = car->limits.h_temp, out_min = car->limits.l_temp;
    car->state.temp = *temp * (out_max - out_min) / (BIT_8_MAX) + out_min;;
}


// Set the load in the elevator to a specific weight
void set_weight(elevator_t * car, uint8_t * weight)
{
    car->state.weight = bin_to_int(weight, 2);
}


// Request the elevator to go a floor
void request_floor(elevator_t * car, uint8_t * floor)
{
    if (*floor && *floor <= car->limits.floor)
    {
        car->attrs.pressed_floors[*floor - 1] = true;

        // If no floor is currently requested, then assign the requested
        // floor as the next floor the elevator should go to
        if (!car->attrs.next_floor)
        {
            car->attrs.next_floor = *floor;
        }
    }
}