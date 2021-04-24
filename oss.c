#include "utility.h"

struct shmbuf *shmp;
int shmid;

int main(int argc, char** argv) {

  printf("Compiled in oss.c\n");

  struct msgbuf msg;
  msg.mi.local_pid = -1;
  msg.mi.sec = 0;
  msg.mi.nanosec = 0;

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
  key_t messageKey;
  if ((messageKey = ftok("./README", 0)) == ((key_t) - 1))
  {
    fprintf(stderr, "%s: ", argv[0]);
    perror("Error: Failed to derive key from README\n");
    exit(EXIT_FAILURE);
  }
  int msqid;
  if ((msqid = msgget(messageKey, IPC_CREAT | 0600 )) == -1)
  {
    perror("Error: Failed to create message queue\n");
    return EXIT_FAILURE;
  }
  
  //init_pcb(12, &shmp->pcb[0]);
  /*int i = 0;
  int queue[2];
  for(; i < 2; i++) {
    printf("i = %d\n", i );
    init_pcb(i, &shmp->pcb[i]);
    int child = fork();
    switch(child) {
      case -1:
        printf("Failed to fork\n");
        exit(1);
      case 0:
        execl("./child_proc", "./child_proc", NULL);
      default:
        queue[i] = child;
        // adding time for creating child
        shmp->nanosec += 10;
        // consider function to add to sec and subtract from nano
        // waiting for child to set up
        msgrcv(msqid, &msg, sizeof(msg), getpid(), 0);
        printf("parent: msg.sec = %d\n", msg.mi.sec);
        printf("parent: msg.nanosec = %d\n", msg.mi.nanosec);
        shmp->sec += msg.mi.sec;
        shmp->nanosec += msg.mi.nanosec;
        printf("sec = %d, nanosec = %d\n", shmp->sec, shmp->nanosec);
    }
  }

  for(i = 0; i < 2; i++) {
    msg.mtype = queue[i];
    msgsnd(msqid, &msg, sizeof(msg), 0);
    msgrcv(msqid, &msg, sizeof(msg), getpid(), 0);   
    printf("status = %d", msg.mi.status);
  }*/

  int pid = getpid();
  printf("oss: Parent pid = %d\n", pid);
  struct Queue* ready_queue = createQueue(19);
  int blocked[19];
  bool next = false,
       stop = false;
  int i;
  for(i = 0; i < 19; i++) {
    blocked[i] = -1;
    init_pcb(i, &shmp->pcb[i]);
  }
  i = 0;
  int pcb_index = -1;
  while(true) {
    if(i >= 5) {
      if(isEmpty(ready_queue)) {
        if(empty_blocked_queue(blocked)) {
          break;
        }
      }
    }
    if(i < 5) {
      printf("oss: here\n");
      pcb_index = -1;
      int j;
      for(j = 0; j < 19; j++) {
        if(!shmp->pcb[j].full) {
          pcb_index = j;
	  shmp->pcb[pcb_index].full = true;
	  break;
        }
      }
      i++;
      int child = fork();
      switch(child) {
        case -1:
          printf("Failed to fork\n");
          exit(1);
        case 0:
          printf("oss: here2\n");
          char num2[50];
          sprintf(num2, "%d", pcb_index);
          execl("./child_proc", "./child_proc", num2, NULL);
        default:
          printf("oss: here3\n");
          msgrcv(msqid, &msg, sizeof(msg), getpid(), 0);
          shmp->nanosec += 10;
          printf("Child %d placed on queue\n", pcb_index);
          enqueue(ready_queue, pcb_index);
      }
    }
    // check blocked queue, place any expired onto que
    int l;
    for(l = 0; l < 19; l++) {
      if(blocked[l] != -1) {
        int ind = blocked[l];
        printf("oss: Child %d moving from blocked to ready queue\n", ind);
        enqueue(ready_queue, ind);
        shmp->pcb[ind].blocked = false;
        blocked[l] = -1;
      }
    }
  
    if(!isEmpty(ready_queue)) {
      // deque from the ready queue and run process by sending message
      int cur_proc = dequeue(ready_queue);
      printf("oss: Child %d being dequeued from ready queue, message send\n", cur_proc);
      msg.mtype = shmp->pcb[cur_proc].real_pid;
      msg.mi.sec = 0;
      msg.mi.nanosec = 0;
      msgsnd(msqid, &msg, sizeof(msg), 0);
      // wait for process to send message back
      fprintf(stderr, "oss: Waiting for Child %d\n", cur_proc);
      msgrcv(msqid, &msg, sizeof(msg), getpid(), 0);
      shmp->sec += msg.mi.sec;
      shmp->nanosec += msg.mi.nanosec;
      fprintf(stderr, "oss: Waiting for Child sec = %d, nanosec = %d\n", shmp->sec, shmp->nanosec); 
      if(shmp->pcb[cur_proc].blocked) {
	  // place in blocked array
	printf("oss: Child %d was blocked and placed on blocked queue\n", cur_proc);
        int k;
	for(k = 0; k < 19; k++) {
          if(blocked[k] == -1) {
            blocked[k] = cur_proc;
            printf("blocked[%d] = %d\n", k, blocked[k]);
            break;
          } 
        }
      }
      else if(shmp->pcb[cur_proc].done) {
        printf("oss: Child %d is done, detaching from shared memory\n", cur_proc);
        shmp->pcb[cur_proc].full = false;
      }
      else {
        printf("oss: Child %d is not done yet, going back on queue\n", cur_proc);	
        enqueue(ready_queue, cur_proc);
      }
    }

  }


  
  // delete message queue;
  msgctl(msqid, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);
  return 0;
}
