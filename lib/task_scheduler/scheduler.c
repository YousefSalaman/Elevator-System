#include <stdlib.h>
#include <string.h>

#include "scheduler.h"
#include "cobs/cobs.h"
#include "task_queue/queue.h"
#include "task_table/table.h"


/* Scheduler constants */

// Possible decoding errors
enum decode_errs
{
    SHORT_PKT_HDR_SIZE,
    CRC_CHECKSUM_FAIL,
    TASK_NOT_REGISTERED,
    INCORRECT_PAYLOAD_SIZE
};

// Decode msg sizes
enum decode_msg_size
{
    DECODE_ERR_MSG_SIZE = 17,
    SHORT_PKT_MSG_SIZE = 36,
    CRC_FAIL_MSG_SIZE = 15,
    TASK_NOT_REGISTER_MSG_SIZE = 52,
    INCORRECT_PAYLOAD_MSG_SIZE = 52
};


/* Scheduler variables */

static uint8_t decode_err_msg_sizes[] = // Array to access decode message lengths more easily
{
    SHORT_PKT_MSG_SIZE, 
    CRC_FAIL_MSG_SIZE,
    TASK_NOT_REGISTER_MSG_SIZE, 
    INCORRECT_PAYLOAD_MSG_SIZE
};


/* Scheduler function prototypes */

static void print_decode_err(err_t * decode_err);


/* Public scheduler functions */

// Initialize an rx packet
static serial_rx_pkt_t init_rx_serial_pkt(uint16_t pkt_size)
{
    serial_rx_pkt_t pkt;

    pkt.byte_count = 0;
    pkt.size = pkt_size;
    pkt.in_buf = malloc(sizeof(uint8_t) * pkt_size);

    return pkt;
}


// Initialize task scheduler
task_scheduler_t init_task_scheduler(uint8_t queue_size, uint16_t table_size, uint16_t pkt_size)
{
    task_scheduler_t scheduler;

    scheduler.queue = init_task_queue(queue_size);
    scheduler.table = init_task_table(table_size);
    scheduler.rx_pkt = init_rx_serial_pkt(pkt_size);
    // scheduler.tx_pkt = init_tx_serial_pkt(pkt_size);

    return scheduler;
}


// Unitialize task scheduler
void deinit_task_scheduler(task_scheduler_t * scheduler)
{
    deinit_task_queue(&scheduler->queue);
    deinit_task_table(scheduler->table);
}


// Scheduler a task 
void schedule_task(task_scheduler_t * scheduler, uint8_t id)
{
    if (!in_queue(&scheduler->queue, id))
    {
        push_task(&scheduler->queue, id);
    }
}


// Process incoming information byte-by-byte
bool process_incoming_byte(task_scheduler_t * scheduler, uint8_t byte)
{
    serial_rx_pkt_t * rx_pkt = &scheduler->rx_pkt;

    if (rx_pkt->byte_count < rx_pkt->size)  // Add element to the rx packet buffer
    {
        rx_pkt->in_buf[rx_pkt->byte_count++] = byte;
    }
    else  // Reset rx buffer and save byte
    {
        rx_pkt->in_buf[0] = byte;
        rx_pkt->byte_count = 1;
    }

    return !byte; // If byte = 0, then a packet was found
}


/** Process the stored rx scheduler packet
 * 
 * TODO: Need to add crc stuff
 */ 
void process_rx_scheduler_pkt(task_scheduler_t * scheduler)
{
    serial_rx_pkt_t * rx_pkt = &scheduler->rx_pkt;

    // Check for minimum header length

    if (rx_pkt->byte_count < SCHEDULER_HDR_OFFSET)
    {
        scheduler->err.type = SHORT_PKT_HDR_SIZE;
        goto handle_invalid_pkt;
    }

    size_t pkt_size = cobs_decode(rx_pkt->in_buf, rx_pkt->size, rx_pkt->out_buf);  // Decode packet and pass to new buffer

    // TODO: Put crc stuff in here

    // Check if task exists and if the length matches

    uint8_t task_id = rx_pkt->out_buf[TASK_ID_OFFSET];
    task_entry_t * entry = lookup_task(scheduler->table, task_id);

    if (entry == NULL)  // Entry was not registered
    {
        scheduler->err.args = &task_id;
        scheduler->err.type = TASK_NOT_REGISTERED;
        goto handle_invalid_pkt;
    }

    if (entry->size != NULL)  // If stored size is NULL, then size won't be checked
    {
        if (pkt_size != *entry->size + PAYLOAD_OFFSET - 1)  // Check if stored size matches size of packet
        {
            scheduler->err.args = (uint16_t []){*entry->size + PAYLOAD_OFFSET - 1, pkt_size};
            scheduler->err.type = INCORRECT_PAYLOAD_SIZE;
            goto handle_invalid_pkt;
        }
    }

    // If all checks passed, run rx callback and reset rx packet buffer

    scheduler->rx_cb(task_id, entry->task, rx_pkt->out_buf + PAYLOAD_OFFSET);
    rx_pkt->byte_count = 0;


    // If packet is invalid, then skip other checks, reset buffer,
    // and send packet decoding error
    handle_invalid_pkt:

        rx_pkt->byte_count = 0;  
        print_decode_err(&scheduler->err);
}


/* Private scheduler functions */

// Send decode errors for computer to print out in its terminal
static void print_decode_err(err_t * decode_err)
{  
    uint8_t msg_length = DECODE_ERR_MSG_SIZE + decode_err_msg_sizes[decode_err->type];

    decode_err->msg = malloc(sizeof(char) * msg_length);  // Allocate enough space for error message

    // Create error message
    strcpy(decode_err->msg, "Decode error - ");
    switch (decode_err->type)
    {
        case SHORT_PKT_HDR_SIZE:
            strcat(decode_err->msg, "packet header was not fully formed\n");
            break;
        
        case CRC_CHECKSUM_FAIL:
            strcat(decode_err->msg, "checksum failed\n");
            break;
        
        case TASK_NOT_REGISTERED:
            strcat(decode_err->msg, "no tasks have been registered with an ID of ");
            strcat(decode_err->msg, itoa(*(uint8_t *) decode_err->args, decode_err->msg, 10));
            strcat(decode_err->msg, "\n");
            break;
        
        case INCORRECT_PAYLOAD_SIZE:
            strcat(decode_err->msg, "expected ");
            strcat(decode_err->msg, itoa(((char *)decode_err->args)[0], decode_err->msg, 10));
            strcat(decode_err->msg, " bytes, but received ");
            strcat(decode_err->msg, itoa(((char *)decode_err->args)[1], decode_err->msg, 10));
            strcat(decode_err->msg, " bytes\n");
            break;
    }

    // TODO: Add the logger function in here

    // Reset error object

    free(decode_err->msg);
    decode_err->args = NULL;
}


