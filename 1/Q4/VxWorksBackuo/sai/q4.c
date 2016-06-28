#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>
#define EXIT_FAILURE 0

#define NSEC_PER_SEC (1000000000)
#define DELAY_TICKS (1)
#define ERROR (-1)
#define OK (0)

static struct timespec sleep_time = {0, 0};
static struct timespec sleep_requested = {0, 0};
static struct timespec remaining_time = {0, 0};

static struct timespec fib10_dt = {0, 0};
static struct timespec fib10_start_time = {0, 0};
static struct timespec fib10_stop_time = {0, 0};

static struct timespec fib20_dt = {0, 0};
static struct timespec fib20_start_time = {0, 0};
static struct timespec fib20_stop_time = {0, 0};

static unsigned int sleep_count = 0;
int rt_max_prio, rt_min_prio, min;
struct sched_param seq_param,fib10_param,fib20_param;
pthread_attr_t seq_sched_attr,fib10_sched_attr,fib20_sched_attr; // Thread creation attribute => main_sched_attr

#define FIB_LIMIT_FOR_32_BIT 47
int abortTest = 0;
uint32_t idx = 0, jdx = 1;
uint32_t fib = 0, fib0 = 0, fib1 = 1;
uint32_t fib10Cnt=0, fib20Cnt=0;
uint32_t seqIterations = FIB_LIMIT_FOR_32_BIT;

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

sem_t semF10, semF20;
pthread_t threads[3]; 

void* fib10(void* x)
{
   while(!abortTest)
   {
	   
	   sem_wait(&semF10);
	   /* start time stamp */ 
	   clock_gettime(CLOCK_REALTIME, &fib10_start_time);
	   FIB_TEST(seqIterations, 170000);
	   /* stop time stamp */ 
	   clock_gettime(CLOCK_REALTIME, &fib10_stop_time);
	   delta_t(&fib10_stop_time, &fib10_start_time, &fib10_dt);
	   fib10Cnt++;		
	   printf("In fib10: fib10Cnt:%d, seconds = %ld, nanoseconds = %ld\n", fib10Cnt, fib10_dt.tv_sec, fib10_dt.tv_nsec);
   }
}

void print_scheduler(void)
{
   int schedType;

   schedType = sched_getscheduler(getpid()); // sched_getscheduler() returns the current scheduling policy of the thread identified by pid

   // schedType value in Host Ubuntu = 0 i.e. SCHED_OTHER
   
   switch(schedType)
   {
     case SCHED_FIFO:  //a first-in, first-out policy; Value in Host Ubuntu = 1
           printf("Pthread Policy is SCHED_FIFO\n");
           break;
     case SCHED_OTHER: //the standard round-robin time-sharing policy; Value in Host Ubuntu = 0
           printf("Pthread Policy is SCHED_OTHER\n");
       break;
     case SCHED_RR:	   //a round-robin policy; Value in Host Ubuntu = 2
           printf("Pthread Policy is SCHED_OTHER\n");
           break;
     default:
       printf("Pthread Policy is UNKNOWN\n");
   }
}

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
  else
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


void* fib20(void* x)
{
   while(!abortTest)
   {
	   
	   sem_wait(&semF20);
	   /* start time stamp */ 
	   clock_gettime(CLOCK_REALTIME, &fib20_start_time);
	   FIB_TEST(seqIterations, 340000);
	   /* stop time stamp */ 
	   clock_gettime(CLOCK_REALTIME, &fib20_stop_time);
	   delta_t(&fib20_stop_time, &fib20_start_time, &fib20_dt);
	   fib20Cnt++;		
	   printf("In fib20: fib20Cnt:%d, seconds = %ld, nanoseconds = %ld\n", fib20Cnt, fib20_dt.tv_sec, fib20_dt.tv_nsec);
   }
}

void shutdown(void)
{
	abortTest=1;
}



