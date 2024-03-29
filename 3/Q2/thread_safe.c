/******************************************************************

Program to code and demonstratete usage of mutexlock for creating a 
Thread safe code section for update of a complex safe with a timestamp 


Two threads are been created :
Thread one 		- update X,Y,Z co-ordinates and other attributes at a time
(WRITE_SERVICE) 
				- for analysis purposes X,Y,Z increasing for different 
				dummy logic till 360 and reinitializing themselves
	
Thread two 		- Read this value of when able to access this resources 
(READ_SERVICE) 	  Both Threads have been assigned same priority


Code used in following while demonstration  
Case 1) #define MUTEX_LOCK - to allow locking and unlocking  
		of mutex msgSem and allowing thread safe update and read 	
Case 2) if not defined  mutex lock and unlock is disabled 
	


Note:
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

#define ERROR -1

#define FIB_ITERATIONS 2000


#define MUTEX_LOCK 
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


   
//Function /THread service to update navigational values with timestamp
// print value at updation 
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
		  #ifdef MUTEX_LOCK
			pthread_mutex_lock(&msgSem);
		  #endif	
		  
		  clock_gettime(CLOCK_REALTIME, &ng_state.timeNow);
		  clock_gettime(CLOCK_REALTIME, &timeWrite);
		  
		  
		  //gettimeofday(&ng_state.timeNow);//Time
		  //gettimeofday(&timeWrite);
		  
		  //printf("**** %d Before Updating Navigation values , %d nsec\n", (int)threadid, 
		  printf("****@WRITE:Before update  %d secs %d nsec Values \n\t X=%lf deg Y=%lf deg Z=%lf deg \n\t acc=%lf \n\t yaw=%lf roll=%lf pitch=%lf \n", ng_state.timeNow.tv_sec, ng_state.timeNow.tv_nsec,ng_state.X,ng_state.Y,ng_state.Z,ng_state.acc,ng_state.yaw,ng_state.roll,ng_state.pitch);
		  if (ng_state.X>=MAX_VALUE)
		  {
			  ng_state.X=0;
		  }
		  else
		  {
			  ///delay 
			  FIB_TEST(seqIterations,FIB_ITERATIONS);//approx 10ms/10
			  ng_state.X++;  
		  }	  
		  if (ng_state.Y>=MAX_VALUE)
		  {
			  ng_state.Y=0;
		  }
		  else
		  {
			  ///delay 
			  FIB_TEST(seqIterations,FIB_ITERATIONS);//approx 10ms/10
			  ng_state.Y+=2;  
		  }	  
		  
		  
		  if (ng_state.Z>=MAX_VALUE)
		  {
			  ng_state.Z=0;
		  }
		  else
		  {
			  ///delay 
			  FIB_TEST(seqIterations,FIB_ITERATIONS);//approx 10ms/10
			  ng_state.Z+=5;  
		  }	  
		  
		  if (ng_state.X>=150)
		  {
			  ng_state.yaw=0;
		  }
		  else
		  {
			  ///delay 
			  FIB_TEST(seqIterations,FIB_ITERATIONS);//approx 10ms/10
			  ng_state.yaw+=4;  
		  }	  
		  
		  if (ng_state.Y>=200)
		  {
			  ng_state.roll=0;
			  ng_state.acc=0;
		  }
		  else
		  {
			  ///delay 
			  FIB_TEST(seqIterations,FIB_ITERATIONS);//approx 10ms/10
			  ng_state.acc+=10;  
		  }	  
		  
		  
		  if (ng_state.Z>=250)
		  {
			  ng_state.pitch=0;			
		  }
		  else
		  {
			  ///delay 
			  FIB_TEST(seqIterations,FIB_ITERATIONS);//approx 10ms/10
			  ng_state.pitch+=20;			
		  }	    
		  
		  
		  #ifdef MUTEX_LOCK
			pthread_mutex_unlock(&msgSem);//unlock if locked
		  #endif
		  
		  
		  clock_gettime(CLOCK_REALTIME, &timeWrite);
		  
		  //gettimeofday(&timeWrite);//Time
		  printf("****@WRITE:After update  %d secs %d nsec Values \n\t X=%lf deg Y=%lf deg Z=%lf deg \n\t acc=%lf \n\t yaw=%lf roll=%lf pitch=%lf \n", ng_state.timeNow.tv_sec, ng_state.timeNow.tv_nsec,ng_state.X,ng_state.Y,ng_state.Z,ng_state.acc,ng_state.yaw,ng_state.roll,ng_state.pitch);
		  
		  
	}	  

		
		  pthread_exit(NULL);
}


//Function /THread service to read and print navigational values with timestamp
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
	  
	  //Default init ??
	  #ifdef MUTEX_LOCK
		pthread_mutex_lock(&msgSem);
	  #endif
		  clock_gettime(CLOCK_REALTIME, &timeRead);
		  //gettimeofday(&timeRead);
		  printf("****@READ: TimeStamp %d secs %d nsec\n",timeRead.tv_sec, timeRead.tv_nsec);  
		  printf("****@READ:At Write TimeStamp %d secs %d nsec Values \n\t X=%lf deg Y=%lf deg Z=%lf deg \n\t acc=%lf \n\t yaw=%lf roll=%lf pitch=%lf \n ", ng_state.timeNow.tv_sec, ng_state.timeNow.tv_nsec,ng_state.X,ng_state.Y,ng_state.Z,ng_state.acc,ng_state.yaw,ng_state.roll,ng_state.pitch);
	  #ifdef MUTEX_LOCK
			pthread_mutex_unlock(&msgSem);//unlock if locked
	  #endif
  }
	  pthread_exit(NULL);
  	  
}


//Function to print scheduler policy
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

  
   //printing the execution time between creation of threads
   //Schedule policy is SCHED_OTHER at present
   print_scheduler();

   //Assign Attribute, scheduling policy - SCHED_FIFO
   // To three threads 
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

// Function to create two threads for reading and writing Navigational data 
void *startService(void *threadid)
{
   struct timespec timeNow;
   int rc;

   struct timespec rtclk_resolution;
   
   //Initial dummy values for navigational data 
	ng_state.X=100.0;
	ng_state.Y=50.0;
	ng_state.Z=180.0;
	ng_state.yaw=0.0;
	ng_state.pitch=10.0;
	ng_state.roll=20.0;
	
	ng_state.timeNow.tv_nsec =0.0;
	ng_state.timeNow.tv_sec =0.0;
	   
  //clock_getres() finds the resolution (precision) of the specified clock clk_id,
  //and, if res is non-NULL, stores it in the struct timespec pointed to by res
  if(clock_getres(CLOCK_REALTIME, &rtclk_resolution) == ERROR)
  {
      perror("clock_getres");
      exit(-1);
  }
  else
  {
      printf("\n\nPOSIX Clock demo using system RT clock with resolution:\n\t%ld secs, %ld microsecs, %ld nanosecs\n", rtclk_resolution.tv_sec, (rtclk_resolution.tv_nsec/1000), rtclk_resolution.tv_nsec);
  }


  
  
	//Creating Write Service and updating its parameters
   rt_param[WRITE_SERVICE].sched_priority = rt_max_prio-10;
   pthread_attr_setschedparam(&rt_sched_attr[WRITE_SERVICE], &rt_param[WRITE_SERVICE]);
   
  
   //printf("Creating Write thread %d\n", WRITE_SERVICE);
   rc = pthread_create(&threads[WRITE_SERVICE], &rt_sched_attr[WRITE_SERVICE], Write, (void *)WRITE_SERVICE);
 
	//error check 
   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   
   
   gettimeofday(&timeNow);
   printf("****Write Service thread spawned(Higher priority) at %d sec, %d nsec\n",timeNow.tv_sec, timeNow.tv_nsec);

	
   //sleep(1);
	
	
   rt_param[READ_SERVICE].sched_priority = rt_max_prio-20;//Set priority of Low Priority Thread for Read
   pthread_attr_setschedparam(&rt_sched_attr[READ_SERVICE], &rt_param[READ_SERVICE]);//Set Schedule paramater of this thread
   
   //printf("Creating Read thread %d\n", READ_SERVICE);
   //create this low priority thread for Read Service 
   rc = pthread_create(&threads[READ_SERVICE], &rt_sched_attr[READ_SERVICE], Read, (void *)READ_SERVICE);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror(NULL);
       exit(-1);
   }
   
   
   gettimeofday(&timeNow);
   printf("****Read Service thread spawned(Lower Priority) at %d sec, %d nsec\n",timeNow.tv_sec, timeNow.tv_nsec);

  
   //Joining Threads at completion of threads 
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
