/************************************************************
Code by Prof Sam Seiwart for portraying a deadlock(circular wait)

updation of Code by chinmay.shah@colorado.edu


************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <sched.h>
#include <time.h>
#include <stdlib.h>

#define NUM_THREADS 2 //total number of threads
#define THREAD_1 1    //Thread one
#define THREAD_2 2    //Thread two

pthread_t threads[NUM_THREADS];
struct sched_param nrt_param;

pthread_mutex_t rsrcA, rsrcB;

volatile int rsrcACnt=0, rsrcBCnt=0, noWait=0;

/****************************


*****************************/
void *grabRsrcs(void *threadid)
{
   if((int)threadid == THREAD_1)
   {
     printf("THREAD 1 grabbing resources\n");
     pthread_mutex_lock(&rsrcA);// Acquiring resource lock on RsrcA
     rsrcACnt++;

     if(!noWait) usleep(1000000);

     printf("THREAD 1 got A, trying for B\n");
     pthread_mutex_lock(&rsrcB);
     rsrcBCnt++;

     printf("THREAD 1 got A and B\n");

     //Unlocking or unblocking mutex
     pthread_mutex_unlock(&rsrcB);
     pthread_mutex_unlock(&rsrcA);

     printf("THREAD 1 done\n");
   }
   else
   {

     printf("THREAD 2 grabbing resources\n");
	 usleep(1000000);
     pthread_mutex_lock(&rsrcB);
     rsrcBCnt++;
     if(!noWait) usleep(1000000);
     // The comment had a mistake of THread 1 ---> changing it to Thread 2

     printf("THREAD 2 got B, trying for A\n");

     pthread_mutex_lock(&rsrcA);
     rsrcACnt++;
     printf("THREAD 2 got B and A\n");

     pthread_mutex_unlock(&rsrcA);
     pthread_mutex_unlock(&rsrcB);
     printf("THREAD 2 done\n");

   }
   pthread_exit(NULL);
}

int main (int argc, char *argv[])
{
   int rc, safe=0;

   rsrcACnt=0, rsrcBCnt=0, noWait=0;

   if(argc < 2)
   {
     printf("Will set up unsafe deadlock scenario\n");
   }
   else if(argc == 2)
   {
     if(strncmp("safe", argv[1], 4) == 0)
       safe=1;
     else if(strncmp("race", argv[1], 4) == 0)
       noWait=1;
     else
       printf("Will set up unsafe deadlock scenario\n");
   }
   else
   {
     printf("Usage: deadlock [safe|race|unsafe]\n");
   }

   // Set default protocol for mutex
   pthread_mutex_init(&rsrcA, NULL);
   pthread_mutex_init(&rsrcB, NULL);

   printf("Creating thread %d\n", THREAD_1);

   //Start function/task for thread 1 and thread 2 is same
   rc = pthread_create(&threads[0], NULL, grabRsrcs, (void *)THREAD_1);

    //check if thread 1 is created or not
   if (rc) {
            printf("ERROR; pthread_create() rc is %d\n", rc);
            perror(NULL); exit(-1);
            }
   printf("Thread 1 spawned\n");

   ///Join thread if Safe condition before Thread two in progress
   if(safe) // Make sure Thread 1 finishes with both resources first
   {
     if(pthread_join(threads[0], NULL) == 0)
       printf("Thread 1: %d done\n", threads[0]);
     else
       perror("Thread 1");
   }

   printf("Creating thread %d\n", THREAD_2);

   //Start function/task for thread 2
   rc = pthread_create(&threads[1], NULL, grabRsrcs, (void *)THREAD_2);

   //check if thread 2 is created or not
   if (rc) {
        printf("ERROR; pthread_create() rc is %d\n", rc);
        perror(NULL); exit(-1);}

   printf("Thread 2 spawned\n");

   //Check present count of resources taken /blocked count
   printf("rsrcACnt=%d, rsrcBCnt=%d\n", rsrcACnt, rsrcBCnt);

   printf("will try to join CS threads unless they deadlock\n");

   ///Join thread if Nowait / other condition after Thread two in progress
   if(!safe)
   {
     if(pthread_join(threads[0], NULL) == 0)
       printf("Thread 1: %d done\n", threads[0]);
     else
       perror("Thread 1");
   }

   /*  The pthread_join() function waits for the thread specified by thread
       to terminate.  If that thread has already terminated, then
       pthread_join() returns immediately.  The thread specified by thread
       must be joinable.

       It can only join if the threads are terminated

    */
   if(pthread_join(threads[1], NULL) == 0)
     printf("Thread 2: %d done\n", threads[1]);
   else
     perror("Thread 2");


     //destroying Mutex
   if(pthread_mutex_destroy(&rsrcA) != 0)
     perror("mutex A destroy");

   if(pthread_mutex_destroy(&rsrcB) != 0)
     perror("mutex B destroy");

   printf("All done\n");

   exit(0);
}
