#include <stdint.h>
#include <stdio.h>

extern int uart_lock;
extern char* shadow;

int task_PerfCounter(uint64_t core_id);
int task_Sanitiser(uint64_t core_id);