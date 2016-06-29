/*
Sam Siewert - 7/8/97
Compiled and tested for: Solaris 2.5
Re-tested on Linux 2.6

Updated by @Chinmay Shah and ported from  posix_mq.c(code by Prof Sam Siewert)
            referred posix_linux_demo (code by Prof Sam Siewert)


POSIX features used: queuing signals and semaphores

Reading of Msg queue
1) https://www.softprayog.in/programming/interprocess-communication-using-posix-message-queues-in-linux
2) http://man7.org/linux/man-pages/man7/mq_overview.7.html

Referring source code of Prof @SAM Siewart
3)http://ecee.colorado.edu/~siewerts/extra/code/example_code_archive/a490dmis_code/POSIX2/pmq_receive.c
4)http://mercury.pr.erau.edu/~siewerts/cec450/code/POSIX-Examples/posix_mq.c

*** - Link with -lrt

//   Mounting the message queue file system
//       On  Linux,  message queues are created in a virtual file system.
//       (Other implementations may also provide such a feature, but  the
//       details  are likely to differ.)  This file system can be mounted
//       (by the superuser) using the following commands:
//
//    By mounting, while not required, you can get message queue status
//
//           # mkdir /dev/mqueue
//           # mount -t mqueue none /dev/mqueue
//
//     Do man mq_overview for more information
//


*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for getpid() function[get process id number of the calling process]
#include <signal.h>
#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <string.h>

//Included for msg queues
#include <mqueue.h> //message queue header
#include <fcntl.h> /* O_flags */
#include <sys/types.h>
#include <sys/stat.h>//mode constant


#define SNDRCV_MQ "/send_rxc_mq"
#define ERROR (-1)

#define MAX_MSG_SIZE 128

// Attribute for Message Queue declaration
static struct mq_attr mq_attr;
//static mqd_t mymq; //?


//Attribute for priority and scheduling 
pthread_attr_t main_sched_attr;
int rt_max_prio, rt_min_prio, min;
struct sched_param rx_param,tx_param;


pthread_t rx_thr_id,tx_thr_id;



static char canned_msg[] = "this is a test, and only a test, in the event of a real emergency, you would be instructed ...";



void print_scheduler(void)
{
   int schedType;

   //queries the scheduling policy currently applied to the process identified by pid.
   //If pid equals zero, the policy of the calling process will be retrieved
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
            //corrected
           printf("Pthread Policy is SCHED_RR\n");
           break;
     default:
       printf("Pthread Policy is UNKNOWN\n");
   }
}





/* receives pointer to heap, reads it, and deallocate heap memory */

void *receiver(void * R)
{
  char buffer[MAX_MSG_SIZE];
  static mqd_t mymq;
  int prio;
  int nbytes;

  /* note that VxWorks does not deal with permissions? */
  mymq = mq_open(SNDRCV_MQ, O_CREAT|O_RDWR, 0644, &mq_attr);

  if(mymq == (mqd_t)ERROR)
    perror("mq_open");

    /*******
    ssize_t mq_receive (mqd_t mqdes, char *msg_ptr, size_t msg_len,
                    unsigned int *msg_prio);

    receives a message from the queue referred by the descriptor mqdes. The
    oldest of the highest priority is deleted from the queue and passed to the process in the buffer pointed by msg_ptr.
    msg_len is the length of buffer in bytes and it must be greater than the maximum message size, the mq_msgsize attribute, for the queue


    On success, mq_receive returns the number of bytes received in the buffer pointed by msg_ptr.

    ***/


/*
    if((nbytes = mq_receive(mymq, (void *)&buffptr, (size_t)sizeof(void *), &prio)) == ERROR)
*/
     if((nbytes = mq_receive(mymq, buffer, MAX_MSG_SIZE, &prio)) == ERROR)
  {
    perror("mq_receive");
  }
  else
  {
    buffer[nbytes] = '\0';
    printf("receive: msg %s received with priority = %d, length = %d\n",
           buffer, prio, nbytes);
  }
    
}







void *sender(void * S)
{
  static mqd_t mymq;
  int prio;
  int nbytes;

  
  mymq = mq_open(SNDRCV_MQ, O_RDWR, 0644, &mq_attr);

  if(mymq == (mqd_t)ERROR)
    perror("mq_open");

  /* send message with priority=30 */
  if((nbytes = mq_send(mymq, canned_msg, sizeof(canned_msg), 30)) == ERROR)
  {
    perror("mq_send");
  }
  else
  {
    printf("send: message successfully sent\n");
  }
  
}


static int sid, rid;

//Main of program
main() {


  static mqd_t mymq;
  print_scheduler();

  /***************** setup common message q attributes
  Initialize
  struct mq_attr {
          long    mq_flags;       // message queue flags
          long    mq_maxmsg;      // maximum number of messages
          long    mq_msgsize;     // maximum message size
          long    mq_curmsgs;     // number of messages currently queued
          long    __reserved[4];  // ignored for input, zeroed for output
  */

  mq_attr.mq_maxmsg = 100;
  mq_attr.mq_msgsize = MAX_MSG_SIZE;
  mq_attr.mq_flags = 0;


  //assigning priority 
   pthread_attr_setinheritsched(&main_sched_attr, PTHREAD_EXPLICIT_SCHED);

   pthread_attr_setschedpolicy(&main_sched_attr, SCHED_FIFO);

   rt_max_prio = sched_get_priority_max(SCHED_FIFO);
   printf("rt_max_prio value %d\n", rt_max_prio);
   rt_min_prio = sched_get_priority_min(SCHED_FIFO);
   printf("rt_min_prio value %d\n", rt_min_prio);

  
   rx_param.sched_priority = rt_max_prio;
   tx_param.sched_priority = rt_max_prio-10;
   /**************
   int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                          void *(*start_routine) (void *), void *arg);


   */
   
   sched_setscheduler(getpid(), SCHED_FIFO, &rx_param);
   printf("\r\nAfter updating scheduler:\r\n");
   print_scheduler();
   pthread_attr_setschedparam(&main_sched_attr, &rx_param);

   pthread_create(&rx_thr_id, &main_sched_attr, receiver, NULL);
   
  /*if(pthread_create(&rx_thr_id, &main_sched_attr, receiver, NULL)) {
    perror("creating Rxv  thread\n");
    exit(-1);
   }
  else {
   //  make process wait on idle thread to do pthread_exit
      pthread_join(rx_thr_id, NULL);

      printf("RxV thread exited, process exiting\n");
      fflush(stdout);

      exit(0);

    }*/
  
  pthread_attr_setschedparam(&main_sched_attr, &tx_param);

  pthread_create(&tx_thr_id, &main_sched_attr, sender, NULL);
    //Creating thread for sender
  /*if(pthread_create(&tx_thr_id, &main_sched_attr, sender, NULL)) {
    perror("creating Sender  thread\n");
    exit(-1);
   }
  else {
      // make process wait on idle thread to do pthread_exit 
      pthread_join(tx_thr_id, NULL);

      printf("Tx thread exited, process exiting\n");
      fflush(stdout);

      exit(0);

    }*/

  
  pthread_join(rx_thr_id, NULL);
  pthread_join(tx_thr_id, NULL);
	
  printf ("Rxv and Tx task/process has been spawned\n");

  fflush(stdout);
  //close message queue
  //rc = ;
  
  if(mq_close(mymq) == ERROR)
  {
    perror("receiver mq_close");
    exit(-1);
  }
  printf ("Mqueue closed\n");	

}
