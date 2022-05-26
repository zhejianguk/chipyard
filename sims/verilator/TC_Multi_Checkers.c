#include <stdio.h>
#include <stdlib.h>
#include "rocc.h"
#include "spin_lock.h"
#include "ght.h"
#include "ghe.h"
#include "malloc.h"
#include "tasks.h"

int uart_lock;
char* shadow;


/* Core_0 thread */
int main(void)
{
  int *ptr = NULL;
  int ptr_size = 128;
  int sum = 0;

  //================== Initialisation ==================//
  // shadow memory
  shadow = shadow_malloc(32*1024*1024*sizeof(char));
  if(shadow == NULL) {
    printf("C0: Error! memory not allocated.");
    exit(0);
  }

  lock_acquire(&uart_lock);
  printf("C0: Test is now start!\r\n");
  lock_release(&uart_lock);
  ght_set_status (0x01); // start monitoring


  //===================== Execution =====================//
  ptr = (int*) malloc(ptr_size * sizeof(int));
 
  // if memory cannot be allocated
  if(ptr == NULL) {
    printf("C0: Error! memory not allocated.");
    exit(0);
  }

  for (int i = 0; i < ptr_size; i++)
  {
    *(ptr + i) = i;
  }

  for (int i = 0; i < ptr_size; i++)
  {
    sum = sum + *(ptr+i);
  }

  lock_acquire(&uart_lock);
  printf("C0: Sum = %d \r\n", sum);
  lock_release(&uart_lock);
  free(ptr);


  // Use after free
  *(ptr) = sum;
  sum = sum + *(ptr+8);


  //=================== Post execution ===================//
  ght_set_status (0x00); // ght: pause

  while (ght_get_status() < 0xFFFF) {
    //drain_checkers();
  }	
  ght_set_status (0x02); // All: completed
  
  
  // shadow memory
  shadow_free(shadow);
  while (ght_get_status() < 0x1FFFF) {
    // Check if GHE are released!
  }

  lock_acquire(&uart_lock);
  printf("C0: Finished computation The test result is: %d!\n", sum);
  lock_release(&uart_lock);

  return 0;
}


/* Core_1 & 2 thread */
int __main(void)
{
  uint64_t Hart_id = 0;
  asm volatile ("csrr %0, mhartid"  : "=r"(Hart_id));
  
  switch (Hart_id){
      case 0x01:
        task_PerfCounter(Hart_id);
      break;

      case 0x02:
        task_PerfCounter(Hart_id);
      break;

      case 0x03:
        task_Sanitiser(Hart_id);
      break;

      case 0x04:
        task_Sanitiser(Hart_id);
      break;

      default:
      break;
  }
  
  
  return 0;
}


