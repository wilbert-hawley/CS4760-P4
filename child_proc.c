#include "utility.h"


int main(int argc, char** argv) {
  key_t sharedMemoryKey;
  if ((sharedMemoryKey = ftok("./utility.h", 0)) == ((key_t) - 1))
  { 
    fprintf(stderr, "%s: ", argv[0]);
    perror("Error: Failed to derive key from utility.h\n");
    exit(EXIT_FAILURE);
  }
  int shmid;
  if ((shmid = shmget(sharedMemoryKey, sizeof(struct shmbuf), IPC_CREAT | 0600)) == -1)
  { 
    fprintf(stderr, "%s: ", argv[0]);
    perror("Error: Failed to create shared mem with key\n");
    exit(EXIT_FAILURE);
  }

  struct shmbuf *shmp = (struct shmbuf *)shmat(shmid, NULL, 0);
  // initialize pcb
  int loc = atoi(argv[1]); 
  shmp->pcb[loc].real_pid = getpid();
  shmp->pcb[loc].type = type_select(TYPE_PROB, ((loc * shmp->sec) + getpid()));
  shmp->pcb[loc].type = true;
  shmp->pcb[loc].done = false;
  shmp->pcb[loc].blocked = false;
  shmp->pcb[loc].cpu_sec = loc % 2;
  shmp->pcb[loc].cpu_nanosec = 500000000;
  printf("Child %d will run on cpu for %d.%d\n", loc, shmp->pcb[loc].cpu_sec,
          shmp->pcb[loc].cpu_nanosec); 
  
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
  fprintf(stderr, "child: Child %d is prepared.\n", loc);
  msg1.mtype = getppid();
  msg1.mi.sec = 0;
  msg1.mi.nanosec = 0;
  msgsnd(msqid1, &msg1, sizeof(msg1), 0);

  int x = 0;
  do {
    msgrcv(msqid2, &msg2, sizeof(msg2), getpid(), 0);
    fprintf(stderr, "child: Child %d recieved parent message, proceeding...\n", loc);
    msg1.mtype = getppid();
    //if interupted
    if(type_select(INTERUPT_PROB, x + loc)) {
      printf("child: Child %d is interupted, sending message back.\n", loc);
      shmp->pcb[loc].done = true;
      msg1.mi.sec = 0;
      msg1.mi.nanosec = 150000;
      msg1.mi.status = 1;
      msgsnd(msqid1, &msg1, sizeof(msg1), 0);
      break;
    }
    // if blocked: 
    else if(type_select(BLOCKED_PROB, ((x * loc) + 11 + x))) {
      //printf("child: Child %d is blocked, sending message back.\n", loc);
      shmp->pcb[loc].blocked = true;
      shmp->pcb[loc].block_sec = shmp->sec + 2;
      shmp->pcb[loc].block_nanosec = shmp->nanosec + 2;
      printf("child: Child %d blocked until %d.%d~~~~~~~~\n", loc, 
             shmp->pcb[loc].block_sec, shmp->pcb[loc].block_nanosec);
      msg1.mi.sec = 0;
      msg1.mi.nanosec = 150000;
      msg1.mi.status = 2;
      msgsnd(msqid1, &msg1, sizeof(msg1), 0);
    }
    // if finished with proc:
    else if(x >= 2) {
      printf("child: Child %d is finished, sending message back.\n", loc);
      shmp->pcb[loc].done = true;
      msg1.mi.sec = 0;
      msg1.mi.nanosec = 460000;
      msg1.mi.status = 3;
      msgsnd(msqid1, &msg1, sizeof(msg1), 0);
      break;
    }
    else if(x < 2) {
      printf("child: Child %d is not done yet, so put back on ready queue.\n", loc);
      msg1.mi.sec = 0;
      msg1.mi.nanosec = 10000000;
      msg1.mi.status = 0;
      msgsnd(msqid1, &msg1, sizeof(msg1), 0);
      //sleep(1);
    }
    else{
      fprintf(stderr, "something broke. x = %d\n", x);
    }
    x++;
  }while(true);
  shmdt(shmp);
  printf("child: Child %d finished\n", loc);
  return 0;
}
