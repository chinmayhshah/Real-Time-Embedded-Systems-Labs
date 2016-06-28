/**********************************************************
Updating code for Q4
Code the Fib10 and Fib20 synthetic load
generation and work to adjust iterations to see if you can at least produce a reliable 10
millisecond and 20 millisecond load on ECES Linux or a Jetson system (Jetson is preferred and
should result in more reliable results)


Original COde by Sam Siewart
pthread3.c and lab1.c
***********************************************************/
#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>


#define NUM_THREADS		3
#define SEQ_SERVICE 	0 //SEQUENCER SERVICE
#define FIB10_SERVICE 	1
#define FIB20_SERVICE 	2
//#define LOW_PRIO_SERVICE 	3
//#define NUM_MSGS 		3

#define NSEC_PER_SEC (1000000000)
#define DELAY_TICKS (1)
#define ERROR (-1)
#define OK (0)


void end_delay_test(void);


static struct timespec sleep_time = {0, 0};
static struct timespec sleep_requested = {0, 0};
static struct timespec remaining_time = {0, 0};
//Pthreads
pthread_t threads[NUM_THREADS];
pthread_attr_t rt_sched_attr[NUM_THREADS];
int rt_max_prio, rt_min_prio;
struct sched_param rt_param[NUM_THREADS];
struct sched_param nrt_param;
struct timespec timeNow;
int abortTest = 0;

static unsigned int sleep_count = 0;


//Semaphores
sem_t semF10,semF20;

//Mutexs
//pthread_mutex_t msgSem;
//pthread_mutexattr_t rt_safe;

int rt_protocol;

volatile int runInterference=0, CScount=0;
volatile unsigned long long idleCount[NUM_THREADS];
int intfTime=0;


void *startService(void *threadid);

unsigned int idx = 0, jdx = 1;
unsigned int seqIterations = 47;
unsigned int reqIterations = 1, Iterations = 1000;
unsigned int fib = 0, fib0 = 0, fib1 = 1;
unsigned  fib10Cnt=0, fib20Cnt=0;
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






int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
  int dt_sec=stop->tv_sec - start->tv_sec;
  int dt_nsec=stop->tv_nsec - start->tv_nsec;

  if(dt_sec >= 0)
  {
    if(dt_nsec >= 0)
    {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else
    {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
    }
  }
  else//why no change in assignment
  {
    if(dt_nsec >= 0)
    {
      delta_t->tv_sec=dt_sec;
      delta_t->tv_nsec=dt_nsec;
    }
    else
    {
      delta_t->tv_sec=dt_sec-1;
      delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
    }
  }

  return(OK);
}

static struct timespec rtclk_dt = {0, 0};
static struct timespec rtclk_start_time = {0, 0};
static struct timespec rtclk_stop_time = {0, 0};
static struct timespec delay_error = {0, 0};


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
       //printf("\n In Fib10");

	   sem_wait(&semF10);
	   FIB_TEST(seqIterations, 170000);
	   //gettimeofday(&timeNow);
       //printf("FIB10 thread %d thread executed  at %d sec, %d nsec\n", FIB10_SERVICE, timeNow.tv_sec, timeNow.tv_nsec);

	   fib10Cnt++;
   }

}

void fib20(void)
{
   while(!abortTest)
   {
       //printf("\n In Fib10");
	   sem_wait(&semF20);
	   //gettimeofday(&timeNow);
	   FIB_TEST(seqIterations, 340000);
       //printf("FIB20 thread %d thread executed  at %d sec, %d nsec\n", FIB20_SERVICE, timeNow.tv_sec, timeNow.tv_nsec);

	   fib20Cnt++;
   }


}


int CScnt=0;
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
   abortTest = 0;
   CScount=0;


   printf("Main Started ");
   //printing the execution time between creation of threads
   //Schedule policy is SCHED_OTHER at present
   print_scheduler();

   //Assign Attribute, scheduling policy - SCHED_FIFO to all tasks
   pthread_attr_init(&rt_sched_attr[SEQ_SERVICE]);
   pthread_attr_setinheritsched(&rt_sched_attr[SEQ_SERVICE], PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr[SEQ_SERVICE], SCHED_FIFO);

   pthread_attr_init(&rt_sched_attr[FIB10_SERVICE]);
   pthread_attr_setinheritsched(&rt_sched_attr[FIB10_SERVICE], PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr[FIB10_SERVICE], SCHED_FIFO);

   pthread_attr_init(&rt_sched_attr[FIB20_SERVICE]);
   pthread_attr_setinheritsched(&rt_sched_attr[FIB20_SERVICE], PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rt_sched_attr[FIB20_SERVICE], SCHED_FIFO);

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
   pthread_attr_getscope(&rt_sched_attr[SEQ_SERVICE], &scope);

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


   rt_param[SEQ_SERVICE].sched_priority = rt_max_prio;//setting priority 99
   /*
   The pthread_attr_setschedparam() function sets the scheduling parameter
   attributes of the thread attributes object referred to by attr to the
   values specified in the buffer pointed to by param. These attributes determine the scheduling
   parameters of a thread created using the thread attributes object attr.
   */
   pthread_attr_setschedparam(&rt_sched_attr[SEQ_SERVICE], &rt_param[SEQ_SERVICE]);
   printf("Creating thread %d\n", SEQ_SERVICE);
   //Create the thread 0 and call start Service(sequencer Service)
   rc = pthread_create(&threads[SEQ_SERVICE], &rt_sched_attr[SEQ_SERVICE], startService, (void *)SEQ_SERVICE);
   // extra check for scheduler
   //print_scheduler();
   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }

   printf("Start services thread spawned\n");
   printf("will join service threads\n");

   if(pthread_join(threads[SEQ_SERVICE], NULL) == 0)
     printf("SEQ_SERVICE done\n");
   else
     perror("SEQ_SERVICE");
   rc=sched_setscheduler(getpid(), SCHED_OTHER, &nrt_param);
   //Destroy semaphore instead of mutex
   /*
   if(pthread_mutex_destroy(&msgSem) != 0)
     perror("mutex destroy");

   printf("All done\n");
    */
   exit(0);
}


