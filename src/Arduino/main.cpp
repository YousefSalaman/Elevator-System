#include <Arduino.h>
#include <devices.h>
#include <scheduler.h>

#include "elevator/elevator.h"


/* Function prototypes */

static void receive_serial_pkt(void);
static void serial_tx_cb(uint8_t * pkt, uint8_t pkt_size);
static uint8_t serial_rx_cb(uint8_t id, task_t task, uint8_t * pkt);

/* Main functions */


void setup()
{
    // Initialize serial channel
    Serial.begin(115200);

    // Initialize scheduler
    if (init_task_scheduler(serial_rx_cb, serial_tx_cb, millis))
    {
        init_device_trackers(1);
        register_platform("elevator");

        init_elevators(2);

        // Setup elevator objects
        if (device_initialized(ELEVATOR_TRACKER))  // Check if the elevators where initialized
        {
            // Floor names for the elevators
            const char * floor_names_1[] = {"LOBBY", "1", "2", "3", "4", "5"};
            const char * floor_names_2[] = {"LOBBY", "1", "TR","2", "3", "4", "5"};

            // Setup each elevator individually
            set_elevator_attrs(0, floor_names_1, 6, ELEVATOR_MAX_TEMP, ELEVATOR_MIN_TEMP, ELEVATOR_CAPACITY, ELEVATOR_MAX_WEIGHT);
            set_elevator_attrs(1, floor_names_2, 7, ELEVATOR_MAX_TEMP, ELEVATOR_MIN_TEMP, ELEVATOR_CAPACITY, ELEVATOR_MAX_WEIGHT);

            alert_setup_completion();
        }
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
    int byte;  // It's an int cuz Serial.read outputs an int
    
    // Read incoming bytes and store in scheduler rx pkt buffer
    while ((byte = Serial.read()) >= 0)
    {
        build_rx_task_pkt(byte);
    }
}


// Callback for interpreting and running a task
static uint8_t serial_rx_cb(uint8_t id, task_t task, uint8_t * pkt)
{
    switch (id)
    {
        // Elevator tasks
        case ENTER_ELEVATOR:
        case REQUEST_ELEVATOR:
            uint8_t car_index = pkt[CAR_INDEX_OFFSET];

            if (car_index < ELEVATOR_COUNT)
            {
                ((elevator_task_t) task)(car_index, &pkt[CAR_PAYLOAD_OFFSET]);

                return 0;
            }
            return INVALID_CAR_INDEX;

        default: // Set device attribute task
            ((set_dev_attr_cb) task)(pkt);
            return 0;
    }
}


// Callback for serial communication transmission
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