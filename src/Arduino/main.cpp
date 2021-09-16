#include <Arduino.h>
#include <scheduler.h>

#include "elevator/elevator.h"


/* Constants */

#define ELEVATOR_COUNT 2       // The amount of elevators present in this device

#define INVALID_CAR_INDEX 255  // Return code for serial rx callbacks that signifies an invalid elevator car index was given

// Offsets for payload processing in serial callback

#define CAR_INDEX_OFFSET 0  // Offset for the index of the car
#define PAYLOAD_OFFSET   1  // Offset for the payload data


// Floor names for the elevators

char * floor_names_1[] = {"LOBBY", "1", "2", "3", "4", "5"};
char * floor_names_2[] = {"LOBBY", "1", "TR","2", "3", "4", "5"};


/* Function prototypes */

static void receive_serial_pkt(void);
static void serial_tx_cb(uint8_t * pkt, uint8_t pkt_size);
static uint8_t serial_rx_cb(uint8_t id, task_t task, uint8_t * pkt);

/* Main functions */

void setup() 
{
    // Initialize serial channel
    Serial.begin(9600);

    // Initialize elevator subsystem
    if (init_elevator_subsystem(ELEVATOR_COUNT, serial_rx_cb, serial_tx_cb, millis))
    {
        uint8_t mode;

        // Wait until user has chosen an operation mode
        while(!(mode = get_elevator_system_mode()))
        {
            receive_serial_pkt();
        }

        // Set elevator attributes
        if (mode == REGISTER_MODE)
        {
            set_elevator_attrs(0, floor_names_1, 6, 120, 60, 10, 2000);
            set_elevator_attrs(1, floor_names_2, 7, 120, 60, 10, 2000);
        }

        // Register elevator tasks
        register_elevator_task("enter_elevator", ENTER_ELEVATOR, 3, enter_elevator);
        register_elevator_task("request_elevator", REQUEST_ELEVATOR, 2, request_elevator);
        register_elevator_task("set_floor", SET_FLOOR, 2, set_floor);
        register_elevator_task("set_weight", SET_WEIGHT, 2, set_weight);
        register_elevator_task("set_door_state", SET_DOOR_STATUS, 2, set_door_state);
        register_elevator_task("set_temperature", SET_TEMPERATURE, 2, set_temperature);
        register_elevator_task("set_light_state", SET_LIGHT_STATUS, 2, set_light_state);
        register_elevator_task("set_system_mode", SET_ELEVATOR_SYSTEM_MODE, 2, set_elevator_system_mode);
    }
}


void loop() 
{
    send_task();
    receive_serial_pkt();
    run_elevators();
}


/* Private functions */

// Read from serial port
static void receive_serial_pkt(void)
{
    int byte;  
    
    // Read incoming bytes and store in scheduler rx pkt buffer
    while ((byte = Serial.read()) >= 0)
    {
        if (build_rx_task_pkt(byte))  // If a packet is found, then process it entirely
        {
            perform_task();
        }
    }
}


// Callback for interpreting and running a task
static uint8_t serial_rx_cb(uint8_t id, task_t task, uint8_t * pkt)
{
    uint8_t car_index = pkt[CAR_INDEX_OFFSET];

    if (car_index < ELEVATOR_COUNT)
    {
        ((elevator_task_t) task)(car_index, &pkt[PAYLOAD_OFFSET]);

        return 0;
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