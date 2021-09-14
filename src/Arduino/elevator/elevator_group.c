#include <stdlib.h>
#include <string.h>

#include <scheduler.h>

#include "elevator.h"


#define QUEUE_SIZE 5
#define TABLE_SIZE 23

#define INVALID_CAR_INDEX 255


// Elevator group object
typedef struct
{
    uint8_t count;       // Amount of elevators present
    bool is_create;      // 
    elevator_t * group;  // Array of elevators

} elevator_group_t;


/* Elevator group global variables */

static elevator_group_t elevators;


/* Elevator group function prototypes */




/* Public elevator group functions */

// Initialize the elevator group for this device
bool init_elevator_subsystem(uint8_t count, rx_schedule_cb rx_cb, tx_schedule_cb tx_cb, timer_schedule_cb timer_cb)
{
    elevators.group = NULL;  // Set the group to a placeholder value

    // Initialize task scheduler and default tasks
    if (init_task_scheduler(QUEUE_SIZE, TABLE_SIZE, rx_cb, tx_cb, timer_cb))
    {
        // Allocate elevators to set up
        elevators.count = count;
        elevators.group = malloc(sizeof(elevator_t) * count);

        // Pass amount of elevators to system
        if (elevators.group != NULL)
        {
            schedule_normal_task(PASS_ELEVATOR_COUNT, (uint8_t *){&elevators.count}, 1);       
        }
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


// Register a task related to the elevator
void register_elevator_task(char * name, uint8_t id, uint8_t payload_size, task_t task)
{
    register_task(id, payload_size, task);

    
}


// Run all the elevators in the device
void run_elevators(void)
{
    for (uint8_t i = 0; i < elevators.count; i++)
    {
        run_elevator(&elevators.group[i]);
    }
}