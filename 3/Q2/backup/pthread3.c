/******************************************************************

Program to code and demonstarte usage of mutexlock for Thread safe
update of a complex safe with a timestamp 


Thread one - update X,Y,Z co-ordinates and other attributes at sample time 
		   - for analysis purposes X,Y,Z increasing by 2 every time till 360
				and reinitializing 
	

Thread two - Read this value of sampled time 



Both Threads with same priority

updated by chinmay Shah for mutex Lock 
orginal code by Sam Siewart referred from ptherad3.c for Ex3 Q2



*****************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>

#define NUM_THREADS		3
#define START_SERVICE 	0
#define READ_SERVICE 	1
#define WRITE_SERVICE 	2
//#define LOW_PRIO_SERVICE 	3
//#define NUM_MSGS 		3


//#define MUTEX_LOCK 
#define MAX_VALUE 360

//Struture definitions 

struct nag_state
{
	struct timespec timeNow;
	double X,Y,Z;
	double acc;
	double roll,pitch,yaw;
	
};

//Global data to be read and updated 
struct nag_state ng_state;



//Priority and shceduling attributes
pthread_t threads[NUM_THREADS];
pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param nrt_param;

pthread_mutex_t msgSem;
pthread_mutexattr_t rt_safe;

int rt_protocol;

volatile int runInterference=0, CScount=0;
volatile unsigned long long idleCount[NUM_THREADS];
int intfTime=0;


void *startService(void *threadid);

unsigned int idx = 0, jdx = 1;
unsigned int seqIterations = 47;
unsigned int reqIterations = 1, Iterations = 1000;
unsigned int fib = 0, fib0 = 0, fib1 = 1;

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



void *Write(void *threadid)
{
  struct timespec timeWrite;

  /*************
  The mutex object referenced by mutex shall be locked by calling pthread_mutex_lock().
  If the mutex is already locked,
  the calling thread shall block until the mutex becomes available
  *****/
  while(1)
  { 
		  printf("\n Thread id %d \n",(int *)threadid);
		  //Default init ??
		  #ifdef MUTEX_LOCK
			pthread_mutex_lock(&msgSem);
		  #endif	
		  gettimeofday(&ng_state.timeNow);//Time
		  gettimeofday(&timeWrite);
		  
		  //printf("**** %d Before Updating Navigation values , %d nsec\n", (int)threadid, 
		  printf("****@ before update  %d secs %d nsec Values : %lf %lf %lf \n", ng_state.timeNow.tv_sec, ng_state.timeNow.tv_nsec,ng_state.X,ng_state.Y,ng_state.Z);
		  if (ng_state.X>=MAX_VALUE)
		  {
			  ng_state.X=0;
		  }
		  else
		  {
			  ///delay 
			  FIB_TEST(seqIterations,13000);//approx 10ms/10
			  ng_state.X++;  
		  }	  
		  if (ng_state.Y>=MAX_VALUE)
		  {
			  ng_state.Y=0;
		  }
		  else
		  {
			  ///delay 
			  FIB_TEST(seqIterations,13000);//approx 10ms/10
			  ng_state.Y+=2;  
		  }	  
		  
		  
		  if (ng_state.Z>=MAX_VALUE)
		  {
			  ng_state.Z=0;
		  }
		  else
		  {
			  ///delay 
			  FIB_TEST(seqIterations,13000);//approx 10ms/10
			  ng_state.Z+=5;  
		  }	  
		  
		  #ifdef MUTEX_LOCK
			pthread_mutex_unlock(&msgSem);//unlock if locked
		  #endif
		  
		  gettimeofday(&timeWrite);//Time
		  printf("****@ after update %d secs %d nsec Values : %lf %lf %lf \n", timeWrite.tv_sec, timeWrite.tv_nsec,ng_state.X,ng_state.Y,ng_state.Z);
		  //printf("**** %d After Updating Navigation values , %d nsec\n", (int)threadid, timeWrite.tv_sec, timeWrite.tv_nsec);
		  
	}	  

		
		  pthread_exit(NULL);
}

