#include <string.h>
#include <stdlib.h>

#include "elevator.h"


/* Private elevator constants */

#define FLOOR_NAME_HEADER 2
#define FLOOR_NAME_LIMIT 10

/* Elevator group global variables */

static elevator_t * elevators;

/* Elevator function prototypes*/

static void create_elevators(void);
static void update_elevator_attrs(uint8_t * pkt);
static uint8_t find_requested_floors(elevator_t * car);
static car_attrs_t init_misc_elevator_attrs(uint8_t floor_count, uint8_t capacity);
static void pass_elevator_names(const char ** floor_names, uint8_t floor_count, uint8_t car_index);
static void deinit_elevators(device_t * dev_elevators, uint8_t count);
static void set_floor(uint8_t car_index, uint8_t * floor);
static void set_weight(uint8_t car_index, uint8_t * weight);
static void set_temperature(uint8_t car_index, uint8_t * temp);
static void set_door_state(uint8_t car_index, uint8_t * state);
static void set_light_state(uint8_t car_index, uint8_t * state);
static void set_maintanence_state(uint8_t car_index, uint8_t * status);



/* Public system elevator functions */

// Initialize elevator subsystem
void init_elevators(uint8_t count)
{
    const char * elevator_attrs[] = {"capacity", "current_floor", "door_state", "emergency_state", "floors", "maintanence_state", "movement", "next_floor", "light_state", "temperature", "weight"};

    // General elevator setup
    register_device_tracker("elevator", ELEVATOR_TRACKER, count, deinit_elevators, update_elevator_attrs);
    add_device_attrs(ELEVATOR_TRACKER, elevator_attrs, 11);
    create_device_instances(ELEVATOR_TRACKER);
    create_elevators();

    // Register elevator tasks
    register_device_task("enter_elevator", ENTER_ELEVATOR, 3, enter_elevator, NORMAL);
    register_device_task("request_elevator", REQUEST_ELEVATOR, 2, request_elevator, NORMAL);
}

