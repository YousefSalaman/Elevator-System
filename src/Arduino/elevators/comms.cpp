#include <Arduino.h>

#include <scheduler.h>


static task_scheduler_t scheduler;


// Read from serial port
void receive_serial_pkt()
{
    uint8_t bytes_to_rx = Serial.available();

    while (bytes_to_rx)
    {
        int byte = Serial.read();

        if (byte < 0)
        {
            break;
        }

        if (process_incoming_byte(&scheduler, byte))
        {
            process_rx_scheduler_pkt(&scheduler);  // If a packet is found, then process it entirely
        }
    }
}


// Send data to serial port
void send_serial_pkt(uint8_t id, uint8_t* payload, uint16_t payload_sz)
{}