void *Read(void *threadid)
{
  struct timespec timeRead;

  /*************
  The mutex object referenced by mutex shall be locked by calling pthread_mutex_lock().
  If the mutex is already locked,
  the calling thread shall block until the mutex becomes available
  *****/
  while(1)
  {
	  printf("\n Reading %d \n",(int *)threadid);
	  //Default init ??
	  #ifdef MUTEX_LOCK
		pthread_mutex_lock(&msgSem);
	  #endif
		  gettimeofday(&timeRead);
		  printf("**** Reading @  ,%d secs-%d nsec\n",timeRead.tv_sec, timeRead.tv_nsec);  
		  printf("****@ %d secs %d nsec Values : %lf %lf %lf \n", ng_state.timeNow.tv_sec, ng_state.timeNow.tv_nsec,ng_state.X,ng_state.Y,ng_state.Z);
	  #ifdef MUTEX_LOCK
			pthread_mutex_unlock(&msgSem);//unlock if locked
	  #endif
  }
	  pthread_exit(NULL);
  	  
}

void print_scheduler(void)
{
   int schedType;

   schedType = sched_getscheduler(getpid());

   switch(schedType)
   {
     case SCHED_FIFO:
	   printf("Pthread Policy is SCHED_FIFO\n");
	   break;
     case SCHED_OTHER:
	   printf("Pthread Policy is SCHED_OTHER\n");
       break;
     case SCHED_RR:
	   printf("Pthread Policy is SCHED_OTHER\n");
	   break;
     default:
       printf("Pthread Policy is UNKNOWN\n");
   }
}

