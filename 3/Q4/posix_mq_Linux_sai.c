#include <pthread.h>
#include <unistd.h> // for getpid() function[get process id number of the calling process]
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>

pthread_t sThread, rThread;
pthread_attr_t sThread_sched_attr, rThread_sched_attr; 
int rt_max_prio, rt_min_prio, min;
struct sched_param sThread_param, rThread_param;

#define SNDRCV_MQ "/send_receive_mq" // name of queue to open
#define MAX_MSG_SIZE 128
#define ERROR -1

static char canned_msg[] = "this is a test, and only a test, in the event of a real emergency, you would be instructed ...";

static struct mq_attr mq_attr_var; // message queue attributes variable


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

void* receiver(void* threadId){
  
  static mqd_t mymq; // message queue descriptor
  char buffer[MAX_MSG_SIZE]; // creating char buffer of size 128
  int prio;
  int nbytes;

  /* note that VxWorks does not deal with permissions? */
  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, 0666, &mq_attr_var);

  if(mymq == (mqd_t)ERROR)
    perror("mq_open");

  /* read oldest, highest priority msg from the message queue */
  if((nbytes = mq_receive(mymq, buffer, MAX_MSG_SIZE, &prio)) == ERROR) 
  // mq_receive returns The length of the selected message in bytes, otherwise -1 (ERROR).
  {
    perror("mq_receive");
  }
  else
  {
    buffer[nbytes] = '\0';
    printf("receive: msg %s received with priority = %d, length = %d\n",
           buffer, prio, nbytes);
  }
  printf("\r\nLeaving receiver\n");
}

void* sender(void* threadId){
  
  static mqd_t mymq;
  int prio;
  int nbytes;
 /* note that VxWorks does not deal with permissions? */
  mymq = mq_open(SNDRCV_MQ, O_RDWR, 0666, &mq_attr_var);

  if(mymq == (mqd_t)ERROR)
    perror("mq_open");

  /* send message with priority=30 */
  if((nbytes = mq_send(mymq, canned_msg, sizeof(canned_msg), 30)) == ERROR) // 30 is the priority of the message, A message with a higher numeric value for msgPrio is inserted before messages with a lower value. The value of msgPrio must be less than or equal to 31.
  // mq_send returns 0 (OK), otherwise -1 (ERROR).
  {
    perror("mq_send");
  }
  else
  {
    printf("send: message successfully sent\n");
  }
  printf("\r\nLeaving sender\n");
}

void main(void)
{
   int rc, scope;

   printf("Before adjustments to scheduling policy:\n");
   print_scheduler();

   pthread_attr_init(&sThread_sched_attr); 
   pthread_attr_setinheritsched(&sThread_sched_attr, PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&sThread_sched_attr, SCHED_FIFO); 
   
   pthread_attr_init(&rThread_sched_attr); 
   pthread_attr_setinheritsched(&rThread_sched_attr, PTHREAD_EXPLICIT_SCHED);
   pthread_attr_setschedpolicy(&rThread_sched_attr, SCHED_FIFO);

   rt_max_prio = sched_get_priority_max(SCHED_FIFO); 
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);
   
   sThread_param.sched_priority = rt_max_prio - 5;
   pthread_attr_setschedparam(&sThread_sched_attr, &sThread_param);
   
   rThread_param.sched_priority = rt_max_prio;
   pthread_attr_setschedparam(&rThread_sched_attr, &rThread_param);

   rThread_param.sched_priority = rt_max_prio;
   rc=sched_setscheduler(getpid(), SCHED_FIFO, &rThread_param);

   if (rc)
   {
       printf("ERROR; sched_setscheduler rc is %d\n", rc);
       perror("sched_setschduler"); exit(-1); // prints the error message encountered by sched_setschduler
   }

   printf("After adjustments to scheduling policy:\n");
   print_scheduler();
   
   /* setup common message q attributes */
   mq_attr_var.mq_maxmsg = 100; // Maximum number of messages in the message queue
   mq_attr_var.mq_msgsize = MAX_MSG_SIZE; //Maximum message size
   mq_attr_var.mq_flags = 0; // Flags = 0 or O_NONBLOCK

   rc = pthread_create(&rThread, &rThread_sched_attr, receiver, (void *)1);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror("pthread_create");
       exit(-1);
   }
   
   rc = pthread_create(&sThread, &sThread_sched_attr, sender, (void *)0);

   if (rc)
   {
       printf("ERROR; pthread_create() rc is %d\n", rc);
       perror("pthread_create");
       exit(-1);
   }

   pthread_join(rThread, NULL);
   pthread_join(sThread, NULL);

   /*if(pthread_attr_destroy(&main_sched_attr) != 0)
     perror("attr destroy");*/
}
