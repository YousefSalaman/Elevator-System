#include <stdlib.h>
#include <string.h>

#include "scheduler.h"
#include "cobs/cobs.h"


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
    TASK_NOT_REGISTER_MSG_SIZE = 49,
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

static void print_decode_err(uint8_t err_type, void * args);
static task_entry_t * check_scheduler_rx_pkt(task_scheduler_t * scheduler, serial_pkt_t * rx_pkt);


/* Public scheduler functions */

// Initialize task scheduler
task_scheduler_t init_task_scheduler(uint8_t queue_size, uint16_t table_size, uint16_t pkt_size)
{
    task_scheduler_t scheduler;

    // scheduler.queue = init_task_queue(queue_size);
    scheduler.table = init_task_table(table_size);
    scheduler.rx_pkt = init_serial_pkt(pkt_size);
    // scheduler.tx_pkt = init_tx_serial_pkt(pkt_size);

    return scheduler;
}


// Unitialize task scheduler
void deinit_task_scheduler(task_scheduler_t * scheduler)
{
    // deinit_task_queue(&scheduler->queue);
    deinit_task_table(scheduler->table);
}


// Scheduler a task 
// void schedule_task(task_scheduler_t * scheduler, uint8_t id)
// {
//     if (!in_queue(&scheduler->queue, id))
//     {
//         push_task(&scheduler->queue, id);
//     }
// }


// Process incoming information byte-by-byte
bool process_incoming_byte(task_scheduler_t * scheduler, uint8_t byte)
{
    serial_pkt_t * rx_pkt = &scheduler->rx_pkt;

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


/** Process the stored scheduler rx packet
 * 
 * In here, the packet that was built with process_incoming_byte
 * will be processed in its entirety. 
 * 
 * TODO: Change name to perform_task.
 */ 
void process_scheduler_rx_pkt(task_scheduler_t * scheduler)
{
    serial_pkt_t * rx_pkt = &scheduler->rx_pkt;
    task_entry_t * entry = check_scheduler_rx_pkt(scheduler, rx_pkt);

    // If all checks passed, run rx callback
    if (entry != NULL)
    {
        scheduler->rx_cb(entry->id, entry->task, rx_pkt->out_buf + PAYLOAD_OFFSET);
        // alert_task_completion(entry->id);
    }

    rx_pkt->byte_count = 0;  // Reset rx packet buffer
}


void schedule_task(task_scheduler_t * scheduler, uint8_t * pkt, uint8_t task_id)
{
    // serial_pkt_t * tx_pkt = &scheduler->tx_pkt;

    
}

/* Private scheduler functions */


// Decode and check for packet validity
static task_entry_t * check_scheduler_rx_pkt(task_scheduler_t * scheduler, serial_pkt_t * rx_pkt)
{
    // Check for minimum header length
    if (rx_pkt->byte_count < SCHEDULER_HDR_OFFSET)
    {
        print_decode_err(SHORT_PKT_HDR_SIZE, NULL);
        return NULL;
    }

    size_t pkt_size = cobs_decode(rx_pkt->in_buf, rx_pkt->size, rx_pkt->out_buf);  // Decode packet and pass to new buffer

    // TODO: Put crc stuff in here

    uint8_t task_id = rx_pkt->out_buf[TASK_ID_OFFSET];
    task_entry_t * entry = lookup_task(scheduler->table, task_id);

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


// Send decode errors for computer to print out in its terminal
static void print_decode_err(uint8_t err_type, void * args)
{
    char * err_msg;
    uint8_t msg_length = DECODE_ERR_MSG_SIZE + decode_err_msg_sizes[err_type];

    err_msg = malloc(sizeof(char) * msg_length);  // Allocate enough space for error message

    // Create error message
    strcpy(err_msg, "Decode error - ");
    switch (err_type)
    {
        case SHORT_PKT_HDR_SIZE:
            strcat(err_msg, "packet header was not fully formed\n");
            break;
        
        case CRC_CHECKSUM_FAIL:
            strcat(err_msg, "checksum failed\n");
            break;
        
        case TASK_NOT_REGISTERED:
            strcat(err_msg, "no task has been registered with an ID of ");
            strcat(err_msg, itoa(*(uint8_t *) args, err_msg, 10));
            strcat(err_msg, "\n");
            break;
        
        case INCORRECT_PAYLOAD_SIZE:
            strcat(err_msg, "expected ");
            strcat(err_msg, itoa(((uint16_t *) args)[0], err_msg, 10));
            strcat(err_msg, " bytes, but received ");
            strcat(err_msg, itoa(((uint16_t *) args)[1], err_msg, 10));
            strcat(err_msg, " bytes\n");
            break;
    }

    // TODO: Add the logger function in here

    free(err_msg);
}