void* Sequencer(void* x)
{

  printf("Starting Sequencer\n");
  int rvalue=0;

  /* Just to be sure we have 1 msec tick and TOs */
 // sysClkRateSet(1000);
		if( (rvalue=pthread_create(&threads[1], &fib10_sched_attr, fib10, NULL)) != 0)
		{
			printf("Pthread creation failed, return code: %d\r\n",rvalue);
			return EXIT_FAILURE;
		}
		
		
		if( (rvalue=pthread_create(&threads[2], &fib20_sched_attr, fib20, NULL)) != 0)
		{
			printf("Pthread creation failed, return code: %d\r\n",rvalue);
			return EXIT_FAILURE;
		}

		printf("Functions Created\r\n");
   

  /* Simulate the C.I. for S1 and S2 and mark on windview and log
     wvEvent first because F10 and F20 can preempt this task!
   */
  /*if(wvEvent(0xC, ciMarker, sizeof(ciMarker)) == ERROR)
	  printf("WV EVENT ERROR\n");
  semGive(semF10); semGive(semF20);*/
  
  /* start time stamp */ 
  clock_gettime(CLOCK_REALTIME, &rtclk_start_time);
  printf("StartTimeEvent:\n");
  sem_post(&semF10);
  sem_post(&semF20);

  /* Sequencing loop for LCM phasing of S1, S2
   */
  while(!abortTest)
  {
	//abortTest = 1;
	  /* Basic sequence of releases after CI */
      usleep(20000); sem_post(&semF10);	
      usleep(20000); sem_post(&semF10);
      usleep(10000); sem_post(&semF20);
      usleep(10000); sem_post(&semF10);
      usleep(20000); sem_post(&semF10);
      usleep(20000);
	  
	  /* back to C.I. conditions, log event first due to preemption */
	/*  if(wvEvent(0xC, ciMarker, sizeof(ciMarker)) == ERROR)
		  printf("WV EVENT ERROR\n");
	  sem_post(&semF10); sem_post(&semF20);*/
  
	  printf("StopTimeEvent:\n");
	  clock_gettime(CLOCK_REALTIME, &rtclk_stop_time);
	  delta_t(&rtclk_stop_time, &rtclk_start_time, &rtclk_dt);
	  
	  printf("RT clock DT seconds = %ld, nanoseconds = %ld\n", rtclk_dt.tv_sec, rtclk_dt.tv_nsec);
	  /* start time stamp */ 
	  printf("StartTimeEvent:\n");
	  clock_gettime(CLOCK_REALTIME, &rtclk_start_time);
	  fib10Cnt = 0;
	  fib20Cnt = 0;
	  sem_post(&semF10);
	  sem_post(&semF20);
  }  
 
		pthread_join(threads[1],NULL);
		pthread_join(threads[2],NULL);
 
 
 
}


int main(void)
{
  	int rc;
	abortTest=0;
	int rvalue=0;
	sem_init (&semF10,0,0);
	sem_init(&semF20,0,0);
	print_scheduler();
	
	pthread_attr_init(&seq_sched_attr);
	pthread_attr_init(&fib10_sched_attr);
	pthread_attr_init(&fib20_sched_attr);
 
   pthread_attr_setinheritsched(&seq_sched_attr, PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setinheritsched(&fib10_sched_attr, PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setinheritsched(&fib20_sched_attr, PTHREAD_EXPLICIT_SCHED);

   pthread_attr_setschedpolicy(&seq_sched_attr, SCHED_FIFO); 
   pthread_attr_setschedpolicy(&fib10_sched_attr, SCHED_FIFO); 
   pthread_attr_setschedpolicy(&fib20_sched_attr, SCHED_FIFO); 

   rt_max_prio = sched_get_priority_max(SCHED_FIFO); 
   rt_min_prio = sched_get_priority_min(SCHED_FIFO); 

   seq_param.sched_priority = rt_max_prio;
   rc=sched_setscheduler(getpid(), SCHED_FIFO, &seq_param);
   
      if (rc)
   {
       printf("ERROR; sched_setscheduler rc is %d\n", rc);
       perror("sched_setschduler"); exit(-1); // prints the error message encountered by sched_setschduler
   }

   printf("After adjustments to scheduling policy:\n");
   print_scheduler();

   seq_param.sched_priority = rt_max_prio;
   pthread_attr_setschedparam(&seq_sched_attr, &seq_param);

   fib10_param.sched_priority = rt_max_prio-5;
   pthread_attr_setschedparam(&fib10_sched_attr, &fib10_param);

   fib20_param.sched_priority = rt_max_prio-10;
   pthread_attr_setschedparam(&fib20_sched_attr, &fib20_param);
	
	/*if(taskSpawn("Sequencer", 20, 0, 8192, Sequencer, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0) == ERROR)
	{
	  printf("Sequencer task spawn failed\n");
	}
	else
	  printf("Sequencer task spawned\n");*/
  	if( (rvalue=pthread_create(&threads[0], &seq_sched_attr, Sequencer, NULL)) != 0)
	{
		printf("Pthread creation failed, return code: %d\r\n",rvalue);
		return EXIT_FAILURE;
	}
		
		

		
		
		pthread_join(threads[0],NULL);

		return 0;
  
  
  
  

}
