#ifndef SERIAL_PKT_T
#define SERIAL_PKT_T

#include <stdint.h>


typedef struct 
{
    uint16_t size;
    uint8_t * in_buf;
    uint8_t * out_buf;
    uint16_t byte_count;

} serial_pkt_t;


/* Serial packet methods */

void deinit_serial_pkt(serial_pkt_t * pkt);
serial_pkt_t init_serial_pkt(uint8_t pkt_size);

#endif