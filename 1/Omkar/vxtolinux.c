#define _GNU_SOURCE
#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include <string.h>
#include <stdint.h>

#define NSEC_PER_SEC (1000000000)
#define DELAY_TICKS (1)
#define OK (0)
#define FIB_LIMIT_FOR_32_BIT 47
#define serviceSeq 0
#define service_F10 1
#define service_F20 2
#define tenMs		1100000
#define twentyMs	2300000

static struct timespec service_F10_delayTime = {0, 0};
static struct timespec service_F10_start_time = {0, 0};
static struct timespec service_F10_stop_time = {0, 0};

static struct timespec service_F20_delayTime = {0, 0};
static struct timespec service_F20_start_time = {0, 0};
static struct timespec service_F20_stop_time = {0, 0};

static struct timespec rtclk_delayTime = {0, 0};
static struct timespec rtclk_start_time = {0, 0};
static struct timespec rtclk_stop_time = {0, 0};

int rt_max_prio, rt_min_prio;
struct sched_param seq_param, service_F10_param, service_F20_param;

pthread_t threads[3];
pthread_attr_t seq_sched_attr, service_F10_sched_attr, service_F20_sched_attr;

sem_t semF10, semF20;

int abortTest = 0;
uint32_t idx = 0, jdx = 1;
uint32_t fib = 0, fib0 = 0, fib1 = 1;
uint32_t fib10Cnt=0, fib20Cnt=0;
uint32_t seqIterations = FIB_LIMIT_FOR_32_BIT;

cpu_set_t cpuset; // Declaring CPUSet to be used by the threads

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

// fib10 is the start function for service_F10 thread
void* fib10(void* x)
{
	while(!abortTest)
	{
	   sem_wait(&semF10);
	   /* start time stamp */ 
	   clock_gettime(CLOCK_REALTIME, &service_F10_start_time);
	   FIB_TEST(seqIterations, tenMs);
	   /* stop time stamp */ 
	   clock_gettime(CLOCK_REALTIME, &service_F10_stop_time);
	   delta_t(&service_F10_stop_time, &rtclk_start_time, &service_F10_delayTime);
	   fib10Cnt++;		
	   printf("service_F10 Completion Time: sec = %ld, nanosec = %ld\n", service_F10_delayTime.tv_sec, service_F10_delayTime.tv_nsec);
	}
}

// fib20 is the start function for service_F20 thread
void* fib20(void* x)
{
	while(!abortTest)
	{
	   sem_wait(&semF20);
	   /* start time stamp */ 
	   clock_gettime(CLOCK_REALTIME, &service_F20_start_time);
	   FIB_TEST(seqIterations, twentyMs);
	   /* stop time stamp */ 
	   clock_gettime(CLOCK_REALTIME, &service_F20_stop_time);
	   delta_t(&service_F20_stop_time, &rtclk_start_time, &service_F20_delayTime);
	   fib20Cnt++;		
	    printf("service_F20 Completion Time: sec = %ld, nanosec = %ld\n", service_F20_delayTime.tv_sec, service_F20_delayTime.tv_nsec);
	}
}  

// print_scheduler function to print the current scheduling policy
void print_scheduler(void)
{
	int schedType;
	schedType = sched_getscheduler(getpid());   
	switch(schedType)
	{
		case SCHED_FIFO: 									// FIFO
			printf("Pthread Policy is SCHED_FIFO\n");
			break;
		case SCHED_RR:										// Round Robin
			printf("Pthread Policy is SCHED_RR\n");
			break;
		case SCHED_OTHER:									// Other
			printf("Pthread Policy is SCHED_OTHER\n");
			break;
		default:
			printf("Pthread Policy is UNKNOWN\n");
	}
}

// delta_t function to get the delay time between two different timestamps
int delta_t(struct timespec *stop, struct timespec *start, struct timespec *delta_t)
{
	int dt_sec=stop->tv_sec - start->tv_sec;
	int dt_nsec=stop->tv_nsec - start->tv_nsec;

	if(dt_sec >= 0)
	{
		if(dt_nsec >= 0)
		{
		  //printf("***Case1\n");
		  delta_t->tv_sec=dt_sec;
		  delta_t->tv_nsec=dt_nsec;
		}
		else
		{
		  //printf("***Case2\n");
		  delta_t->tv_sec=dt_sec-1;
		  delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
		}
	}
	else
	{
		if(dt_nsec >= 0)
		{
		  //printf("***Case3\n");
		  delta_t->tv_sec=dt_sec;
		  delta_t->tv_nsec=dt_nsec;
		}
		else
		{
		  //printf("***Case4\n");
		  delta_t->tv_sec=dt_sec-1;
		  delta_t->tv_nsec=NSEC_PER_SEC+dt_nsec;
		}
	}

	return(OK);
}

// shutdown function for terminating the LCM Invariant Schedule
void shutdown(void)
{
	abortTest=1;
}

