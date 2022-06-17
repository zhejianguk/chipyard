#include <stdint.h>
#include <stdio.h>

extern int uart_lock;
extern char* shadow;

uint64_t task_synthetic ();
uint64_t task_synthetic2 (uint64_t input);
int task_hello (int hart_id);

int task_PerfCounter(uint64_t core_id);
int task_Sanitiser(uint64_t core_id);
int task_ShadowStack (uint64_t core_id);