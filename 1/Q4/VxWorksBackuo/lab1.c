/****************************************************************************/
/*                                                                          */
/* Original code Sam Siewert - 2005

                                             */
/*                                                                          */
/****************************************************************************/

#include "vxWorks.h"
#include "semLib.h"
#include "sysLib.h"
#include "wvLib.h"

#define FIB_LIMIT_FOR_32_BIT 47

SEM_ID semF10, semF20;
int abortTest = 0;
UINT32 seqIterations = FIB_LIMIT_FOR_32_BIT;
UINT32 idx = 0, jdx = 1;
UINT32 fib = 0, fib0 = 0, fib1 = 1;
UINT32 fib10Cnt=0, fib20Cnt=0;
char ciMarker[]="CI";


#define FIB_TEST(seqCnt, iterCnt)      \
   for(idx=0; idx < iterCnt; idx++)    \
   {                                   \
      fib = fib0 + fib1;               \
      while(jdx < seqCnt)              \
      {                                \
         fib0 = fib1;                  \
         fib1 = fib;                   \
         fib = fib0 + fib1;            \
         jdx++;                        \
      }                                \
   }                                   \


/* Iterations, 2nd arg must be tuned for any given target type
   using windview

   170000 <= 10 msecs on Pentium at home

   Be very careful of WCET overloading CPU during first period of
   LCM.

 */
void fib10(void)
{
   while(!abortTest)
   {
	   semTake(semF10, WAIT_FOREVER);
	   FIB_TEST(seqIterations, 170000);
	   fib10Cnt++;
   }
}

void fib20(void)
{
   while(!abortTest)
   {
	   semTake(semF20, WAIT_FOREVER);
	   FIB_TEST(seqIterations, 340000);
	   fib20Cnt++;
   }
}

void shutdown(void)
{
	abortTest=1;
}


void Sequencer(void)
{

  printf("Starting Sequencer\n");

  /* Just to be sure we have 1 msec tick and TOs */
  sysClkRateSet(1000);

  /* Set up service release semaphores
  SEM_ID semBCreate
    (
    int         options,      ///* semaphore options
    SEM_B_STATE initialState  ///* initial semaphore state
    )
  */
  //create and initialize a binary semaphore


  semF10 = semBCreate(SEM_Q_FIFO, SEM_EMPTY);
  semF20 = semBCreate(SEM_Q_FIFO, SEM_EMPTY);

  //creating a task

  /*
  int taskSpawn
    (
    char *  name,    //         /* name of new task (stored at pStackBase)
    int     priority,//         /* priority of new task
    int     options, //         /* task option word
    int     stackSize,//        /* size (bytes) of stack needed plus name
    FUNCPTR entryPt,  //        /* entry point of new task
    int     arg1,     //        /* 1st of 10 req'd task args to pass to func
    int     arg2,
    int     arg3,
    int     arg4,
    int     arg5,
    int     arg6,
    int     arg7,
    int     arg8,
    int     arg9,
    int     arg10
    )

  */
  if(taskSpawn("serviceF10", 21, 0, 8192, fib10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
  {
    printf("F10 task spawn failed\n");
  }
  else
    printf("F10 task spawned\n");

  if(taskSpawn("serviceF20", 22, 0, 8192, fib20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
  {
    printf("F20 task spawn failed\n");
  }
  else
    printf("F20 task spawned\n");


  /* Simulate the C.I. for S1 and S2 and mark on windview and log
     wvEvent first because F10 and F20 can preempt this task!
   */
  if(wvEvent(0xC, ciMarker, sizeof(ciMarker)) == ERROR)
	  printf("WV EVENT ERROR\n");
  semGive(semF10); semGive(semF20);


  /* Sequencing loop for LCM phasing of S1, S2
   */
  while(!abortTest)
  {

	  /* Basic sequence of releases after CI */
      taskDelay(20); semGive(semF10);
      taskDelay(20); semGive(semF10);
      taskDelay(10); semGive(semF20);
      taskDelay(10); semGive(semF10);
      taskDelay(20); semGive(semF10);
      taskDelay(20);

	  /* back to C.I. conditions, log event first due to preemption */
	  if(wvEvent(0xC, ciMarker, sizeof(ciMarker)) == ERROR)
		  printf("WV EVENT ERROR\n");
	  semGive(semF10);
	  semGive(semF20);
  }


}

//Loooks like main
void start(void)
{
	abortTest=0;

	if(taskSpawn("Sequencer", 20, 0, 8192, Sequencer, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
	{
	  printf("Sequencer task spawn failed\n");
	}
	else
	  printf("Sequencer task spawned\n");

}

