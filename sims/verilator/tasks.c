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
      ROCC_INSTRUCTION_D (1, Func_Opcode, 0x0B);
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
      ROCC_INSTRUCTION_D (1, Address, 0x0D);
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


int task_ShadowStack_S (uint64_t core_id) {
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
      ROCC_INSTRUCTION_D (1, Header, 0x0A);
      ROCC_INSTRUCTION_D (1, Payload, 0x0D);

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
            // return -1;
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
  if (core_id == 1){
    lock_acquire(&uart_lock);
    printf("[C%x SS]: Completed. No error is detected\r\n", core_id);
    lock_release(&uart_lock);
  }
  ghe_release();

  return 0;
}


int task_ShadowStack_M_Pre (uint64_t core_id) {
  uint64_t Address = 0x0;
  uint64_t Header = 0x0;  
  uint64_t Opcode = 0x0;
  uint64_t Rd = 0x0;
  uint64_t RS1 = 0x0;
  uint64_t Payload = 0x0;
  uint64_t Header_index = (core_id << 32);
  uint64_t Err_Cnt = 0x0;
  uint64_t pause = 0xFF;

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
      ROCC_INSTRUCTION_D (1, Header, 0x0A);
      ROCC_INSTRUCTION_D (1, Payload, 0x0D);

      Opcode = Header & 0x7F;
      Rd = (Header & 0xF80) >> 7;
      
      // Push -- a function is called
      if (((Opcode == 0x6F) || (Opcode == 0x67)) && (Rd == 0x01)) {
        if (full(&shadow_payload) == 0) {
          enqueueF(&shadow_header, Header);
          enqueueF(&shadow_payload, Payload);
        } else {
          lock_acquire(&uart_lock);
          printf("[C%x SS]: **Error** shadow stack is full. \r\n", core_id);
          lock_release(&uart_lock);
        }
      }

      // Pull -- a function is returned
      if ((Opcode == 0x67) && (Rd == 0x00)) {
        if (empty(&shadow_payload) == 1) {
          // Send it to AGG
          while (ghe_agg_status() == GHE_FULL) {
          }
          uint64_t S_Header = Header | Header_index;
          uint64_t S_Payload = Payload;
          ghe_agg_push (S_Header, S_Payload);
        } else {
          u_int64_t comp = dequeueF(&shadow_payload);
          dequeueF(&shadow_header);
          
          if (comp != Payload){
            Err_Cnt++;
            lock_acquire(&uart_lock);
            printf("[C%x SS]: **Error** %x v.s. %x. \r\n", core_id, Payload, comp);
            lock_release(&uart_lock);
          } else {
            // Successfully paired
          }
        }
      }

      if (pause != 0x00) {
        pause = 0x00;
      }
    }

    if ((ghe_sch_status() == 0x01) && (pause == 0x00))
    {
      while (ghe_agg_status() == GHE_FULL) {
      }
      ghe_agg_push ((0xFFFFFFFF|Header_index), 0x0);
      pause = 1;
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
  printf("[C%x SS]: Completed, %x errors are detected.\r\n", core_id, Err_Cnt);
  lock_release(&uart_lock);
  ghe_release();      
  
  return 0;
}


int task_ShadowStack_Agg (uint64_t core_id) {
  uint64_t Address = 0x0;
  uint64_t Header = 0x0;  
  uint64_t Opcode = 0x0;
  uint64_t Rd = 0x0;
  uint64_t RS1 = 0x0;
  uint64_t Payload = 0x0;
  uint64_t Err_Cnt = 0x0;



  //================== Initialisation ==================//
  dequeue  shadow_header;
  dequeue  shadow_payload;
  dequeue  queues_header[NUM_CORES];
  dequeue  queues_payload[NUM_CORES];
  
  initialize(&shadow_header);
  initialize(&shadow_payload);

  for (int i = 0; i < NUM_CORES; i ++)
  {
    initialize(&queues_header[i]);
    initialize(&queues_payload[i]);
  }


  while (ghe_checkght_status() == 0x00){
  };

  //===================== Execution =====================// 
  while (ghe_checkght_status() != 0x02){
    while (ghe_status() != GHE_EMPTY){
      ROCC_INSTRUCTION_D (1, Header, 0x0A);
      ROCC_INSTRUCTION_D (1, Payload, 0x0D);
      uint64_t from = (Header>>32) & 0xF;
      uint64_t inst = Header & 0xFFFFFFFF;

      Opcode = Header & 0x7F;
      Rd = (Header & 0xF80) >> 7;
      
    }
  }


  //=================== Post execution ===================//
  lock_acquire(&uart_lock);
  printf("[C%x SS]: Completed, %x errors are detected.\r\n", core_id, Err_Cnt);
  lock_release(&uart_lock);
  ghe_release();      
  
  return 0;
}
