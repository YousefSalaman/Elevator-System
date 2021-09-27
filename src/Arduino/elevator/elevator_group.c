#include <stdlib.h>
#include <string.h>

#include <scheduler.h>

#include "elevator.h"


/*Elevator group constants */

#define QUEUE_SIZE 5
#define TABLE_SIZE 23

#define TASK_NAME_PAYLOAD 1
#define TASK_NAME_LIMIT   20

#define INVALID_CAR_INDEX 255


// Elevator group object
typedef struct
{
    uint8_t mode;        // Flag for operation mode for the elevator system
    uint8_t count;       // Amount of elevators present
    elevator_t * group;  // Array of elevators

} elevator_group_t;


/* Elevator group global variables */

static elevator_group_t elevators;


/* Elevator group function prototypes */

static void pass_task_name(uint8_t id, char * name);


/* Public elevator group functions */

// Initialize the elevator group for this device
bool init_elevator_subsystem(uint8_t count, rx_schedule_cb rx_cb, tx_schedule_cb tx_cb, timer_schedule_cb timer_cb)
{
    elevators.group = NULL;  // Set the group to a placeholder value

    // Initialize task scheduler and default tasks
    if (init_task_scheduler(rx_cb, tx_cb, timer_cb))
    {
        // Allocate elevators to set up
        elevators.count = count;
        elevators.mode = INVALID_MODE;
        elevators.group = malloc(sizeof(elevator_t) * count);

        // Send the amount of allocated elevators (it's always the requested amount or 0 if it couldn't be allocated)
        uint8_t allocated_car_count = (elevators.group == NULL)? 0: elevators.count;
        schedule_normal_task(PASS_ELEVATOR_COUNT, (uint8_t []){allocated_car_count}, 1);       
    }

    return elevators.group != NULL;
}


// Uninitialize objects related to the elevator subsystem
void deinit_elevator_subsystem(void)
{
    deinit_task_scheduler();

    for (uint8_t i = 0; i < elevators.count; i++)
    {
        deinit_elevator(&elevators.group[i]);
    }

    free(elevators.group);
}


// Get how many elevators are present in the device
uint8_t get_elevator_count(void)
{
    return elevators.count;
}


// Fetch an elevator object from the elevator group
elevator_t * get_elevator(uint8_t index)
{
    return (index < elevators.count)? &elevators.group[index]: NULL;
}


// Get the current mode of operation for the elevator system
uint8_t get_elevator_system_mode(void)
{
    return elevators.mode;
}


// Register a task related to the elevator
void _register_elevator_task(char * name, uint8_t id, uint8_t payload_size, task_t task)
{
    switch(elevators.mode)
    {
        case REGISTER_MODE:
            register_task(id, payload_size, task);
            break;
        
        case CREATION_MODE:
            pass_task_name(id, name);
            break;
    }
}


// Run all the elevators in the device
void run_elevators(void)
{
    for (uint8_t i = 0; i < elevators.count; i++)
    {
        run_elevator(&elevators.group[i]);
    }
}


// Set the elevator system to an operating mode
void set_elevator_system_mode(uint8_t _, uint8_t * mode)
{
    elevators.mode = (*mode > (uint8_t) CREATION_MODE)? INVALID_MODE: *mode;
}


/* Private elevator group functions */

// Helper function to send the task name to the computer
static void pass_task_name(uint8_t id, char * name)
{
    size_t name_len;
    uint8_t name_pkt[TASK_NAME_PAYLOAD + TASK_NAME_LIMIT];

    // Build task name packet
    name_pkt[0] = id; // Pass the task id to the packet
    name_len = strlen(name);
    if (name_len <= TASK_NAME_LIMIT)
    {
        memcpy(&name_pkt[TASK_NAME_PAYLOAD], name, name_len);
    }
    else
    {
        memcpy(&name_pkt[TASK_NAME_PAYLOAD], "task_", 5);
        name_len = 4 + strlen(itoa(id, &name_pkt[TASK_NAME_PAYLOAD + 6], 10));
    }

    schedule_normal_task(PASS_ELEVATOR_TASK_NAME, name_pkt, name_len);
}