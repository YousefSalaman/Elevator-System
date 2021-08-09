#include <stdlib.h>
#include <string.h>

#include "scheduler.h"
#include "cobs/cobs.h"


/* Scheduler constants */


/* Scheduler function prototypes */

static void print_decode_err(uint8_t err_type, void * args);
static task_entry_t * check_scheduler_rx_pkt(task_scheduler_t * scheduler, serial_pkt_t * rx_pkt);


/* Public scheduler functions */

// Initialize task scheduler
task_scheduler_t init_task_scheduler(uint8_t queue_size, uint16_t table_size, uint16_t pkt_size)
{
    task_scheduler_t scheduler;

    scheduler.rx_pkt = init_serial_pkt(pkt_size);
    scheduler.table = init_task_table(table_size);
    scheduler.queues = init_scheduling_queues(queue_size, pkt_size);

    return scheduler;
}


// Unitialize task scheduler
void deinit_task_scheduler(task_scheduler_t * scheduler)
{
    deinit_task_table(scheduler->table);
    deinit_scheduling_queues(scheduler->queues);
}


// Schedule a task for an external device to perform
void schedule_task(task_scheduler_t * scheduler, uint8_t id, uint8_t * pkt, uint8_t pkt_size, bool is_priority)
{
    if (!in_queue(scheduler->queues, id))
    {
        push_task(scheduler->queues, id, pkt, pkt_size, is_priority);
    }
}


/** Process the stored scheduler rx packet
 * 
 * In here, the packet that was built with process_incoming_byte
 * will be processed in its entirety. 
 */ 
void perform_task(task_scheduler_t * scheduler)
{
    uint8_t ret_code;
    serial_pkt_t * rx_pkt = &scheduler->rx_pkt;
    task_entry_t * entry = check_rx_pkt(scheduler->table, rx_pkt);

    // If all checks passed, run rx callback
    if (entry != NULL)
    {
        ret_code = scheduler->rx_cb(entry->id, entry->task, rx_pkt->buf + PAYLOAD_OFFSET);

        (ret_code)? alert_task_failure(entry->id): alert_task_success(entry->id);
    }

    rx_pkt->byte_count = 0;  // Reset rx packet buffer
}



// Send decode errors for computer to print out in its terminal
// static void print_decode_err(uint8_t err_type, void * args)
// {
//     char * err_msg;
//     uint8_t msg_length = DECODE_ERR_MSG_SIZE + decode_err_msg_sizes[err_type];

//     err_msg = malloc(sizeof(char) * msg_length);  // Allocate enough space for error message

//     // Create error message
//     strcpy(err_msg, "Decode error - ");
//     switch (err_type)
//     {
//         case SHORT_PKT_HDR_SIZE:
//             strcat(err_msg, "packet header was not fully formed\n");
//             break;
        
//         case CRC_CHECKSUM_FAIL:
//             strcat(err_msg, "checksum failed\n");
//             break;
        
//         case TASK_NOT_REGISTERED:
//             strcat(err_msg, "no task has been registered with an ID of ");
//             strcat(err_msg, itoa(*(uint8_t *) args, err_msg, 10));
//             strcat(err_msg, "\n");
//             break;
        
//         case INCORRECT_PAYLOAD_SIZE:
//             strcat(err_msg, "expected ");
//             strcat(err_msg, itoa(((uint16_t *) args)[0], err_msg, 10));
//             strcat(err_msg, " bytes, but received ");
//             strcat(err_msg, itoa(((uint16_t *) args)[1], err_msg, 10));
//             strcat(err_msg, " bytes\n");
//             break;
//     }

//     // TODO: Add the logger function in here

//     free(err_msg);
// }


