/*************************************************************************************************
Updated code for Q4 creating scheduler  to schedule Fib10 and Fib20 in LCM invariant  
1)Generation and work to adjust iterations to see if you can at least produce a reliable 10
millisecond and 20 millisecond load on ALtera De1 Soc Ubuntu Linux
2)Ported code from lab1.c (VxWorks) code  and converted to Linux 
3)Incoporated use of affinity to set single core for execution of all threads within this process 


Original Code by Sam Siewart
pthread3.c and lab1.c

Creating Code by chinmay.shah@colorado.edu 
binSem_linux.c

*****************************************************************************************************/

//Macros 

#define _GNU_SOURCE


#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <semaphore.h>
#include <errno.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>


#define NUM_THREADS		3
#define SEQ_SERVICE 	0 //SEQUENCER SERVICE
#define FIB10_SERVICE 	1
#define FIB20_SERVICE 	2
#define NSEC_PER_SEC (1000000000)
#define DELAY_TICKS (1)
#define ERROR (-1)
#define OK (0)



//Global Variable 
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
struct sched_param main_param;
struct timespec timeNow;
int abortTest = 0;
static unsigned int sleep_count = 0;


//Semaphores
sem_t semF10,semF20;


int rt_protocol;
volatile int runInterference=0, CScount=0;
volatile unsigned long long idleCount[NUM_THREADS];
int intfTime=0;

//CPU Set for affinity
cpu_set_t cpuset;	


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



// To find Delta value between start and stop time 


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

//For Complete Event
static struct timespec rtclk_dt = {0, 0};
static struct timespec rtclk_start_time = {0, 0};
static struct timespec rtclk_stop_time = {0, 0};

// For FIB10 task
static struct timespec fib10_dt = {0, 0};
static struct timespec fib10_start_time = {0, 0};
static struct timespec fib10_stop_time = {0, 0};

// For FIB20 task
static struct timespec fib20_dt = {0, 0};
static struct timespec fib20_start_time = {0, 0};
static struct timespec fib20_stop_time = {0, 0};

static struct timespec delay_error = {0, 0};


/* Iterations, 2nd arg must be tuned for any given target type
   using windview

   1300000 <= 10 msecs on Pentium at home

   Be very careful of WCET overloading CPU during first period of LCM.

 */
 
 
 // Function for Fib10 Thread
void fib10(void)
{

   while(!abortTest)
   {
	   sem_wait(&semF10);//Wait for Semaphore 
       clock_gettime(CLOCK_REALTIME, &fib10_start_time);// Not been used 
       FIB_TEST(seqIterations, 1290000);//For 10 ms of execution time 
       clock_gettime(CLOCK_REALTIME, &fib10_stop_time);//Storing the stop time 
	   fib10Cnt++;
	   delta_t(&fib10_stop_time, &rtclk_start_time, &fib10_dt);
       printf("Fib10 entry %d  DT seconds = %ld, nanoseconds = %ld\n",fib10Cnt, fib10_dt.tv_sec, fib10_dt.tv_nsec);// Printing the Fib10 occurence from start time of event 
   }

}

// Function for Fib20 Thread

void fib20(void)
{
   while(!abortTest)
   {
	   sem_wait(&semF20);//Wait for Semaphore 
	   clock_gettime(CLOCK_REALTIME, &fib20_start_time);//not used
	   FIB_TEST(seqIterations, 2550000);
	   clock_gettime(CLOCK_REALTIME, &fib20_stop_time);//Storing the stop time 
	   fib20Cnt++;
	   delta_t(&fib20_stop_time, &rtclk_start_time, &fib20_dt);
       printf("Fib20 entry %d  DT seconds = %ld, nanoseconds = %ld\n",fib20Cnt, fib20_dt.tv_sec, fib20_dt.tv_nsec);// Printing the Fib20 occurence from start time of event 
   }
}

//Function to Print the scheduler used 
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

