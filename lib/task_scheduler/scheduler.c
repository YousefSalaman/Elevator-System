#include <stdlib.h>

#include "scheduler.h"
#include "task_queue/queue.h"
#include "task_table/table.h"



// Initialize a packet
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


bool process_incoming_byte(task_scheduler_t * scheduler, uint8_t byte)
{
    serial_rx_pkt_t * rx_pkt = &scheduler->rx_pkt;

    if (rx_pkt->byte_count < rx_pkt->size)
    {
        rx_pkt->in_buf[rx_pkt->byte_count++] = byte;  // Add element to the rx packet buffer
    }
    else
    {
        // Reset rx buffer and save byte
        rx_pkt->in_buf[0] = byte;
        rx_pkt->byte_count = 1;
    }

    return !byte; // If byte = 0, then a packet was found
}


void process_rx_scheduler_pkt(task_scheduler_t * scheduler)
{
    // Check for minimum packet length
}