int main (int argc, char *argv[])
{
   int rc, invSafe=0, i, scope;
   struct timespec sleepTime, dTime;

   //CScount=0;

    
    //printing the execution time between creation of threads
    //Schedule policy is SCHED_OTHER at present
   print_scheduler();

   //Assign Attribute, scheduling policy - SCHED_FIFO
   pthread_attr_init(&rt_sched_attr[START_SERVICE]);
   pthread_attr_setinheritsched(&rt_sched_attr[START_SERVICE], PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr[START_SERVICE], SCHED_FIFO);

   pthread_attr_init(&rt_sched_attr[WRITE_SERVICE]);
   pthread_attr_setinheritsched(&rt_sched_attr[WRITE_SERVICE], PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr[WRITE_SERVICE], SCHED_FIFO);

   pthread_attr_init(&rt_sched_attr[READ_SERVICE]);
   pthread_attr_setinheritsched(&rt_sched_attr[READ_SERVICE], PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr[READ_SERVICE], SCHED_FIFO);

   
   rt_max_prio = sched_get_priority_max(SCHED_FIFO);
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);

   /*
   sched_setparam() sets the scheduling parameters associated with the scheduling policy for the process identified by pid.
    If pid is zero, then the parameters of the calling process are set.
    The interpretation of the argument param depends on the scheduling policy of the process identified by pid.
     See sched_setscheduler(2) for a description of the scheduling policies supported under Linux.

    sched_getparam() retrieves the scheduling parameters for the process identified by pid.
    If pid is zero, then the parameters of the calling process are retrieved.
   */
   rc=sched_getparam(getpid(), &nrt_param);

   if (rc)
   {
       printf("ERROR; sched_setscheduler rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }

   print_scheduler();//prints scheduling to be still SCHED_OTHER

   printf("min prio = %d, max prio = %d\n", rt_min_prio, rt_max_prio);

   /*
    PTHREAD_SCOPE_SYSTEM
    //The thread competes for resources with all other threads in all processes on the system that are in the same scheduling
    //allocation domain (a group of one or more processors). PTHREAD_SCOPE_SYSTEM threads are scheduled relative to one
     another according to their scheduling policy and priority.
     */
   pthread_attr_getscope(&rt_sched_attr[START_SERVICE], &scope);

   if(scope == PTHREAD_SCOPE_SYSTEM)
     printf("PTHREAD SCOPE SYSTEM\n");
   else if (scope == PTHREAD_SCOPE_PROCESS)
     printf("PTHREAD SCOPE PROCESS\n");
   else
     printf("PTHREAD SCOPE UNKNOWN\n");


     /*
     The pthread_mutex_init() function shall initialize the
     mutex referenced by mutex with attributes specified by attr.
     If attr is NULL, the default mutex attributes are used;
     the effect shall be the same as passing the address of a
     default mutex attributes object. Upon successful initialization,
      the state of the mutex becomes initialized and unlocked.
     */
   pthread_mutex_init(&msgSem, NULL);

   rt_param[START_SERVICE].sched_priority = rt_max_prio;//setting priority 99
   //Set attribute

   /*
   The pthread_attr_setschedparam() function sets the scheduling parameter
   attributes of the thread attributes object referred to by attr to the
   values specified in the buffer pointed to by param. These attributes determine the scheduling
   parameters of a thread created using the thread attributes object attr.
   */
   pthread_attr_setschedparam(&rt_sched_attr[START_SERVICE], &rt_param[START_SERVICE]);

   printf("Creating thread %d\n", START_SERVICE);
   //Create the thread 0 and call start Service
   rc = pthread_create(&threads[START_SERVICE], &rt_sched_attr[START_SERVICE], startService, (void *)START_SERVICE);
   // extra check for scheduler
    print_scheduler();
   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   printf("Start services thread spawned\n");


   printf("will join service threads\n");

   if(pthread_join(threads[START_SERVICE], NULL) == 0)
     printf("START SERVICE done\n");
   else
     perror("START SERVICE");


   rc=sched_setscheduler(getpid(), SCHED_OTHER, &nrt_param);

   if(pthread_mutex_destroy(&msgSem) != 0)
     perror("mutex destroy");

   printf("All done\n");

   exit(0);
}


void *startService(void *threadid)
{
   struct timespec timeNow;
   int rc;

   //runInterference=intfTime;

   
   rt_param[WRITE_SERVICE].sched_priority = rt_max_prio-10;
   pthread_attr_setschedparam(&rt_sched_attr[WRITE_SERVICE], &rt_param[WRITE_SERVICE]);
    //Creating Mid Priority thread and function called with no Mutex
   printf("Creating thread %d\n", WRITE_SERVICE);
   rc = pthread_create(&threads[WRITE_SERVICE], &rt_sched_attr[WRITE_SERVICE], Write, (void *)WRITE_SERVICE);
   //trying with mutex
   //rc = pthread_create(&threads[MID_PRIO_SERVICE], &rt_sched_attr[MID_PRIO_SERVICE], idle, (void *)MID_PRIO_SERVICE);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   //pthread_detach(threads[MID_PRIO_SERVICE]);
   gettimeofday(&timeNow);
   printf("Write  prio %d thread spawned at %d sec, %d nsec\n", WRITE_SERVICE, timeNow.tv_sec, timeNow.tv_nsec);

	
	sleep(1);
	
	
   rt_param[READ_SERVICE].sched_priority = rt_max_prio-20;//Set priority of Low Priority Thread
   pthread_attr_setschedparam(&rt_sched_attr[READ_SERVICE], &rt_param[READ_SERVICE]);//Set Schedule paramater of this thread

   printf("Creating Read thread %d\n", READ_SERVICE);
   //create this low priority thread and call idle function
   rc = pthread_create(&threads[READ_SERVICE], &rt_sched_attr[READ_SERVICE], Read, (void *)READ_SERVICE);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   //pthread_detach(threads[LOW_PRIO_SERVICE]);
   gettimeofday(&timeNow);
   printf("Low prio %d  read service thread spawned at %d sec, %d nsec\n", READ_SERVICE, timeNow.tv_sec, timeNow.tv_nsec);

  
   
   if(pthread_join(threads[READ_SERVICE], NULL) == 0)
     printf("READ SERVICE done\n");
   else
     perror("READ SERVICE");

   if(pthread_join(threads[WRITE_SERVICE], NULL) == 0)
     printf("WRITE SERVICE done\n");
   else
     perror("WRITE SERVICE");
 
   pthread_exit(NULL);

}
