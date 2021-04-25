#include "utility.h"

struct shmbuf *shmp;
int shmid;

int main(int argc, char** argv) {

  printf("Compiled in oss.c\n");

  //struct msgbuf msg;
  //msg.mi.local_pid = -1;
  //msg.mi.sec = 0;
  //msg.mi.nanosec = 0;

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
  /*key_t messageKey;
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
  }*/


  key_t messageKey1;
  if ((messageKey1 = ftok("./README", 0)) == ((key_t) - 1))
  {
    fprintf(stderr, "%s: ", argv[0]);
    perror("Error: Failed to derive key from README\n");
    exit(EXIT_FAILURE);
  }
  int msqid1;
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
  int msqid2;
  if ((msqid2 = msgget(messageKey1, IPC_CREAT | 0600 )) == -1)
  {
    perror("Error: Failed to create message queue\n");
    return EXIT_FAILURE;
  } 

  // messages from child to parent
  struct msgbuf msg1;
  // messages from parent to child
  struct msgbuf msg2;

  
  unsigned next_proc_sec = 2;
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
  int pcb_index;

  while(true) {
    shmp->nanosec += 10000000;
    if(shmp->nanosec > 1000000000) {
      shmp->sec += 1;
      shmp->nanosec -= 1000000000;
    } 
    printf("oss: Top of loop. Time: shmp->sec = %d, shmp->nanosec = %d\n", shmp->sec, shmp->nanosec);
    if(shmp->sec >= 4) {
      stop = true;
      if(isEmpty(ready_queue) && empty_blocked_queue(blocked)) {
        break;
      }
    }

    if(shmp->sec >= next_proc_sec && !stop) {
      next_proc_sec++;
      pcb_index = -1;
      int j;
      for(j = 0; j < 19; j++) {
        if(!shmp->pcb[j].full) {
          pcb_index = j;
	  shmp->pcb[pcb_index].full = true;
	  break;
        }
      }
      if(pcb_index != -1) {
        i++;
        int child = fork();
        switch(child) {
          case -1:
            printf("Failed to fork\n");
            exit(1);
          case 0: ;
            char num2[50];
            sprintf(num2, "%d", pcb_index);
            execl("./child_proc", "./child_proc", num2, NULL);
          default:
            //msgrcv(msqid, &msg, sizeof(msg), getpid(), 0);
            msgrcv(msqid1, &msg1, sizeof(msg1), getpid(), 0);
            shmp->nanosec += 1000;
            printf("Child %d placed on queue\n", pcb_index);
            enqueue(ready_queue, pcb_index);
        }
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
     /* msg.mtype = shmp->pcb[cur_proc].real_pid;
      msg.mi.sec = 0;
      msg.mi.nanosec = 0;
      msgsnd(msqid, &msg, sizeof(msg), 0);*/
      msg2.mtype = shmp->pcb[cur_proc].real_pid;
      msg2.mi.sec = 0;
      msg2.mi.nanosec = 0;
      msgsnd(msqid2, &msg2, sizeof(msg2), 0);
      // wait for process to send message back
      fprintf(stderr, "oss: Waiting for Child %d\n", cur_proc);
      sleep(1);
      /*msgrcv(msqid, &msg, sizeof(msg), getpid(), 0);
      shmp->sec += msg.mi.sec;
      shmp->nanosec += msg.mi.nanosec;*/
      msgrcv(msqid1, &msg1, sizeof(msg1), getpid(), 0);
      shmp->sec += msg1.mi.sec;
      shmp->nanosec += msg1.mi.nanosec;
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

 
/*  while(true) {
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
          msgrcv(msqid1, &msg1, sizeof(msg1), getpid(), 0);
          shmp->nanosec += 10;
          printf("Child %d placed on queue\n", pcb_index);
          enqueue(ready_queue, pcb_index);
      }
    }

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
     
      int cur_proc = dequeue(ready_queue);
      printf("oss: Child %d being dequeued from ready queue, message send\n", cur_proc);
      msg2.mtype = shmp->pcb[cur_proc].real_pid;
      msg2.mi.sec = 0;
      msg2.mi.nanosec = 0;
      msgsnd(msqid2, &msg2, sizeof(msg2), 0);
      
      fprintf(stderr, "oss: Waiting for Child %d\n", cur_proc);
      msgrcv(msqid1, &msg1, sizeof(msg1), getpid(), 0);
      shmp->sec += msg1.mi.sec;
      shmp->nanosec += msg1.mi.nanosec;
      fprintf(stderr, "oss: Waiting for Child sec = %d, nanosec = %d\n", shmp->sec, shmp->nanosec); 
      if(shmp->pcb[cur_proc].blocked) {
	  
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
*/
 
  // delete message queue;
  msgctl(msqid1, IPC_RMID, NULL);
  msgctl(msqid2, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);
  return 0;
}
