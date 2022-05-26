#include <stdint.h>
#include <stdio.h>

extern int uart_lock;
extern char* shadow;

int task_PerfCounter(uint64_t core_id) {
  uint64_t Func_Opcode = 0x0;
  uint64_t perfc = 0;

  //================== Initialisation ==================//
  while (ghe_checkght_status() == 0x00){
  };

  //===================== Execution =====================// 
  while (ghe_checkght_status() != 0x02){
    while (ghe_status() != GHE_EMPTY){
      ROCC_INSTRUCTION_D (1, Func_Opcode, 0x03);
      perfc = perfc + 1;
    }


    if ((ghe_status() == GHE_EMPTY) && (ghe_checkght_status() == 0x00)) {
      ghe_complete();
      while((ghe_checkght_status() == 0x00)) {
        // Wait big core to re-start
      }
      ghe_go();
    }
  }

  //=================== Post execution ===================//
  lock_acquire(&uart_lock);
  printf("[C%x PMC]: Completed, PMC = %x! \r\n", core_id, perfc);
  lock_release(&uart_lock);
  ghe_release();
  
  idle();
  return 0;
}


int task_Sanitiser(uint64_t core_id) {
  uint64_t Address = 0x0;
  uint64_t Err_Cnt = 0x0;

  //================== Initialisation ==================//
  while (ghe_checkght_status() == 0x00){
  };

  //===================== Execution =====================// 
  while (ghe_checkght_status() != 0x02){
    while (ghe_status() != GHE_EMPTY){    
      ROCC_INSTRUCTION_D (1, Address, 0x05);
      asm volatile("fence rw, rw;");

      char bits = shadow[(Address)>>7];
      
      // if(!bits) continue;

      if(bits & (1<<((Address >> 7)&8))) {
        /*
        lock_acquire(&uart_lock);
        printf("[C%x Sani]: Error memory accesses is detected at %x. \r\n", core_id, Address);
        lock_release(&uart_lock);
        return -1;
        */
        Err_Cnt ++;
      }
    }


    if ((ghe_status() == GHE_EMPTY) && (ghe_checkght_status() == 0x00)) {
      ghe_complete();
      while((ghe_checkght_status() == 0x00)) {
        // Wait big core to re-start
      }
      ghe_go();
    }
  }


  //=================== Post execution ===================//
  lock_acquire(&uart_lock);
  printf("[C%x Sani]: Completed, %x illegal accesses are detected.\r\n", core_id, Err_Cnt);
  lock_release(&uart_lock);
  ghe_release();      
  idle();
  
  return 0;
}
