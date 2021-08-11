#include <Arduino.h>

#include <scheduler.h>


static task_scheduler_t scheduler;


// Read from serial port
void receive_serial_pkt(void)
{
    int byte;  
    
    while ((byte = Serial.read()) >= 0)
    {
        if (store_rx_byte(scheduler, byte))  // If a packet is found, then process it entirely
        {
            perform_task(&scheduler);
        }
    }
}


// 
void serial_rx_cb(uint8_t task_id, void * task, uint8_t * pkt)
{}


// Send data to serial port
void send_serial_pkt(uint8_t id, uint8_t* payload, uint16_t payload_sz)
{}