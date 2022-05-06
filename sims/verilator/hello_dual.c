#include <stdio.h>


int uart_lock = 0;
int *uart_lock_p = &uart_lock;

void lock_acquire(int *lock);
void lock_release (int *lock);

/* Core_0 thread */
int main(void)
{
  unsigned long hart_id;
  asm volatile ("csrr %0, mhartid"  : "=r"(hart_id));

  lock_acquire(uart_lock_p);
  printf("Hello, World! From Hart %d. \n", hart_id);
  lock_release(uart_lock_p);

  while (1)
  {

  }

  return 0;
}

/* Core_1 thread */
int __main(void)
{
  unsigned long hart_id;
  asm volatile ("csrr %0, mhartid"  : "=r"(hart_id));

  lock_acquire(uart_lock_p);
  printf("Hello, World! From Hart %d. \n", hart_id);
  lock_release(uart_lock_p);

  while (1)
  {

  }
  return 0;
}



void lock_acquire(int *lock)
{
	int temp0 = 1;

	__asm__(
		"loop%=: "
		"amoswap.w.aq %1, %1, (%0);"
		"bnez %1,loop%="
		://no output register
		:"r" (lock), "r" (temp0)
		:/*no clobbered registers*/
	);
}

void lock_release (int *lock)
{
	__asm__(
		"amoswap.w.rl x0, x0, (%0);"// Release lock by storing 0.
		://no output
		:"r" (lock)
		://no clobbered register
	);
}