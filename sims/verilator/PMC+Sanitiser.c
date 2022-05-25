#include <stdio.h>
#include <stdlib.h>
#include "rocc.h"
#include "spin_lock.h"
#include "gh_sbsys.h"
#include "ghe.h"
#include "malloc.h"

int uart_lock;
void task_Sanitiser(uint64_t core_id);
void task_PerfCounter(uint64_t core_id);

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

  *(ptr) = sum;
  sum = sum + *(ptr);
  */

  //=================== Post execution ===================//
  ght_set_status (0x02); // ght: stop
  while (ght_get_status() < 0x0F) {

  }

  lock_acquire(&uart_lock);
  printf("C0: All tests are done! The test result is: %d!\n", sum);
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
  
  idle();
  return 0;
}

void task_PerfCounter(uint64_t core_id) {
  uint64_t Func_Opcode = 0x0;
  uint64_t perfc = 0;

  //================== Initialisation ==================//
  while (ghe_checkght_status() == 0x00){
  };

  while(1)
  {
    if (ghe_status() != GHE_EMPTY)
    {     
      ROCC_INSTRUCTION_D (1, Func_Opcode, 0x03);
      perfc = perfc + 1;
    }

    if ((ghe_status() == GHE_EMPTY) && (ghe_checkght_status() == 0x00)){
      // lock_acquire(&uart_lock);
      // printf("C%x: PMC = %x \n", core_id, perfc);
      // lock_release(&uart_lock);
      ghe_complete();
      while((ghe_checkght_status() == 0x00)) {
      }
      ghe_go();
    } 
  }
  //=================== Post execution ===================//

  
  ghe_complete();  
}



void task_Sanitiser(uint64_t core_id) {
  uint64_t Func_Opcode = 0x0;
  uint64_t Address = 0x0;
  uint64_t perfc = 0;

 //================== Initialisation ==================//
  while (ghe_checkght_status() == 0x00){
  };

  //===================== Execution =====================//
  while(1)
  {
    if (ghe_status() != GHE_EMPTY)
    {     
      ROCC_INSTRUCTION_D (1, Address, 0x05);
      asm volatile("fence rw, rw;");


      char bits = shadow[(Address)>>7];
      
      if(!bits) continue;
      if(bits & (1<<((Address >> 7)&8))) {
        lock_acquire(&uart_lock);
        printf("C%x: an error access at memory address %x \r\n", core_id, Address);
        lock_release(&uart_lock);
      }
    }

    if ((ghe_status() == GHE_EMPTY) && (ghe_checkght_status() == 0x00)){

      ghe_complete();
      while((ghe_checkght_status() == 0x00)) {
        
      }
      ghe_go();
    }
  }  
}