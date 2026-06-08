#ifndef TASK_H
#define TASK_H
#include <stdint.h>
 //the PCB, this tracks a single running program.
typedef struct task{
    uint32_t id; // the process ID
    uint32_t esp; //where the task's stack is currently is
    struct task* next; // pointer to the next program in line
}task_t ;
void task_init(void);
void create_task(void (*entry_point)());
void tasking_init();
void switch_task();
#endif

