#include <stdlib.h>

#include "scheduler.h"
#include "task_queue/queue.h"
#include "task_table/table.h"





typedef void (*rx_schedule_cb)(uint8_t *);



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
    // serial_pkt_t rx_pkt;
    // serial_pkt_t tx_pkt;

} task_scheduler_t;


// Initialize a packet
serial_pkt_t init_serial_pkt(uint16_t pkt_size)
{
    serial_pkt_t pkt;

    pkt.size = pkt_size;
    pkt.buf = malloc(sizeof(uint8_t) * pkt_size);
    pkt.p_buf = pkt.buf;

    return pkt;
}


// Initialize task scheduler
task_scheduler_t init_task_scheduler(uint8_t queue_size, uint16_t table_size)
{
    task_scheduler_t scheduler;

    // scheduler.rx_pkt = init_serial_pkt(pkt_size);
    // scheduler.tx_pkt = init_serial_pkt(pkt_size);
    scheduler.queue = init_task_queue(queue_size);
    scheduler.table = init_task_table(table_size);

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


bool process_incoming_bytes(uint8_t * byte)
{
    static uint8_t buf[PKT_SIZE];


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


