#include <stdlib.h>
#include <string.h>

#include "scheduler.h"
#include "task_queue/queue.h"
#include "task_table/table.h"


/** Scheduler object
 * 
 */
typedef struct 
{
    // Scheduler rx attributes
    serial_pkt_t rx_pkt;
    rx_schedule_cb rx_cb;

    // Scheduler tx attributes
    tx_schedule_cb tx_cb;

    // Task scheduling attributes
    uint8_t prev_task;
    task_table_t table;
    unsigned long start_time;
    schedule_queues_t * queues;
    timer_schedule_cb timer_cb;

} task_scheduler_t;


static task_scheduler_t scheduler;


/* Private scheduler macro functions */

#define reschedule_normal_task() reschedule_queue_task(scheduler.queues, false)
#define reschedule_priority_task() reschedule_queue_task(scheduler.queues, true)

#define prioritize_task() move_to_front(&scheduler.queues->priority_head, &scheduler.queues->normal_head)

#define check_reply_timer(entry) ((entry)->rescheduled)? scheduler.timer_cb() - scheduler.start_time >= LONG_TIMER: scheduler.timer_cb() - scheduler.start_time >= SHORT_TIMER


/* Scheduler function prototypes */

static void process_current_task(void);


/* Public scheduler functions */

// Initialize task scheduler
void init_task_scheduler(uint8_t queue_size, uint16_t table_size, rx_schedule_cb rx_cb, tx_schedule_cb tx_cb, timer_schedule_cb timer_cb)
{
    scheduler.rx_cb = rx_cb;
    scheduler.tx_cb = tx_cb;
    scheduler.timer_cb = timer_cb;
    scheduler.table = init_task_table(table_size);
    scheduler.rx_pkt = init_serial_pkt(PKT_BUF_SIZE);
    scheduler.queues = init_scheduling_queues(queue_size, PKT_BUF_SIZE);
}


// Unitialize task scheduler
void deinit_task_scheduler(void)
{
    deinit_task_table(scheduler.table);
    deinit_scheduling_queues(scheduler.queues);
}


// Register a task in the scheduler
void register_task(uint8_t id, int payload_size, void * task)
{
    register_task_in_table(&scheduler.table, id, payload_size, task);
}


// Form and detect the task rx packet reading byte-by-byte
bool store_task_rx_byte(uint8_t byte)
{
    return process_incoming_byte(&scheduler.rx_pkt, byte);
}


// Schedule a task for an external device to perform
void schedule_task(uint8_t id, uint8_t type, uint8_t * pkt, uint8_t pkt_size, bool is_priority, bool is_fast)
{
    if (!in_queue(scheduler.queues, id))
    {
        // Send a task to free up space in queues
        if (queues_are_full(scheduler.queues))
        {
            if (is_priority_queue_empty(scheduler.queues))
            {
                prioritize_task();
            }
            send_task();
        }

        // Schedule task depending on the option that was chosen
        if (is_fast)
        {
            push_task_to_front(scheduler.queues, id, type, pkt, pkt_size, true);
        }
        else
        {
            push_task(scheduler.queues, id, type, pkt, pkt_size, is_priority);
        }
        send_task();
    }
}


/** Process the stored scheduler rx packet
 * 
 * In here, the packet that was built with process_incoming_byte
 * will be processed in its entirety. 
 */ 
void perform_task(void)
{
    uint8_t ret_code;
    serial_pkt_t * rx_pkt = &scheduler.rx_pkt;
    task_entry_t * entry = process_incoming_pkt(scheduler.table, rx_pkt);

    if (entry != NULL) // If all checks passed, run rx callback (this is the handler for external tasks)
    {
        ret_code = scheduler.rx_cb(entry->id, entry->task, rx_pkt->buf + PAYLOAD_OFFSET);

        alert_task_completion(entry->id, ret_code);
    }

    else if (get_task_type(rx_pkt) == INTERNAL_TASK && get_task_id(rx_pkt) == UNSCHEDULE_TASK)
    {
        process_current_task();
    }

    rx_pkt->byte_count = 0;  // Reset rx packet buffer
}


// Send a task to be performed by another scheduling system connected to this one
void send_task(void)
{
    if (!queues_are_empty(scheduler.queues))
    {
        schedule_queues_t * queues = scheduler.queues;
        queue_entry_t * entry = is_priority_queue_empty(queues)? peek_normal(queues): peek_priority(queues);

        process_outgoing_pkt(&entry->pkt);

        if (!is_priority_queue_empty(queues))  // Send priority tasks
        {
            scheduler.tx_cb(entry->pkt.buf, entry->pkt.byte_count);
            pop_priority_task(scheduler.queues);
        }
        else  // Send normal task
        {
            bool reply_time_passed = check_reply_timer(entry);

            // Check if the task has been sent already
            if (scheduler.prev_task != *entry->id)
            {
                scheduler.prev_task = *entry->id;
                scheduler.start_time = scheduler.timer_cb();
                scheduler.tx_cb(entry->pkt.buf, entry->pkt.byte_count);  // Run the serial tx routine
            }

            // Checks if the allowed reply time has passed
            if (!entry->rescheduled && reply_time_passed)  // First reply window range check
            {
                entry->rescheduled = true;
                reschedule_normal_task();
            }
            else if (entry->rescheduled && reply_time_passed)  // Second reply window range check
            {
                pop_normal_task(scheduler.queues);
            }
        }
    }
}


/* Internal scheduler commands */

//Modify a task value
void send_task_val(uint8_t task_id, uint8_t task_type, uint8_t value_id, void * value, size_t size)
{
    uint8_t var[size + 1];

    var[0] = value_id;
    memcpy(var + 1, value, size);
    schedule_fast_task(task_id, task_type, var, size);
}


/**Decides what action for the task to take based on the other system's reply
 * 
 * This function is recognized as one of the internal commands of the
 * scheduling system. If the return code in the packet is non-zero, then
 * the system will reschedule the task. Otherwise, it will be taken out
 * of the normal queue.
 */ 
static void process_current_task(void)
{
    queue_entry_t * entry = peek_normal(scheduler.queues);

    if (get_task_id(&scheduler.rx_pkt) == *entry->id)
    {
        if (scheduler.rx_pkt.buf[RET_CODE_OFFSET] && !entry->rescheduled)
        {
            entry->rescheduled = true;
            reschedule_normal_task();
        }
        else
        {
            pop_normal_task(scheduler.queues);
        }
    }
}