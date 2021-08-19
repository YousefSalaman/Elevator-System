#include <Arduino.h>

#include <scheduler.h>


// Constants

#define QUEUE_SIZE 5
#define TABLE_SIZE 30


// Function prototypes

void receive_serial_pkt(void);
void serial_tx_cb(uint8_t * pkt, uint8_t pkt_size);


void setup() 
{
    // Set serial port
    Serial.begin(9600);

    // Initialize task scheduler
    init_task_scheduler(QUEUE_SIZE, TABLE_SIZE, NULL, serial_tx_cb, millis);

    // Register tasks
}


void loop() 
{
    send_task();
    receive_serial_pkt();
}


// Read from serial port
void receive_serial_pkt(void)
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


// Send data to serial port
void serial_tx_cb(uint8_t * pkt, uint8_t pkt_size)
{
    size_t bytes_to_send = pkt_size;

    while (bytes_to_send)
    {
        bytes_to_send -= Serial.write(pkt, pkt_size);

        // Arithmetic overflow due to sending more bytes than expected
        if (bytes_to_send > pkt_size)
        {
            break;
        }
    }
}