// Set up attributes for elevator object
void set_elevator_attrs(uint8_t car_index, const char ** floor_names, uint8_t floor_count, uint8_t max_temp, uint8_t min_temp, uint8_t capacity, uint16_t weight)
{
    elevator_t * car = get_elevator(car_index);

    if (car != NULL)
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
        elevator_t car_copy = 
        {
            .attrs = init_misc_elevator_attrs(floor_count, capacity),
            .state = car_state,
            .behavior = create_fsm(ELEVATOR_STATE_CNT, elevator_states),
            .limits = car_limits
        };

        car_copy.behavior.curr_state = elevator_states[START];  // Start with the idle state (Change to init later)

        memcpy(car, &car_copy, sizeof(elevator_t));

        // Send initialized data to the computer
        update_elevator_temp(car_index, car);
        update_elevator_floor(car_index, car);
        update_elevator_weight(car_index, car);
        update_elevator_capacity(car_index, car);
        update_elevator_next_floor(car_index, car);
        update_elevator_light_status(car_index, car);
        update_elevator_movement_state(car_index, car);
        update_elevator_emergency_status(car_index, car);

        pass_elevator_names(floor_names, floor_count, car_index);
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

void alert_comp_elevator(uint8_t attr_id, uint8_t car_index, uint8_t floor)
{
    schedule_normal_task(attr_id, ((uint8_t []){car_index, floor}), sizeof(uint8_t) * 2);
}

/**A person enters an elevator
 * 
 * In here, the information related to a specific person entering the
 * elevator is stored and the elevator's state is updated with this 
 * new information. 
 * 
 * This is done to keep track of the values that are changing in the
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
void enter_elevator(uint8_t car_index, uint8_t * attrs)
{
    elevator_t * car = get_elevator(car_index);

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

        // Send new states to manager and alert user a person has been added
        update_elevator_temp(car_index, car);
        update_elevator_weight(car_index, car);
        update_elevator_capacity(car_index, car);
        alert_person_addition(car_index, floor);
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
void exit_elevator(elevator_t * car, uint8_t car_index)
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

        alert_person_removal(car_index, car->state.floor);
    }

    // Remove people from elevator at current floor
    while(car->attrs.pressed_floors[floor] != NULL)
    {
        move_to_front(&car->attrs.riders, &car->attrs.pressed_floors[floor]);
    }

    // Send updated states to manager
    update_elevator_temp(car_index, car);
    update_elevator_weight(car_index, car);
    update_elevator_capacity(car_index, car);
}


// Fetch an elevator object from the elevator array
elevator_t * get_elevator(uint8_t index)
{
    device_tracker_t * tracker = get_tracker(ELEVATOR_TRACKER);
    return (index < tracker->count)? get_device(tracker, index): NULL;
}


/**Move the elevator
 * 
 * This function will move the elevator based on the direction it's
 * currently moving.
*/
void move_elevator(elevator_t * car, uint8_t car_index)
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

    update_elevator_floor(car_index, car);
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
void request_elevator(uint8_t car_index, uint8_t * p_floor)
{
    elevator_t * car = get_elevator(car_index);
    schedule_fast_task(130, EXTERNAL_TASK, "requesting elevs", 16);
    if (*p_floor && *p_floor <= car->limits.floor)
    {
        uint8_t floor = *p_floor - 1;

        // Assign an empty rider if one hasn't been placed already on requested floor
        if (car->attrs.pressed_floors[floor] == NULL)
        {
            schedule_fast_task(130, EXTERNAL_TASK, "placing empty rider", 19);
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
            
            schedule_fast_task(130, EXTERNAL_TASK, "assigned next floor", 19);
            update_elevator_next_floor(car_index, car);
            update_elevator_movement_state(car_index, car);
        }
    }
}


// Run all the elevators in the device
void run_elevators(void)
{
    if (is_comp_device_setup_complete(ELEVATOR_TRACKER))
    {
        device_tracker_t * tracker = get_tracker(ELEVATOR_TRACKER);

        for (uint8_t i = 0; i < tracker->count; i++)
        {
            run_fsm(&elevators[i].behavior, &i);
        }
    }
}

// Update elevator attribute in comp
void update_comp_elevator_attr(uint8_t car_index, elevator_t * car, uint8_t attr_id)
{
    uint8_t pkt[6];
    uint8_t pkt_size;

    // Add default packet values
    pkt[0] = ELEVATOR_TRACKER;
    pkt[1] = car_index;
    pkt[2] = attr_id;

    // Add the rest of the packet values
    if (attr_id == WEIGHT)  // For updating the weight attribute
    {
        pkt_size = 6;
        pkt[3] = ATTR_UINT16_T;
        memcpy(pkt + 4, &car->state.weight, sizeof(uint16_t));
    }
    else  // For updating everything else
    {
        pkt_size = 5;
        pkt[3] = ATTR_UINT8_T;

        switch (attr_id)
        {
            case CAPACITY:
                pkt[4] = car->attrs.riders != NULL;
                break;  

            case TEMPERATURE:
                pkt[4] = car->state.temp;
                break;  
            
            case CURRENT_FLOOR:
                pkt[4] = car->state.floor;
                break;  
            
            case DOOR_STATE:
                pkt[4] = car->state.is_door_open;
                break;  
            
            case LIGHT_STATE:
                pkt[4] = car->state.is_light_on;
                break; 

            case MAINTENANCE_STATE:
                pkt[4] = car->attrs.maintenance_needed;
                break;   

            case MOVEMENT:
                pkt[4] = car->attrs.move;
                break;  
            
            case EMERGENCY_STATE:
                pkt[4] = car->behavior.curr_state == car->behavior.states[EMERGENCY];
                break;  
            
            case NEXT_FLOOR:
                pkt[4] = car->attrs.next_floor;
                break;
        }
    }
    
    schedule_fast_task(UPDATE_DEVICE_ATTR_COMP, EXTERNAL_TASK, pkt, pkt_size);

//     if (is_comp_device_setup_complete(ELEVATOR_TRACKER))
//     {
//         schedule_normal_task(UPDATE_DEVICE_ATTR_COMP, pkt, pkt_size);
//     }
//     else
//     {
//         schedule_fast_task(UPDATE_DEVICE_ATTR_COMP, EXTERNAL_TASK, pkt, pkt_size);
//     }
}


/* Private elevator functions */

// Creates the elevator object array
static void create_elevators(void)
{
    device_tracker_t * tracker = get_tracker(ELEVATOR_TRACKER);
    elevators = malloc(sizeof(elevator_t) * tracker->count);

    if (elevators != NULL)
    {
        device_t * dev_elevators = tracker->devices;
        for (uint8_t i = 0; i < tracker->count; i++)
        {
            dev_elevators[i].device = &elevators[i];
        }
    }
}

// Initialize miscellaneous attributes for the elevator object
static car_attrs_t init_misc_elevator_attrs(uint8_t floor_count, uint8_t capacity)
{
    car_attrs_t car_attrs = 
    {
        .move = STOP,
        .init_time = 0,
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


// Pass the floor names to the computer
static void pass_elevator_names(const char ** floor_names, uint8_t floor_count, uint8_t car_index)
{
    size_t name_len;
    char name_pkt[FLOOR_NAME_LIMIT + FLOOR_NAME_HEADER];

    name_pkt[0] = car_index;  // Add the car index to the packet
    for (uint8_t i = 0; i < floor_count; i++)
    {
        name_pkt[1] = i + 1;  // Add floor number to packet

        // Pass floor name to the packet
        name_len = strlen(floor_names[i]);
        if (name_len <= FLOOR_NAME_LIMIT)
        {
            memcpy(&name_pkt[FLOOR_NAME_HEADER], floor_names[i], name_len);
        }
        else
        {
            name_len = strlen(itoa(i, &name_pkt[FLOOR_NAME_HEADER], 10));  // Uses the floor number as the floor name
        }

        schedule_fast_task(PASS_ELEVATOR_FLOOR_NAME, EXTERNAL_TASK, (uint8_t *) name_pkt, FLOOR_NAME_HEADER + name_len);
    }
}


// Set elevator attributes
static void update_elevator_attrs(uint8_t * pkt)
{
    // Get corresponding elevator
    uint8_t car_index = pkt[0];
    uint8_t attr_id = pkt[1];

    // Set correnposding attribute for given elevator
    switch (attr_id)
    {
        case LIGHT_STATE:
            set_light_state(car_index, &pkt[3]);
            break;
        
        case DOOR_STATE:
            set_door_state(car_index, &pkt[3]);
            break;
        
        case CURRENT_FLOOR:
            set_floor(car_index, &pkt[3]);
            break;
        
        case TEMPERATURE:
            set_temperature(car_index, &pkt[3]);
            break;

        case WEIGHT:
            set_weight(car_index, &pkt[3]);
            break;
        
        case MAINTENANCE_STATE:
            set_maintanence_state(car_index, &pkt[3]);
            break;
    }
}


// Uninitialize objects related to the elevator subsystem
static void deinit_elevators(device_t * _, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
    {
        deinit_elevator(&elevators[i]);
    }

    free(elevators);
}


// Set the light on the elevator to a specific state
static void set_light_state(uint8_t car_index, uint8_t * state)
{
    elevator_t * car = get_elevator(car_index);

    car->state.is_light_on = (*state > 0);
    update_elevator_light_status(car_index, car);
}


// Set the door of the elevator to a specific state
static void set_door_state(uint8_t car_index, uint8_t * state)
{
    elevator_t * car = get_elevator(car_index);

    car->state.is_door_open = (*state > 0);
    update_elevator_door_status(car_index, car);
}


// Set the elevator to a specfic floor
static void set_floor(uint8_t car_index, uint8_t * floor)
{
    elevator_t * car = get_elevator(car_index);

    if (*floor && *floor <= car->limits.floor)
    {
        car->state.floor = *floor;
        update_elevator_floor(car_index, car);
    }
}


// Set the elevator's car to a specific temperature
static void set_temperature(uint8_t car_index, uint8_t * temp)
{
    elevator_t * car = get_elevator(car_index);

    car->state.temp = *temp;
    update_elevator_temp(car_index, car);
}


// Set the load in the elevator to a specific weight
static void set_weight(uint8_t car_index, uint8_t * weight)
{
    elevator_t * car = get_elevator(car_index);

    memcpy(&car->state.weight, weight, sizeof(uint16_t));
    update_elevator_weight(car_index, car);
}


// Set the elevator's car to a specific temperature
static void set_maintanence_state(uint8_t car_index, uint8_t * status)
{
    elevator_t * car = get_elevator(car_index);

    car->attrs.maintenance_needed = *status;
    update_elevator_maintenance_status(car_index, car);
}