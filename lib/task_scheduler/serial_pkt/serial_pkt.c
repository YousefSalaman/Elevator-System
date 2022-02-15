#include <stdlib.h>
#include <string.h>

#include "serial_pkt.h"

#include "../scheduler.h"
#include "../cobs/cobs.h"

/* Private packet constants constants */

// Internal decode printer task values

#define EXPECTED_PKT_SIZE 0
#define RECEIVED_PKT_SIZE 1
#define CURRENT_TASK_NUM  2

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

    // Check if requested size is less than the allowed pkt size
    if (pkt_size < MAX_ALLOWED_PKT_SIZE)
    {
        pkt.size = pkt_size;
        pkt.buf = malloc(sizeof(uint8_t) * pkt_size);
    }
    else
    {
        pkt.size = 0;
        pkt.buf = NULL;
    }

    return pkt;
}


// Process incoming information byte-by-byte
bool process_incoming_byte(serial_pkt_t * rx_pkt, uint8_t byte)
{
    // If byte is 0, then a packet was found and it will not save the byte
    if (!byte)
    {
        return true;
    }

    // Otherwise, save byte in the rx packet

    if (rx_pkt->byte_count < MAX_ENCODED_PKT_BUF_SIZE)  // Add element to the rx packet buffer
    {
        rx_pkt->buf[rx_pkt->byte_count++] = byte;
    }
    else  // Reset rx buffer and save byte
    {
        rx_pkt->buf[0] = byte;
        rx_pkt->byte_count = 1;
    }

    return false;
}


/**Process a completed rx task packet
 * 
 * Processing an rx packet entails the following:
 * 
 * 1. It will COBS decode the rx packet.
 * 
 * 2. It will verify different attributes of the packet to
 *    see if the packet is valid or not.
 */ 
task_entry_t * process_incoming_pkt(task_table_t table, serial_pkt_t * rx_pkt)
{
    static uint8_t encoded_pkt[MAX_ENCODED_PKT_BUF_SIZE];

    memcpy(encoded_pkt, rx_pkt->buf, rx_pkt->byte_count);  // Pass encoded pkt to inner buffer

    // Check for minimum header length
    if (rx_pkt->byte_count < ENCODED_HDR_SIZE)
    {
        print_internal_message(PKT_DECODE, SHORT_PKT_HDR_SIZE); 
        return NULL;
    }

    rx_pkt->byte_count = cobs_decode(encoded_pkt, rx_pkt->byte_count, rx_pkt->buf);  // Decode packet and pass to rx_pkt

    // crc16 verification to validate decoded packet
    uint16_t crc16_checksum;
    memcpy(&crc16_checksum, rx_pkt->buf + CRC16_OFFSET, 2);

    // TODO: For now, I'm just checking for 0. Change for function that performs crc16 verification
    if (crc16_checksum != 0)
    {
        print_internal_message(PKT_DECODE, CRC_CHECKSUM_FAIL);
        return NULL;
    }

    // If task is an internal task, skip table lookup and pass decoded pkt to rx_pkt
    if (rx_pkt->buf[TASK_TYPE_OFFSET] == INTERNAL_TASK)
    {
        return NULL;
    }

    task_entry_t * entry = lookup_task(table, rx_pkt->buf[TASK_ID_OFFSET]);

    // Check if entry was registered
    if (entry == NULL)
    {
        modify_internal_printer_var(PKT_DECODE, CURRENT_TASK_NUM, PRINT_UINT8_T, rx_pkt->buf + TASK_ID_OFFSET, sizeof(uint8_t));
        print_internal_message(PKT_DECODE, TASK_NOT_REGISTERED); 
        return NULL;
    }

    // Check if stored packet size in the task table 
    // matches size of the current packet
    if (entry->size > 0)  // If stored size is negative, then packet length won't be checked
    {
        if (rx_pkt->byte_count != entry->size + DECODED_HDR_SIZE)
        {
            modify_internal_printer_var(PKT_DECODE, CURRENT_TASK_NUM, PRINT_UINT8_T, rx_pkt->buf + TASK_ID_OFFSET, sizeof(uint8_t));
            modify_internal_printer_var(PKT_DECODE, RECEIVED_PKT_SIZE, PRINT_SIZE_T, &rx_pkt->byte_count, sizeof(size_t));
            modify_internal_printer_var(PKT_DECODE, EXPECTED_PKT_SIZE, PRINT_INT16_T, (int16_t []){entry->size + DECODED_HDR_SIZE}, sizeof(int16_t));
            print_internal_message(PKT_DECODE, INCORRECT_PAYLOAD_SIZE);
            return NULL;
        }
    }

    return entry;  // If checks passed, return entry
}


bool process_outgoing_pkt(serial_pkt_t * tx_pkt, uint8_t task_id, uint8_t task_type, uint8_t * payload_pkt, uint8_t payload_size)
{
    static uint8_t decoded_pkt[MAX_DECODED_PKT_BUF_SIZE];

    // Check if the task payload is small enough to fit
    if (payload_size + DECODED_HDR_SIZE > MAX_DECODED_PKT_BUF_SIZE)
    {
        return false;
    }

    // Pass task attributes to tx_pkt buffer
    decoded_pkt[TASK_ID_OFFSET] = task_id;
    decoded_pkt[TASK_TYPE_OFFSET] = task_type;
    memcpy(decoded_pkt + PAYLOAD_OFFSET, payload_pkt, payload_size);

    // TODO: Put crc16 stuff here for the in_buf (add checksum to header; for now, I'm passing 0)
    memcpy(decoded_pkt + CRC16_OFFSET, (uint16_t []){0}, 2);

    tx_pkt->byte_count = cobs_encode(decoded_pkt, DECODED_HDR_SIZE + payload_size, tx_pkt->buf);

    return true;
}