void *startService(void *threadid)
{
   //struct timespec timeNow;
   int rc;

   runInterference=intfTime;


   //create Semaphore
   //pthread_mutex_init(&msgSem, NULL);
   sem_init(&semF10,0,1);//Initialize a Binary semF10
   sem_init(&semF20,0,1);//Initialize a Binary semF20







   //Fib20 has lower priority then Fib10 priority
   rt_param[FIB20_SERVICE].sched_priority = rt_max_prio-20;//Set priority of Low Priority Thread(79)
   pthread_attr_setschedparam(&rt_sched_attr[FIB20_SERVICE], &rt_param[FIB20_SERVICE]);//Set Schedule paramater of this thread

   printf("Creating thread %d\n", FIB20_SERVICE);
   //create this low priority thread and call fib20 function
   rc = pthread_create(&threads[FIB20_SERVICE], &rt_sched_attr[FIB20_SERVICE], fib20, (void *)FIB20_SERVICE);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   //pthread_detach(threads[LOW_PRIO_SERVICE]);
   gettimeofday(&timeNow);
   printf("FIB20 Thread %d thread spawned at %d sec, %d nsec\n", FIB20_SERVICE, timeNow.tv_sec, timeNow.tv_nsec);

    // may not required ??
   //sleep(1);

   rt_param[FIB10_SERVICE].sched_priority = rt_max_prio-10;//Set higher than Fib10 less than sequencer (89)
   pthread_attr_setschedparam(&rt_sched_attr[FIB10_SERVICE], &rt_param[FIB10_SERVICE]);
    //Creating Mid Priority thread and function called with no Mutex
   printf("Creating thread %d\n", FIB10_SERVICE);
   rc = pthread_create(&threads[FIB10_SERVICE], &rt_sched_attr[FIB10_SERVICE], fib10, (void *)FIB10_SERVICE);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }

   //event
    clock_gettime(CLOCK_REALTIME, &rtclk_start_time);

   //
    sem_post(&semF10);sem_post(&semF20);





  /* Sequencing loop for LCM phasing of S1, S2
   */
  while(!abortTest)
  {

	  /* Basic sequence of releases after CI */
      usleep(20000); sem_post(&semF10);
      usleep(20000); sem_post(&semF10);
      usleep(10000); sem_post(&semF20);
      usleep(10000); sem_post(&semF10);
      usleep(20000); sem_post(&semF10);
      usleep(20000);

	  /* back to C.I. conditions, log event first due to preemption */
	 // if(wvEvent(0xC, ciMarker, sizeof(ciMarker)) == ERROR)
	//	  printf("WV EVENT ERROR\n");
	clock_gettime(CLOCK_REALTIME, &rtclk_stop_time);
	delta_t(&rtclk_stop_time, &rtclk_start_time, &rtclk_dt);
	printf("RT clock DT seconds = %ld, nanoseconds = %ld\n",
         rtclk_dt.tv_sec, rtclk_dt.tv_nsec);
	  sem_post(&semF10);
	  sem_post(&semF20);
	 clock_gettime(CLOCK_REALTIME, &rtclk_start_time);
  }


   //pthread_detach(threads[MID_PRIO_SERVICE]);
   gettimeofday(&timeNow);
   printf("FIB10 thread %d thread spawned at %d sec, %d nsec\n", FIB10_SERVICE, timeNow.tv_sec, timeNow.tv_nsec);


   if(pthread_join(threads[FIB20_SERVICE], NULL) == 0)
     printf("FIB20 PRIO done\n");
   else
     perror("FIB20 PRIO");

   if(pthread_join(threads[FIB10_SERVICE], NULL) == 0)
     printf("FIB10 PRIO done\n");
   else
     perror("FIB10 PRIO");

   pthread_exit(NULL);

}
