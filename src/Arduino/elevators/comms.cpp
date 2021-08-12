#include <Arduino.h>

#include <scheduler.h>


// Read from serial port
void receive_serial_pkt(void)
{
    int byte;  
    
    while ((byte = Serial.read()) >= 0)
    {
        if (store_task_rx_byte(byte))  // If a packet is found, then process it entirely
        {
            perform_task();
        }
    }
}


// 
void serial_rx_cb(uint8_t task_id, void * task, uint8_t * pkt)
{

}


// Send data to serial port
void serial_tx_cb(uint8_t* pkt, uint8_t pkt_size)
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