// Sequencer is the start function for Sequencer thread
void* Sequencer(void* x)
{
	printf("Starting Sequencer\n");
	int rc;

	//Initializing Semaphores semF10, semF20
	sem_init (&semF10,0,0);
	sem_init(&semF20,0,0);
	
	rc = pthread_create(&threads[service_F10], &service_F10_sched_attr, fib10, NULL);

  	if(rc)
	{
	   printf("ERROR; pthread_create() for service_F10, rc is %d\n", rc);
       perror(NULL);
       exit(-1);
	}
	
	// Setting afinity for service_F10 Thread to limit it's execution to only CPU 0
	rc = pthread_setaffinity_np(threads[service_F10],sizeof(cpu_set_t),&cpuset);
	
	if(rc){
	   printf("ERROR; setaffinity for service_F10 is %d\n", rc);
       perror(NULL);
       exit(-1);
	}

	
	rc = pthread_create(&threads[service_F20], &service_F20_sched_attr, fib20, NULL);

  	if(rc)
	{
	   printf("ERROR; pthread_create() for service_F20, rc is %d\n", rc);
       perror(NULL);
       exit(-1);
	}

	printf("service_F10, service_F20 Created\r\n");
	
	// Setting afinity for service_F20 Thread to limit it's execution to only CPU 0
	rc = pthread_setaffinity_np(threads[service_F20],sizeof(cpu_set_t),&cpuset);
	
	if(rc){
	   printf("ERROR; setaffinity for service_F20 is %d\n", rc);
       perror(NULL);
       exit(-1);
	}
	
	/* start time stamp */ 
	clock_gettime(CLOCK_REALTIME, &rtclk_start_time);
	printf("===StartTimeEvent Logging===\n");
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

	  printf("===StopTimeEvent Logging===\n\n");
	  /* start time stamp */ 
	  clock_gettime(CLOCK_REALTIME, &rtclk_stop_time);
	  delta_t(&rtclk_stop_time, &rtclk_start_time, &rtclk_delayTime);
	  
	  printf("LCM Invariant Schedule Time = %ld, nanoseconds = %ld", rtclk_delayTime.tv_sec, rtclk_delayTime.tv_nsec);
	  /* start time stamp */ 
	  printf("\n===StartTimeEvent Logging===\n");
	  clock_gettime(CLOCK_REALTIME, &rtclk_start_time);
	  fib10Cnt = 0;
	  fib20Cnt = 0;
	  sem_post(&semF10);
	  sem_post(&semF20);
	}  
	 
	if(pthread_join(threads[service_F10], NULL) == 0)
     printf("service_F10 Service done\n");
	else
     perror("service_F10 Service:");
 
	if(pthread_join(threads[service_F20], NULL) == 0)
     printf("service_F20 Service done\n");
	else
     perror("service_F20 Service:");
  
}

// main function
int main(void)
{
	int rc;
	abortTest=0;

	// Printing the scheduling policy in the start
	print_scheduler();

	pthread_attr_init(&seq_sched_attr);
	pthread_attr_setinheritsched(&seq_sched_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&seq_sched_attr, SCHED_FIFO); 
	
	pthread_attr_init(&service_F10_sched_attr);
	pthread_attr_setinheritsched(&service_F10_sched_attr, PTHREAD_EXPLICIT_SCHED);	
	pthread_attr_setschedpolicy(&service_F10_sched_attr, SCHED_FIFO); 
	
	pthread_attr_init(&service_F20_sched_attr);
	pthread_attr_setinheritsched(&service_F20_sched_attr, PTHREAD_EXPLICIT_SCHED);
	pthread_attr_setschedpolicy(&service_F20_sched_attr, SCHED_FIFO); 
 
	rt_max_prio = sched_get_priority_max(SCHED_FIFO); 
	rt_min_prio = sched_get_priority_min(SCHED_FIFO); 

	seq_param.sched_priority = rt_max_prio;
	rc=sched_setscheduler(getpid(), SCHED_FIFO, &seq_param);

	if (rc)
	{
		printf("ERROR; sched_setscheduler rc is %d\n", rc);
		perror("sched_setschduler"); 
		exit(-1); // prints the error message encountered by sched_setschduler
	}

	printf("After adjustments to scheduling policy:\n");
	print_scheduler();

	seq_param.sched_priority = rt_max_prio;
	pthread_attr_setschedparam(&seq_sched_attr, &seq_param);

	service_F10_param.sched_priority = rt_max_prio-5;
	pthread_attr_setschedparam(&service_F10_sched_attr, &service_F10_param);

	service_F20_param.sched_priority = rt_max_prio-10;
	pthread_attr_setschedparam(&service_F20_sched_attr, &service_F20_param);
	
	rc = pthread_create(&threads[serviceSeq], &seq_sched_attr, Sequencer, NULL);

  	if(rc)
	{
	   printf("ERROR; pthread_create() for Sequencer Service, rc is %d\n", rc);
       perror(NULL);
       exit(-1);
	}
	
	
	CPU_ZERO(&cpuset); // Clearing the CPUSet
	CPU_SET(0, &cpuset);// Setting only CPU 0 to CPUSet
	
	// Setting afinity for Sequencer Thread to limit it's execution to only CPU 0
	rc = pthread_setaffinity_np(threads[serviceSeq],sizeof(cpu_set_t),&cpuset); 
	
	if(rc){
	   printf("ERROR; setaffinity for Sequencer is %d\n", rc);
       perror(NULL);
       exit(-1);
	}
		
	if(pthread_join(threads[serviceSeq], NULL) == 0)
     printf("Sequencer Service done\n");
	else
     perror("Sequencer Service:");
	
	return 0;
}