//main of program 
int main (int argc, char *argv[])
{
   int rc, invSafe=0, i, scope;
   struct timespec sleepTime, dTime;
   abortTest = 0;
   CScount=0;
   
   printf("Main Started ");
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

   //Find the max and min priority 
   rt_max_prio = sched_get_priority_max(SCHED_FIFO);
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);
   rc=sched_getparam(getpid(), &nrt_param);
   main_param.sched_priority = rt_max_prio;
   rc=sched_setscheduler(getpid(), SCHED_FIFO, &main_param);

   if (rc)
   {
       printf("ERROR; sched_setscheduler rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }

   
   print_scheduler();//prints scheduling to be still SCHED_OTHER
   printf("min prio = %d, max prio = %d\n", rt_min_prio, rt_max_prio);
   rt_param[SEQ_SERVICE].sched_priority = rt_max_prio;//setting priority 99
   pthread_attr_setschedparam(&rt_sched_attr[SEQ_SERVICE], &rt_param[SEQ_SERVICE]);
   printf("Creating thread %d\n", SEQ_SERVICE);
   //Create the thread 0 and call start Service(sequencer Service)
   rc = pthread_create(&threads[SEQ_SERVICE], &rt_sched_attr[SEQ_SERVICE], startService, (void *)SEQ_SERVICE);
  
   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
 
   //Set the affinity of Sequencer(Start Service) to use only one core for all threads of // the process 
	int s;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
	s = pthread_setaffinity_np(threads[SEQ_SERVICE], sizeof(cpu_set_t), &cpuset);
    s = pthread_getaffinity_np(threads[SEQ_SERVICE], sizeof(cpu_set_t), &cpuset);
	
    if (CPU_ISSET(0, &cpuset))
         printf("CPU 0\n");
   //
   printf("Start services thread spawned\n");
   
   if(pthread_join(threads[SEQ_SERVICE], NULL) == 0)
     printf("SEQ_SERVICE done\n");
   else
     perror("SEQ_SERVICE");
   rc=sched_setscheduler(getpid(), SCHED_OTHER, &nrt_param);
   
   exit(0);
}

//Function Start Service which acts as a Sequencer /Scheduler 
//creates Fib10 and Fib20 threads 
void *startService(void *threadid)
{
   int rc;
   runInterference=intfTime;
   //create Semaphore
   sem_init(&semF10,0,0);//Initialize a Binary semF10
   sem_init(&semF20,0,0);//Initialize a Binary semF20
  
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
   
   gettimeofday(&timeNow);
   printf("FIB20 Thread %d thread spawned at %d sec, %d nsec\n", FIB20_SERVICE, timeNow.tv_sec, timeNow.tv_nsec);
   rt_param[FIB10_SERVICE].sched_priority = rt_max_prio-10;//Set higher than Fib10 less than sequencer (89)
   pthread_attr_setschedparam(&rt_sched_attr[FIB10_SERVICE], &rt_param[FIB10_SERVICE]);
    //Creating Mid Priority thread and function called with no Mutex
   printf("Creating thread %d\n", FIB10_SERVICE);
   rc = pthread_create(&threads[FIB10_SERVICE], &rt_sched_attr[FIB10_SERVICE], fib10, (void *)FIB10_SERVICE);

   
   //Set the affinity of Fib10 and Fib20 to use only one core for all threads of // the process 
	int s;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);

	//FIB10
    s = pthread_setaffinity_np(threads[FIB10_SERVICE], sizeof(cpu_set_t), &cpuset);     
    s = pthread_getaffinity_np(threads[FIB10_SERVICE], sizeof(cpu_set_t), &cpuset);
	//FIB20
    s = pthread_setaffinity_np(threads[FIB20_SERVICE], sizeof(cpu_set_t), &cpuset);
    s = pthread_getaffinity_np(threads[FIB20_SERVICE], sizeof(cpu_set_t), &cpuset);
	
	if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }

   //sequence service event
    clock_gettime(CLOCK_REALTIME, &rtclk_start_time);//Capturing Start time of event 
	printf("Before Se post\n");
	print_scheduler();//
   //Critical Instant 
    sem_post(&semF10);
	sem_post(&semF20);




  while(!abortTest)
  {
  /* Sequencing loop for LCM phasing of Fib10, Fib20
   */
	usleep(20000);sem_post(&semF10);
	usleep(20000);sem_post(&semF10);
	usleep(10000);sem_post(&semF20);
	usleep(10000);sem_post(&semF10);
	usleep(20000);sem_post(&semF10);
	usleep(20000); 

	clock_gettime(CLOCK_REALTIME, &rtclk_stop_time);
	delta_t(&rtclk_stop_time, &rtclk_start_time, &rtclk_dt);//Capturing Stop time of event 
	printf("RT clock DT seconds = %ld, nanoseconds = %ld\n", rtclk_dt.tv_sec, rtclk_dt.tv_nsec);
	sem_post(&semF10);
	sem_post(&semF20);
	clock_gettime(CLOCK_REALTIME, &rtclk_start_time);
	fib20Cnt=0;
	fib10Cnt=0;
  }
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

   //Destroying Semaphore
   
   
   //Exiting Thread 
   pthread_exit(NULL);

}
