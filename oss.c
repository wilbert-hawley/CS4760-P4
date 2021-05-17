#include "utility.h"

// shared memeory variables
struct shmbuf *shmp;
int shmid;
int msqid1;
int msqid2;
int pid_arr[200];
int pid_num_total = 0;
void processKiller()
{
  int k = 0;
  for(; k < 200; k++) {
    if(pid_arr[k] == 0)
      break;
    kill(pid_arr[k], SIGTERM);
  }

  printf("oss: CPU idle time = %d.%d\n", shmp->rep.idle_sec, shmp->rep.idle_nanosec);
  printf("oss: pid_total_num = %d\n", pid_num_total);
  shmdt(shmp);
  msgctl(msqid1, IPC_RMID, NULL);
  msgctl(msqid2, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);
  exit(0);
}

int setupTimer(int time)
{
  struct itimerval value;

  value.it_interval.tv_sec = time;
  value.it_interval.tv_usec = 0;
  value.it_value = value.it_interval;

  return setitimer(ITIMER_REAL, &value, NULL);
}

void sig_alrm_handler()
{
  fprintf(stderr, "master:");
  perror("Time limit reached!");
  processKiller();
  exit(1);
}

int timerInterrupt()
{
  struct sigaction act;

  act.sa_handler = sig_alrm_handler;
  act.sa_flags = 0;

  return (sigemptyset(&act.sa_mask) || sigaction(SIGALRM, &act, NULL));
}

void sig_int_handler()
{
    fprintf(stderr, "master:");
    perror("ctrl-c interupt, all children terminated.\n");
    processKiller();
}


int start_ctrlc_int_handler()
{
  struct sigaction act;

  act.sa_handler = sig_int_handler;
  act.sa_flags = 0;

  return (sigemptyset(&act.sa_mask) || sigaction(SIGINT, &act, NULL));
}


