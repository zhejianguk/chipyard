#include <stdio.h>
#include <stdlib.h>
#include "rocc.h"
#include "spin_lock.h"
#include "ght.h"
#include "ghe.h"
#include "malloc.h"
#include "tasks.h"
#include "deque.h"


int task_hello (int hart_id)
{
  lock_acquire(&uart_lock);
  printf("Hello, World! From Hart %d. \n", hart_id);
  lock_release(&uart_lock);

  return 0;
}

uint64_t task_synthetic ()
{
  uint64_t output;
  //===================== Execution =====================//
  __asm__(
          "li   t0,   0x81000000;"         // write pointer
          "li   t1,   0x55555000;"         // data
          "li   t2,   0x81000000;"         // Read pointer
          "j    .loop_store;");

  __asm__(
          ".loop_store:"
          "li   a5,   0x81000FFF;"
          "sw         t1,   (t0);"
          "addi t1,   t1,   1;"            // data + 1
          "addi t0,   t0,   64;"            // write address + 4
          "blt  t0,   a5,  .loop_store;"
          "li   t0,   0x82000000;"
          "li   t2,   0x81000000;"
          "j    .loop_load;");

  __asm__(
          ".loop_load:"
          "li   a5,   0x82000FFF;"
          "lw   t1,   (t2);"
          "sw         t1,   (t0);"
          "addi t0,   t0,   64;"
          "addi t2,   t2,   64;"
          "blt  t0,   a5,  .loop_load;");

  __asm__(
          ".loop_load2:"
          "li   a5,   0x82000FFF;"
          "lw   t1,   (t2);"
          "sw         t1,   (t0);"
          "addi t0,   t0,   64;"
          "addi t2,   t2,   64;"
          "blt  t0,   a5,  .loop_load;");

   return output;
}


int task_PerfCounter(uint64_t core_id) {
  uint64_t Func_Opcode = 0x0;
  uint64_t perfc = 0;

  //================== Initialisation ==================//
  while (ghe_checkght_status() == 0x00){
  };

  //===================== Execution =====================// 
  while (ghe_checkght_status() != 0x02){
    while (ghe_status() != GHE_EMPTY){
      ROCC_INSTRUCTION_D (1, Func_Opcode, 0x05);
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
        
        lock_acquire(&uart_lock);
        printf("[C%x Sani]: **Error** illegal accesses at %x. \r\n", core_id, Address);
        lock_release(&uart_lock);
        Err_Cnt ++;
        // return -1;
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
  
  return 0;
}


int task_ShadowStack (uint64_t core_id) {
  uint64_t Address = 0x0;
  uint64_t Header = 0x0;  
  uint64_t Opcode = 0x0;
  uint64_t Rd = 0x0;
  uint64_t RS1 = 0x0;
  uint64_t Payload = 0x0;

  

  //================== Initialisation ==================//
  dequeue  shadow_header;
  dequeue  shadow_payload;
  initialize(&shadow_header);
  initialize(&shadow_payload);

  while (ghe_checkght_status() == 0x00){
  };

  //===================== Execution =====================// 
  while (ghe_checkght_status() != 0x02){
    while (ghe_status() != GHE_EMPTY){
      ROCC_INSTRUCTION_D (1, Header, 0x02);
      ROCC_INSTRUCTION_D (1, Payload, 0x05);

      Opcode = Header & 0x7F;
      Rd = (Header & 0xF80) >> 7;
      
      // Push -- a function is called
      if (((Opcode == 0x6F) || (Opcode == 0x67)) && (Rd == 0x01)) {
        if (full(&shadow_payload) == 0) {
          enqueueF(&shadow_header, Header);
          enqueueF(&shadow_payload, Payload);
          lock_acquire(&uart_lock);
         printf("[C%x SS]: Pushed. Addr: %x. \r\n", core_id, Payload);
         lock_release(&uart_lock);
        }
      }

      // Pull -- a function is returned
      if ((Opcode == 0x67) && (Rd == 0x00)) {
        if (empty(&shadow_payload) == 1) {
          // printf("[C%x ShadowStack]: **Empty**. Pulled %x. \r\n", core_id, Payload);
        } else {
          u_int64_t comp = dequeueF(&shadow_payload);
          dequeueF(&shadow_header);
          
          if (comp != Payload){
            lock_acquire(&uart_lock);
            printf("[C%x SS]: **Error** %x v.s. %x. \r\n", core_id, Payload, comp);
            lock_release(&uart_lock);
          } else {
            lock_acquire(&uart_lock);
            printf("[C%x SS]: Pulled. Addr: %x. \r\n", core_id, Payload);
            lock_release(&uart_lock);
          }
        }
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
  printf("[C%x SS]: Completed. No overflow is detected.\r\n", core_id);
  lock_release(&uart_lock);
  ghe_release();      
  
  return 0;
}
