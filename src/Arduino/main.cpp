#include <Arduino.h>
#include <scheduler.h>

#include "elevator/elevator.h"


/* Constants */

#define ELEVATOR_COUNT 2       // The amount of elevators present in this device

#define INVALID_CAR_INDEX 255  // Return code for serial rx callbacks that signifies a wrong elevator car index was given

// Offsets for payload processing in serial callback

#define CAR_INDEX_OFFSET 0  // Offset for the index of the car
#define PAYLOAD_OFFSET   1  // Offset for the payload data


/* Function prototypes */

static void receive_serial_pkt(void);
static void serial_tx_cb(uint8_t * pkt, uint8_t pkt_size);
static uint8_t serial_rx_cb(uint8_t id, task_t task, uint8_t * pkt);


void setup() 
{
    // Initialize serial channel
    Serial.begin(9600);

    // Initialize elevator subsystem
    init_elevator_subsystem(ELEVATOR_COUNT, serial_rx_cb, serial_tx_cb, millis);

    // Set elevator attributes
    

    // Register elevator tasks
    register_task(ENTER_ELEVATOR, 3, enter_elevator);
    register_task(REQUEST_ELEVATOR, 2, request_elevator);
    register_task(SET_FLOOR, 2, set_floor);
    register_task(SET_WEIGHT, 2, set_weight);
    register_task(SET_DOOR_STATUS, 2, set_door_state);
    register_task(SET_TEMPERATURE, 2, set_temperature);
    register_task(SET_LIGHT_STATUS, 2, set_light_state);
}


void loop() 
{
    send_task();
    receive_serial_pkt();
    run_elevators();
}


// Read from serial port
static void receive_serial_pkt(void)
{
    int byte;  
    
    // Read incoming bytes and store in scheduler rx pkt buffer
    while ((byte = Serial.read()) >= 0)
    {
        if (store_task_rx_byte(byte))  // If a packet is found, then process it entirely
        {
            perform_task();
        }
    }
}


/* Elevator group private functions */

// Callback for interpreting and running a task
static uint8_t serial_rx_cb(uint8_t id, task_t task, uint8_t * pkt)
{
    uint8_t car_index = pkt[CAR_INDEX_OFFSET];

    if (car_index < ELEVATOR_COUNT)
    {
        return ((elevator_task_t) task)(car_index, &pkt[PAYLOAD_OFFSET]);
    }

    return INVALID_CAR_INDEX;
}


// Callback for serial communication
static void serial_tx_cb(uint8_t * pkt, uint8_t pkt_size)
{
    size_t bytes_to_send = pkt_size;

    while (bytes_to_send)
    {
        bytes_to_send -= Serial.write(&pkt[pkt_size - bytes_to_send], bytes_to_send);

        // Arithmetic overflow due to sending more bytes than expected
        if (bytes_to_send > pkt_size)
        {
            break;
        }
    }
}