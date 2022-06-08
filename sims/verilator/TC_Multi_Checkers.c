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

  // ld, index: 0x00, func: not care, opcode: 0x03, dp: alu
  ght_cfg_rtable(0x00, 0x08, 0x03, 0x01);
  // st, index: 0x01, func: not care, opcode: 0x03, dp: alu
  ght_cfg_rtable(0x01, 0x08, 0x23, 0x01);

  // se: 00, end_id: 0x02, scheduling: rr, start_id: 0x01
  ght_cfg_stable_checkers (0x00, 0x02, 0x01, 0x01);
  // se: 00, m_inst_type: 0x00 -- ld; index_m: 0x00, 0x01
  ght_cfg_stable_sch (0x00, 0x00, 0x00);

  // se: 01, end_id: 0x04, scheduling: rr, start_id: 0x03
  ght_cfg_stable_checkers (0x01, 0x04, 0x01, 0x03);
  // se: 01, m_inst_type: 0x00 -- ld; 0x01 --st; index_m: 0x00, 0x01
  ght_cfg_stable_sch (0x01, 0x00, 0x00);
  ght_cfg_stable_sch (0x01, 0x01, 0x01);

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
  ght_set_status (0x02); // All: completed
  uint64_t status;
  while ((status= ght_get_status()) < 0x1FFFF) {
    // Check if GHE are released!
  }


  lock_acquire(&uart_lock);
  printf("C0: Finished computation The test result is: %d! \r\n Status = %x!\n", sum, status);
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


