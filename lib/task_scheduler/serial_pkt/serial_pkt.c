#include <stdlib.h>
#include <string.h>

#include "serial_pkt.h"

#include "scheduler.h"
#include "../cobs/cobs.h"

/* Private serial constants */

// Internal decode pseudo-command values

#define EXPECTED_PKT_SIZE 0
#define RECEIVED_PKT_SIZE 1

// Possible decoding errors

#define SHORT_PKT_HDR_SIZE     0
#define CRC_CHECKSUM_FAIL      1
#define TASK_NOT_REGISTERED    2
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
    free(pkt->buf);
}


// Decode and check for packet validity
task_entry_t * process_incoming_pkt(task_table_t table, serial_pkt_t * rx_pkt)
{
    static uint8_t out_buf[PKT_BUF_SIZE];

    // Check for minimum header length
    if (rx_pkt->byte_count < SCHEDULER_HDR_OFFSET)
    {
        print_internal_message(PKT_DECODE, SHORT_PKT_HDR_SIZE); 
        return NULL;
    }

    size_t pkt_size = cobs_decode(rx_pkt->buf, rx_pkt->size, out_buf);  // Decode packet and pass to new buffer

    // TODO: Put crc stuff in here

    // If task is an internal task, skip table lookup
    if (!rx_pkt->buf[TASK_TYPE_OFFSET])
    {
        return NULL;
    }

    uint8_t task_id = out_buf[TASK_ID_OFFSET];
    task_entry_t * entry = lookup_task(table, task_id);

    // Check if entry was registered
    if (entry == NULL)
    {
        print_internal_message(PKT_DECODE, TASK_NOT_REGISTERED); 
        return NULL;
    }

    // Check if stored size matches size of packet
    if (entry->size != NULL)  // If stored size is NULL, then packet length won't be checked
    {
        if (pkt_size != *entry->size + PAYLOAD_OFFSET - 1)
        {
            modify_internal_task_val(PKT_DECODE, RECEIVED_PKT_SIZE, (uint16_t []){pkt_size}, sizeof(uint16_t));
            modify_internal_task_val(PKT_DECODE, EXPECTED_PKT_SIZE, (uint16_t []){*entry->size + PAYLOAD_OFFSET}, sizeof(uint16_t));
            print_internal_message(PKT_DECODE, INCORRECT_PAYLOAD_SIZE);
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


bool pass_outgoing_pkt(serial_pkt_t * tx_pkt, uint8_t task_id, uint8_t task_type, uint8_t * payload_pkt, uint8_t payload_size)
{
    // Check if the task payload is small enough to fit
    if (payload_size + PAYLOAD_OFFSET < tx_pkt->size)
    {
        return false;
    }

    // Pass task attributes to tx_pkt buffer
    tx_pkt->buf[TASK_ID_OFFSET] = task_id;
    tx_pkt->buf[TASK_TYPE_OFFSET] = task_type;
    memcpy(tx_pkt->buf + PAYLOAD_OFFSET, payload_pkt, payload_size);

    tx_pkt->byte_count = PAYLOAD_OFFSET + payload_size;

    return true;
}


bool process_outgoing_pkt(serial_pkt_t * tx_pkt)
{
    static uint8_t in_buf[PKT_BUF_SIZE];

    // Pass tx_pkt buffer into a separate buffer
    memcpy(in_buf, tx_pkt->buf, tx_pkt->byte_count);

    // Put crc stuff in here

    tx_pkt->byte_count = cobs_encode(in_buf, tx_pkt->byte_count, tx_pkt->buf);

    return true;
}