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
  /*
  lock_acquire(&uart_lock);
  printf("C0: Now test use after free !\r\n", sum);
  lock_release(&uart_lock);
  
  
  asm volatile(
                "li   t0,   0x82005000;"         // write pointer
                "li   t1,   0x55555000;"         // data
                "j    .loop_store;");

  asm volatile(
                ".loop_store:"
                "li   a5,   0x82007FFF;"
                "lw         t1,   (t0);"
                "addi t1,   t1,   1;"            // data + 1
                "addi t0,   t0,   4;"            // write address + 4
                "blt  t0,   a5,  .loop_store;");    
  */
  //=================== Post execution ===================//
  ght_set_status (0x02); // ght: stop
  while (ght_get_status() < 0x0F) {

  }

  lock_acquire(&uart_lock);
  printf("All tests are done!\n");
  lock_release(&uart_lock);
  
  // shadow memory
  shadow_free(shadow);
  return 0;
}


/* Core_1 & 2 thread */
int __main(void)
{
  uint64_t Hart_id = 0;
  asm volatile ("csrr %0, mhartid"  : "=r"(Hart_id));
  
  switch (Hart_id){
      case 0x01:
        task_Sanitiser(Hart_id);
      break;

      case 0x02:
        task_Sanitiser(Hart_id);
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
  
  idle();
  return 0;
}