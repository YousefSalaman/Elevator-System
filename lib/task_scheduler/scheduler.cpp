#include <time.h>

#include "scheduler.h"
#include "task_queue/queue.h"
#include "task_table/table.h"



typedef struct 
{
    uint8_t * buf;
    uint8_t * p_buf;
    uint16_t size;

} serial_pkt_t;


typedef struct 
{
    uint8_t prev_task;
    task_queue_t queue;
    task_table_t table;
    serial_pkt_t rx_pkt;
    serial_pkt_t tx_pkt;

} task_scheduler_t;


// Initialize task scheduler
task_scheduler_t init_task_scheduler(uint8_t queue_sz, uint16_t table_sz)
{
    task_scheduler_t scheduler;

    scheduler.queue = init_task_queue(queue_sz);
    scheduler.table = init_task_table(table_sz);

    return scheduler;
}


void deinit_task_scheduler(task_scheduler_t * scheduler)
{
    deinit_task_queue(&scheduler->queue);
    deinit_task_table(scheduler->table);
}


void schedule_task(task_scheduler_t * scheduler, uint8_t id)
{
    if (!in_queue(&scheduler->queue, id))
    {
        push_task(&scheduler->queue, id);
    }
}

// static void receive_serial_pkt()
// {
//     uint8_t bytes_to_rx = Serial.available();

//     while (bytes_to_rx)
//     {
//         int byte = Serial.read();
//         if (!byte)
//         {
            
//         }
//     }
// }


// static void send_serial_pkt(uint8_t id, uint8_t* payload, uint16_t payload_sz){


// }


