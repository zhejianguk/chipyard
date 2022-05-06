#include <stdio.h>
#include <stdlib.h>
#include "rocc.h"
#include "spin_lock.h"
#include "gh_sbsys.h"
#include "ghe.h"
#include "malloc.h"

int uart_lock;


/* Core_0 thread */
int main(void)
{
  lock_acquire(&uart_lock);
  printf("C0: Test is now start!\r\n");
  lock_release(&uart_lock);

  int n = 100;
  int *ptr = NULL;
  int sum = 0;

  ptr = (int*) malloc(n * sizeof(int));
 
  // if memory cannot be allocated
  if(ptr == NULL) {
    printf("C0: Error! memory not allocated.");
    exit(0);
  }

  for (int i = 0; i < n; i++)
  {
    *(ptr + i) = i;
  }

  for (int i = 0; i < n; i++)
  {
    sum = sum + *(ptr + i);
  }


    

  printf("C0: Sum = %d \r\n", sum);
  
  // deallocating the memory
  free(ptr);



  lock_acquire(&uart_lock);
  printf("C0: All tests are done!\n");
  lock_release(&uart_lock);
  return 0;
}
