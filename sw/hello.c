#include <stdio.h>

int lock = 0;

/* Core_0 thread */
int main(void)
{
  unsigned long hart_id = 7;
  asm volatile ("csrr %0, mhartid"  : "=r"(hart_id));

  while (lock != 0) {
    // wait
  }

  lock = 1;
  printf("Hello, World! From Hart %d. \n", hart_id);
  lock = 0;


  while (1)
  {

  }

  return 0;
}

/* Core_1 thread */
int __main(void)
{
  unsigned long hart_id = 7;
  asm volatile ("csrr %0, mhartid"  : "=r"(hart_id));

  while (lock != 0) {
    // wait
  }

  lock = 1;
  printf("Hello, World! From Hart %d. \n", hart_id);
  lock = 0;
  
  while (1)
  {

  }

  return 0;
}
