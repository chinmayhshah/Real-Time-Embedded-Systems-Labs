#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>

#define NUM_THREADS		4
#define START_SERVICE 		0
#define HIGH_PRIO_SERVICE 	1
#define MID_PRIO_SERVICE 	2
#define LOW_PRIO_SERVICE 	3
#define NUM_MSGS 		3

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



// no Mutex  and less delay
void *idleNoSem(void *threadid)
{
  struct timespec timeNow;

  do
  {
    //idx=0 , jdx=1;//extra added
    FIB_TEST(seqIterations, Iterations);
    idleCount[(int)threadid]++;
  } while(idleCount[(int)threadid] < runInterference);

  gettimeofday(&timeNow);
  printf("**** %d idle NO SEM stopping at %d sec, %d nsec\n", (int)threadid, timeNow.tv_sec, timeNow.tv_nsec);

  pthread_exit(NULL);
}


int CScnt=0;

void *idle(void *threadid)
{
  struct timespec timeNow;

  /*************
  The mutex object referenced by mutex shall be locked by calling pthread_mutex_lock().
  If the mutex is already locked,
  the calling thread shall block until the mutex becomes available
  *****/
   printf("\n Thread id %d \n",(int *)threadid);
  //Default init ??
  pthread_mutex_lock(&msgSem);

  CScnt++;//increment

  do
  {
    FIB_TEST(seqIterations, Iterations);// Loop
    idleCount[(int)threadid]++;//increment
  } while(idleCount[(int)threadid] < runInterference);

  sleep(2);

  idleCount[(int)threadid]=0;// zero

  do
  {
    FIB_TEST(seqIterations, Iterations);
    idleCount[(int)threadid]++;
  } while(idleCount[(int)threadid] < runInterference);

  pthread_mutex_unlock(&msgSem);//unlock if locked

  gettimeofday(&timeNow);//Time
  printf("**** %d idle stopping at %d sec, %d nsec\n", (int)threadid, timeNow.tv_sec, timeNow.tv_nsec);

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

   CScount=0;

    //expecting argument
    //
   if(argc < 2)
   {
     printf("Usage: pthread interfere-seconds\n");
     exit(-1);
   }
   else if(argc >= 2)
   {
     sscanf(argv[1], "%d", &intfTime);
     //Interference time is input to pthread
     printf("interference time = %d secs\n", intfTime);
     printf("unsafe mutex will be created\n");
   }

    //printing the execution time between creation of threads
    //Schedule policy is SCHED_OTHER at present
   print_scheduler();

   //Assign Attribute, scheduling policy - SCHED_FIFO
   pthread_attr_init(&rt_sched_attr[START_SERVICE]);
   pthread_attr_setinheritsched(&rt_sched_attr[START_SERVICE], PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr[START_SERVICE], SCHED_FIFO);

   pthread_attr_init(&rt_sched_attr[HIGH_PRIO_SERVICE]);
   pthread_attr_setinheritsched(&rt_sched_attr[HIGH_PRIO_SERVICE], PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr[HIGH_PRIO_SERVICE], SCHED_FIFO);

   pthread_attr_init(&rt_sched_attr[MID_PRIO_SERVICE]);
   pthread_attr_setinheritsched(&rt_sched_attr[MID_PRIO_SERVICE], PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr[MID_PRIO_SERVICE], SCHED_FIFO);

   pthread_attr_init(&rt_sched_attr[LOW_PRIO_SERVICE]);
   pthread_attr_setinheritsched(&rt_sched_attr[LOW_PRIO_SERVICE], PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr[LOW_PRIO_SERVICE], SCHED_FIFO);

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

   runInterference=intfTime;

   rt_param[LOW_PRIO_SERVICE].sched_priority = rt_max_prio-20;//Set priority of Low Priority Thread
   pthread_attr_setschedparam(&rt_sched_attr[LOW_PRIO_SERVICE], &rt_param[LOW_PRIO_SERVICE]);//Set Schedule paramater of this thread

   printf("Creating thread %d\n", LOW_PRIO_SERVICE);
   //create this low priority thread and call idle function
   rc = pthread_create(&threads[LOW_PRIO_SERVICE], &rt_sched_attr[LOW_PRIO_SERVICE], idle, (void *)LOW_PRIO_SERVICE);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   //pthread_detach(threads[LOW_PRIO_SERVICE]);
   gettimeofday(&timeNow);
   printf("Low prio %d thread spawned at %d sec, %d nsec\n", LOW_PRIO_SERVICE, timeNow.tv_sec, timeNow.tv_nsec);


   sleep(1);

   rt_param[MID_PRIO_SERVICE].sched_priority = rt_max_prio-10;
   pthread_attr_setschedparam(&rt_sched_attr[MID_PRIO_SERVICE], &rt_param[MID_PRIO_SERVICE]);
    //Creating Mid Priority thread and function called with no Mutex
   printf("Creating thread %d\n", MID_PRIO_SERVICE);
   rc = pthread_create(&threads[MID_PRIO_SERVICE], &rt_sched_attr[MID_PRIO_SERVICE], idleNoSem, (void *)MID_PRIO_SERVICE);
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
   printf("Middle prio %d thread spawned at %d sec, %d nsec\n", MID_PRIO_SERVICE, timeNow.tv_sec, timeNow.tv_nsec);

   rt_param[HIGH_PRIO_SERVICE].sched_priority = rt_max_prio-1;
   pthread_attr_setschedparam(&rt_sched_attr[HIGH_PRIO_SERVICE], &rt_param[HIGH_PRIO_SERVICE]);


   printf("Creating thread %d, CScnt=%d\n", HIGH_PRIO_SERVICE, CScnt);
   rc = pthread_create(&threads[HIGH_PRIO_SERVICE], &rt_sched_attr[HIGH_PRIO_SERVICE], idle, (void *)HIGH_PRIO_SERVICE);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   //pthread_detach(threads[HIGH_PRIO_SERVICE]);
   gettimeofday(&timeNow);
   printf("High prio %d thread spawned at %d sec, %d nsec\n", HIGH_PRIO_SERVICE, timeNow.tv_sec, timeNow.tv_nsec);





   if(pthread_join(threads[LOW_PRIO_SERVICE], NULL) == 0)
     printf("LOW PRIO done\n");
   else
     perror("LOW PRIO");

   if(pthread_join(threads[MID_PRIO_SERVICE], NULL) == 0)
     printf("MID PRIO done\n");
   else
     perror("MID PRIO");

   if(pthread_join(threads[HIGH_PRIO_SERVICE], NULL) == 0)
     printf("HIGH PRIO done\n");
   else
     perror("HIGH PRIO");


   pthread_exit(NULL);

}