int main(int argc, char** argv) {

  // initialize an array to hold child pid
  int m = 0;
  for(; m < 200; m++)
    pid_arr[m] = 0;

  // set up handlers
  if (setupTimer(10) == -1) {
    fprintf(stderr, "%s: ", argv[0]);
    perror("Failed to set up the ITIMER_REAL interval timer");
    exit(1);;
  }

  if (timerInterrupt() == -1) {
    fprintf(stderr, "%s: ", argv[0]);
    perror("Failed to set up the inturrept SIGALRM");
    exit(1);
  } 

  if (start_ctrlc_int_handler() == -1) {
    fprintf(stderr, "%s: ", argv[0]);
    perror("Failed to set up the SIGINT handler\n");
    exit(1);
  }

  // set up shared memory

  key_t sharedMemoryKey;
  if ((sharedMemoryKey = ftok("./utility.h", 0)) == ((key_t) - 1))
  {
    fprintf(stderr, "%s: ", argv[0]);
    perror("Error: Failed to derive key from utility.h\n");
    exit(EXIT_FAILURE);
  }
  
  if ((shmid = shmget(sharedMemoryKey, sizeof(struct shmbuf), IPC_CREAT | 0600)) == -1)
  {
    fprintf(stderr, "%s: ", argv[0]);
    perror("Error: Failed to create shared mem with key\n");
    exit(EXIT_FAILURE);
  }

  shmp = (struct shmbuf *)shmat(shmid, NULL, 0);

  shmp->sec = 0;
  shmp->nanosec = 0;

  // set up message queue:
  key_t messageKey1;
  if ((messageKey1 = ftok("./README", 0)) == ((key_t) - 1))
  {
    fprintf(stderr, "%s: ", argv[0]);
    perror("Error: Failed to derive key from README\n");
    exit(EXIT_FAILURE);
  }
  if ((msqid1 = msgget(messageKey1, IPC_CREAT | 0600 )) == -1)
  {
    perror("Error: Failed to create message queue\n");
    return EXIT_FAILURE;
  }

  key_t messageKey2;
  if ((messageKey2 = ftok("./utility.h", 0)) == ((key_t) - 1))
  {
    fprintf(stderr, "%s: ", argv[0]);
    perror("Error: Failed to derive key from README\n");
    exit(EXIT_FAILURE);
  }
  if ((msqid2 = msgget(messageKey1, IPC_CREAT | 0600 )) == -1)
  {
    perror("Error: Failed to create message queue\n");
    return EXIT_FAILURE;
  } 

  // messages from child to parent
  struct msgbuf msg1;
  // messages from parent to child
  struct msgbuf msg2;
 
  // setting up a timer to stop creating new processes
  time_t end;
  time_t start = time(0);
  time_t total = 1;
  end = start + total;

  // initializing report
  shmp->rep.idle_sec = 0;
  shmp->rep.idle_nanosec = 0;
 
  // vars to hold when to create new processes
  srand(time(0) * getpid());
  unsigned next_proc_sec = rand() % (maxTimeBetweenNewProcsSecs + 1);
  unsigned next_proc_nanosec = rand() % (maxTimeBetweenNewProcsNS + 1);
  // set up two queues
  struct Queue* ready_queue = createQueue(19);
  int blocked[19];
  bool stop = false;
  int i = 0;
  for(i = 0; i < 19; i++) {
    blocked[i] = -1;
    init_pcb(i, &shmp->pcb[i]);
  }
  int pcb_index = -1;
  i = 0;
  m = 0;
  int last = -1;
 
  // begin the main loop
  while(true) {
    // advance cloced by 1.xx seconds on each loop (xx between 0-1000ns)
   /* if(isEmpty(ready_queue)) {
      shmp->rep.idle_sec += next_proc_sec;
      shmp->rep.idle_nanosec += next_proc_nanosec;
      if(shmp->rep.idle_nanosec > 1000000000) {
        shmp->rep.idle_sec++;
        shmp->rep.idle_nanosec -= 1000000000;
      }
    }*/
   
    // incremement timer for overhead
    shmp->sec++;
    shmp->nanosec += rand() % 1001;
    if(shmp->nanosec > 1000000000) {
      shmp->sec += 1;
      shmp->nanosec -= 1000000000;
    }
 
    //printf("oss: Top of loop. Time: shmp->sec = %d, shmp->nanosec = %d\n", shmp->sec, shmp->nanosec);
    //printf("oss: next_proc_sec = %d, next_proc_nanosec = %d\n", next_proc_sec, next_proc_nanosec);
    //printf("oss: i = %d ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", i);
   
    start = time(NULL);

    if(i > 30 || start > end) { 
     stop = true;
      if(isEmpty(ready_queue) && empty_blocked_queue(blocked)) {
        break;
      }
    }

    if(shmp->sec >= next_proc_sec && shmp->nanosec >= next_proc_nanosec && !stop) {
      printf("oss: Attempting to create next process. Time: shmp->sec = %d, shmp->nanosec = %d\n", shmp->sec, shmp->nanosec);
      // set time until next process
      next_proc_sec = shmp->sec + (rand() % (maxTimeBetweenNewProcsSecs + 1));
      next_proc_nanosec = shmp->nanosec + (rand() % (maxTimeBetweenNewProcsNS + 1));
      if(next_proc_nanosec > 1000000000) {
        next_proc_sec += 1;
        next_proc_nanosec -= 1000000000;
      }

      // find an open spot on the pcb
      if(i < 19) {
        pcb_index = i;
        i++;
      }
      else {
        pcb_index = -1;
        int j;
        for(j = 0; j < 19; j++) {
          if(!shmp->pcb[j].full) {
            if(last == j)
              continue;
            pcb_index = j;
            last = j;
            i++;
	    shmp->pcb[pcb_index].full = true;
	    break;
          }
        }
      }

      // if there is an available spot in the pcb, dispatch a new process
      if(pcb_index != -1) {
        printf("FORK\n");
        pid_num_total++;
        int child = fork();
        switch(child) {
          case -1:
            printf("Failed to fork\n");
            processKiller();
            //stop = true;
          case 0: ;
            char num2[50];
            sprintf(num2, "%d", pcb_index);
            execl("./child_proc", "./child_proc", num2, NULL);
          default:
            pid_arr[m] = child;
            m++;
            msgrcv(msqid1, &msg1, sizeof(msg1), getpid(), 0);
            shmp->nanosec += 500000;
            if(shmp->nanosec > 1000000000) {
              shmp->sec += 1;
              shmp->nanosec -= 1000000000;
            }
            printf("oss: Child %d placed on queue\n", pcb_index);
            enqueue(ready_queue, pcb_index);
            // adnvace clock between 100 to 10000ns when dispatching
            shmp->nanosec += (rand() % 9900) + 100;
            if(shmp->nanosec > 1000000000) {
              shmp->sec += 1;
              shmp->nanosec -= 1000000000;
            }
        }
      }
      else
        printf("OSS: Process table is full. Skipped forking.");
    }

    // check blocked queue, place any expired onto que
    int l;
    for(l = 0; l < 19; l++) {
      if(blocked[l] != -1) {
        int ind = blocked[l];
        if(shmp->sec >= shmp->pcb[ind].block_sec) {
          if(shmp->nanosec >= shmp->pcb[ind].block_nanosec) {
            printf("oss: Child %d in block[%d] moving from blocked to ready queue\n", ind, l);
            enqueue(ready_queue, ind);
            shmp->pcb[ind].blocked = false;
            blocked[l] = -1;
            // adding to clock from transitions from blocked to ready queue
            shmp->nanosec += 1500000;
            if(shmp->nanosec > 1000000000) {
              shmp->sec += 1;
              shmp->nanosec -= 1000000000;
            }
          } 
        }
      }
    }
    // add time to clock for going through blocked queue
    shmp->nanosec += 200000;
    if(shmp->nanosec > 1000000000) {
      shmp->sec += 1;
      shmp->nanosec -= 1000000000;
    }
    //int cur_proc; 
    if(!isEmpty(ready_queue)) {
      // deque from the ready queue and run process by sending message
      const int cur_proc = dequeue(ready_queue);
      //int temp = cur_proc;
      //printf("OSS: ~~~~~~~~~~~~~cur_proc = %d after dequeueing\n", cur_proc);
      //printf("oss: Child %d being dequeued from ready queue, message send\n", cur_proc);
      msg2.mtype = shmp->pcb[cur_proc].real_pid;
      msg2.mi.sec = 0;
      msg2.mi.nanosec = 0;
      printf("oss: Child %d being dequeued from ready queue, message send\n", cur_proc);
      msgsnd(msqid2, &msg2, sizeof(msg2), 0);
      // wait for process to send message back
      fprintf(stderr, "oss: Waiting for Child %d\n", cur_proc);
      msgrcv(msqid1, &msg1, sizeof(msg1), getpid(), 0);
      //cur_proc = temp;
      //cur_proc = msg1.mi.local_pid;
      printf("OSS: ~~~~~~~~~~~~~cur_proc = %d after geting message back from child\n", cur_proc);
      shmp->sec += msg1.mi.sec;
      shmp->nanosec += msg1.mi.nanosec;
      if(shmp->nanosec > 1000000000) {
        shmp->sec += 1;
        shmp->nanosec -= 1000000000;
      }
      //fprintf(stderr, "oss: Waiting for Child sec = %d, nanosec = %d\n", shmp->sec, shmp->nanosec); 
      //printf("OSS: ~~~~~~~~~~~~~cur_proc = %d before checking for block\n", cur_proc);
      if(shmp->pcb[cur_proc].blocked) {
	  // place in blocked array
	printf("oss: Child %d was blocked and placed on blocked queue\n", cur_proc);
        int k;
	for(k = 0; k < 19; k++) {
          if(blocked[k] == -1) {
            blocked[k] = cur_proc;
            break;
          } 
        }
        shmp->nanosec += 150000;
        if(shmp->nanosec > 1000000000) {
          shmp->sec += 1;
          shmp->nanosec -= 1000000000;
        }
      }
      else if(shmp->pcb[cur_proc].done) {
       // printf("OSS: cur_proc = %d~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", cur_proc);
        printf("oss: Child %d is done, ran for %d.%d seconds.\n", cur_proc,
                shmp->pcb[cur_proc].total_cpu_sec, shmp->pcb[cur_proc].total_cpu_nanosec);
        shmp->pcb[cur_proc].full = false;
        shmp->nanosec += 5000;
        if(shmp->nanosec > 1000000000) {
          shmp->sec += 1;
          shmp->nanosec -= 1000000000;
        }
      }
      else {
        enqueue(ready_queue, cur_proc);
        shmp->nanosec += 10000;
        if(shmp->nanosec > 1000000000) {
          shmp->sec += 1;
          shmp->nanosec -= 1000000000;
        }
      }
    }

  }
  processKiller();
 
  // delete message queue;
  msgctl(msqid1, IPC_RMID, NULL);
  msgctl(msqid2, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);
  return 0;
}
