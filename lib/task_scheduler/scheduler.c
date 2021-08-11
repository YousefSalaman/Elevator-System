#include <stdlib.h>
#include <string.h>

#include <Arduino.h>

#include "scheduler.h"


/* Scheduler macro functions */

#define reschedule_normal_task(scheduler) reschedule_queue_task((scheduler)->queues, false)
#define reschedule_priority_task(scheduler) reschedule_queue_task((scheduler)->queues, true)

#define prioritize_task(scheduler) move_to_front(&(scheduler)->queues->priority_head, &(scheduler)->queues->normal_head)

#define check_reply_timer(scheduler, entry) ((entry)->rescheduled)? (scheduler)->timer_cb() - (scheduler)->start_time >= LONG_TIMER: (scheduler)->timer_cb() - (scheduler)->start_time >= SHORT_TIMER


/* Public scheduler functions */

// Initialize task scheduler
task_scheduler_t init_task_scheduler(uint8_t queue_size, uint16_t table_size, rx_schedule_cb rx_cb, tx_schedule_cb tx_cb, timer_schedule_cb timer_cb)
{
    task_scheduler_t scheduler;

    scheduler.rx_cb = rx_cb;
    scheduler.tx_cb = tx_cb;
    scheduler.timer_cb = timer_cb;
    scheduler.rx_pkt = init_serial_pkt(PKT_BUF_SIZE);
    scheduler.table = init_task_table(table_size);
    scheduler.queues = init_scheduling_queues(queue_size, PKT_BUF_SIZE);

    return scheduler;
}


// Unitialize task scheduler
void deinit_task_scheduler(task_scheduler_t * scheduler)
{
    deinit_task_table(scheduler->table);
    deinit_scheduling_queues(scheduler->queues);
}


// Schedule a task for an external device to perform
void schedule_task(task_scheduler_t * scheduler, uint8_t id, uint8_t type, uint8_t * pkt, uint8_t pkt_size, bool is_priority, bool is_fast)
{
    if (!in_queue(scheduler->queues, id))
    {
        // Send a task to free up space in queues
        if (queues_are_full(scheduler->queues))
        {
            if (is_priority_queue_empty(scheduler->queues))
            {
                prioritize_task(scheduler);
            }
            send_task(scheduler);
        }

        // Schedule task depending on the option that was chosen
        if (is_fast)
        {
            push_task_to_front(scheduler->queues, id, type, pkt, pkt_size, true);
            send_task(scheduler);
        }
        else
        {
            push_task(scheduler->queues, id, type, pkt, pkt_size, is_priority);
        }
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
    task_entry_t * entry = process_incoming_pkt(scheduler->table, rx_pkt);

    if (entry != NULL) // If all checks passed, run rx callback (this is the handler for external tasks)
    {
        ret_code = scheduler->rx_cb(entry->id, entry->task, rx_pkt->buf + PAYLOAD_OFFSET);

        (ret_code)? alert_task_failure(entry->id, ret_code): alert_task_success(entry->id);
    }

    else if (get_task_type(rx_pkt) == INTERNAL_TASK)
    {

    }


    rx_pkt->byte_count = 0;  // Reset rx packet buffer
}


void send_task(task_scheduler_t * scheduler)
{
    if (!queues_are_empty(scheduler->queues))
    {
        schedule_queues_t * queues = scheduler->queues;
        queue_entry_t * entry = is_priority_queue_empty(queues)? peek_normal(queues): peek_priority(queues);

        process_outgoing_pkt(&entry->pkt);

        if (!is_priority_queue_empty(scheduler->queues))  // Send priority tasks
        {
            scheduler->tx_cb(entry->pkt.buf, entry->pkt.byte_count);
            pop_priority_task(scheduler->queues);
        }
        else  // Send normal task
        {
            bool reply_time_passed = check_reply_timer(scheduler, entry);

            // Check if the task has been sent already
            if (scheduler->prev_task != *entry->id)
            {
                scheduler->prev_task = *entry->id;
                scheduler->start_time = millis();
                scheduler->tx_cb(entry->pkt.buf, entry->pkt.byte_count);  // Run the serial tx routine
            }

            // Checks for if the allowed reply time has passed
            if (!entry->rescheduled && reply_time_passed)  // First reply window range check
            {
                entry->rescheduled = true;
                reschedule_normal_task(scheduler);
            }
            else if (entry->rescheduled && reply_time_passed)  // Second reply window range check
            {
                pop_normal_task(scheduler->queues);
            }
        }
    }
}
