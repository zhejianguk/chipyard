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

  // Insepct load operations 
  // index: 0x01 
  // Func: 0x00; 0x01; 0x02; 0x03; 0x04; 0x05
  // Opcode: 0x03
  // Data path: ALU two cycles before
  ght_cfg_filter(0x01, 0x00, 0x03, 0x02); // lb
  ght_cfg_filter(0x01, 0x01, 0x03, 0x02); // lh
  ght_cfg_filter(0x01, 0x02, 0x03, 0x02); // lw
  ght_cfg_filter(0x01, 0x03, 0x03, 0x02); // ld
  ght_cfg_filter(0x01, 0x04, 0x03, 0x02); // lbu
  ght_cfg_filter(0x01, 0x05, 0x03, 0x02); // lhu

  // Insepct store operations 
  // index: 0x01 
  // Func: 0x00; 0x01; 0x02; 0x03
  // Opcode: 0x23
  // Data path: ALU two cycles before
  ght_cfg_filter(0x02, 0x00, 0x23, 0x02); // sb
  ght_cfg_filter(0x02, 0x01, 0x23, 0x02); // sh
  ght_cfg_filter(0x02, 0x02, 0x23, 0x02); // sw
  ght_cfg_filter(0x02, 0x03, 0x23, 0x02); // sd

  // se: 00, end_id: 0x02, scheduling: rr, start_id: 0x01
  ght_cfg_se (0x00, 0x02, 0x01, 0x01);
  // se: 01, end_id: 0x04, scheduling: rr, start_id: 0x03
  ght_cfg_se (0x01, 0x04, 0x01, 0x03);

  ght_cfg_mapper (0x01, 0b0011);
  ght_cfg_mapper (0x01, 0b0001);


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
                "addi t0,   t0,   256;"            // write address + 4
                "blt  t0,   a5,  .loop_store;");    

  //=================== Post execution ===================//
  ght_set_status (0x02); // ght: stop
  uint64_t status;
  while ((status = ght_get_status()) < 0x1FFFF)
  {

  }

  lock_acquire(&uart_lock);
  printf("All tests are done! Status: %x \r\n", status);
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

      case 0x05:
        task_Sanitiser(Hart_id);
      break;

      case 0x06:
        task_Sanitiser(Hart_id);
      break;

      case 0x07:
        task_Sanitiser(Hart_id);
      break;

      default:
      break;
  }
  
  idle();
  return 0;
}