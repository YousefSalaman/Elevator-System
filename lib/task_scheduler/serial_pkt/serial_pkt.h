#ifndef SERIAL_PKT_T
#define SERIAL_PKT_T

#include <stdint.h>
#include <stdbool.h>

#include "task_table/table.h"

/* Public serial constants */

#define PKT_BUF_SIZE 30  // Move to config file
// Pre COBS decoding offsets

#define SCHEDULER_HDR_OFFSET 4

// Post COBS decoding offsets

#define TASK_ID_OFFSET 1
#define CRC_CHECKSUM_OFFSET 2
#define PAYLOAD_OFFSET 4


typedef struct 
{
    uint8_t size;
    uint8_t * buf;
    uint16_t byte_count;

} serial_pkt_t;


/* Serial packet methods */

void deinit_serial_pkt(serial_pkt_t * pkt);
serial_pkt_t init_serial_pkt(uint8_t pkt_size);
bool process_incoming_byte(serial_pkt_t * rx_pkt, uint8_t byte);
task_entry_t * check_rx_pkt(task_table_t table, serial_pkt_t * rx_pkt);

#endif