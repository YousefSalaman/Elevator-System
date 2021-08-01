#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>



typedef struct task_queue
{
    uint8_t size;    // Size of queue
    bool is_empty;   // Indicates if queue is empty or not
    uint8_t* next;   // 
    uint8_t** head;

} task_queue_t;


void pop_task(task_queue_t* queue);
task_queue_t init_task_queue(uint8_t size);
void deinit_task_queue(task_queue_t* queue);
bool in_queue(task_queue_t* queue, uint8_t id);
void push_task(task_queue_t* queue, uint8_t id);


#ifdef __cplusplus
}
#endif

#endif