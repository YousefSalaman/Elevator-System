#include <stdlib.h>

#include "serial_pkt.h"

#include "cobs/cobs.h"

/* Private serial constants */

// Possible decoding errors

#define SHORT_PKT_HDR_SIZE 0
#define CRC_CHECKSUM_FAIL 1
#define TASK_NOT_REGISTERED 2
#define INCORRECT_PAYLOAD_SIZE 3

// Possible encoding errors


/* Public serial functions */

// Initialize a packet
serial_pkt_t init_serial_pkt(uint8_t pkt_size)
{
    serial_pkt_t pkt;

    pkt.byte_count = 0;
    pkt.size = pkt_size;
    pkt.buf = malloc(sizeof(uint8_t) * pkt_size);

    return pkt;
}


// Unitialize a packet
void deinit_serial_pkt(serial_pkt_t * pkt)
{
    if (pkt->buf != NULL)
    {
        free(pkt->buf);
    }
}


// Decode and check for packet validity
task_entry_t * check_rx_pkt(task_table_t table, serial_pkt_t * rx_pkt)
{
    static uint8_t out_buf[PKT_BUF_SIZE];

    // Check for minimum header length
    if (rx_pkt->byte_count < SCHEDULER_HDR_OFFSET)
    {
        print_decode_err(SHORT_PKT_HDR_SIZE, NULL);
        return NULL;
    }

    size_t pkt_size = cobs_decode(rx_pkt->buf, rx_pkt->size, out_buf);  // Decode packet and pass to new buffer

    // TODO: Put crc stuff in here

    uint8_t task_id = out_buf[TASK_ID_OFFSET];
    task_entry_t * entry = lookup_task(table, task_id);

    // Check if entry was registered
    if (entry == NULL)
    {
        print_decode_err(TASK_NOT_REGISTERED, &task_id);
        return NULL;
    }

    // Check if stored size matches size of packet
    if (entry->size != NULL)  // If stored size is NULL, then packet length won't be checked
    {
        if (pkt_size != *entry->size + PAYLOAD_OFFSET - 1)
        {
            print_decode_err(INCORRECT_PAYLOAD_SIZE, (uint16_t []){*entry->size + PAYLOAD_OFFSET - 1, pkt_size});
            return NULL;
        }
    }

    return entry;  // If checks passed, return entry
}


// Process incoming information byte-by-byte
bool process_incoming_byte(serial_pkt_t * rx_pkt, uint8_t byte)
{
    if (rx_pkt->byte_count < rx_pkt->size)  // Add element to the rx packet buffer
    {
        rx_pkt->buf[rx_pkt->byte_count++] = byte;
    }
    else  // Reset rx buffer and save byte
    {
        rx_pkt->buf[0] = byte;
        rx_pkt->byte_count = 1;
    }

    return !byte; // If byte = 0, then a packet was found
}


bool process_outgoing_pkt()
{}