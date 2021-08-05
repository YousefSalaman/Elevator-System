#include <stdlib.h>

#include "serial_pkt.h"


// Initialize a packet
serial_pkt_t init_serial_pkt(uint8_t pkt_size)
{
    serial_pkt_t pkt;

    pkt.byte_count = 0;
    pkt.size = pkt_size;
    pkt.in_buf = malloc(sizeof(uint8_t) * pkt_size);
    pkt.out_buf = malloc(sizeof(uint8_t) * pkt_size);

    return pkt;
}


// Unitialize a packet
void deinit_serial_pkt(serial_pkt_t * pkt)
{
    if (pkt->in_buf != NULL)
    {
        free(pkt->in_buf);
    }

    if (pkt->out_buf != NULL)
    {
        free(pkt->out_buf);
    }
}