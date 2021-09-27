#ifndef SERIAL_PKT_T
#define SERIAL_PKT_T

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "../task_table/table.h"
#include "../scheduler_config.h"

// /* Public serial constants */

// #define PKT_BUF_SIZE 30  // Move to config file

// // COBS encoded pkt offsets

// #define SCHEDULER_HDR_OFFSET 4

// // COBS decoded pkt offsets

// #define CRC_CHECKSUM_OFFSET 1
// #define TASK_ID_OFFSET      3
// #define TASK_TYPE_OFFSET    4
// #define PAYLOAD_OFFSET      5


/* Serial communication objects */

typedef struct 
{
    uint8_t size;
    uint8_t * buf;
    size_t byte_count;

} serial_pkt_t;


/* Serial packet methods */

void deinit_serial_pkt(serial_pkt_t * pkt);
serial_pkt_t init_serial_pkt(uint8_t pkt_size);

bool process_incoming_byte(serial_pkt_t * rx_pkt, uint8_t byte);
task_entry_t * process_incoming_pkt(task_table_t table, serial_pkt_t * rx_pkt);
bool process_outgoing_pkt(serial_pkt_t * tx_pkt, uint8_t task_id, uint8_t task_type, uint8_t * payload_pkt, uint8_t payload_size);

#define get_task_id(pkt) (pkt)->buf[TASK_ID_OFFSET]
#define get_task_type(pkt) (pkt)->buf[TASK_TYPE_OFFSET]

// Unitialize a packet
#define deinit_serial_pkt(pkt) free((pkt)->buf)

#endif