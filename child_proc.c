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
  //init_pcb(loc, &shmp->pcb[loc]);
  shmp->pcb[loc].real_pid = getpid();
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
  fprintf(stderr, "child: Child %d is prepared.\n", loc);
  //sleep(2);
  struct msgbuf msg;
  msg.mtype = getppid();
  msg.mi.sec = 0;
  msg.mi.nanosec = 0;
  //printf("child: Child %d is prepared.\n", loc);
  msgsnd(msqid, &msg, sizeof(msg), 0);
  int x = 0;
  //sleep(3);
  do {
    msgrcv(msqid, &msg, sizeof(msg), getpid(), 0);
    fprintf(stderr, "child: Child %d recieved parent message, proceeding...\n", loc);
    //msg.mtype = getppid();
    printf("child: Child %d: x = %d\n", loc, x);
    // if blocked: 
    if(x == 0) {
      printf("child: Child %d is blocked, sending message back.\n", loc);
      shmp->pcb[loc].blocked = true;
      msg.mtype = getppid();
      printf("child: message type = %ld\n", msg.mtype);
      msg.mi.sec = 0;
      msg.mi.nanosec = 150000;
      msg.mi.status = 1;
      msgsnd(msqid, &msg, sizeof(msg), 0); 
    }
    // if finished with proc:
    else if(x > 3) {
      printf("child: Child %d is finished, sending message back.\n", loc);
      shmp->pcb[loc].done = true;
      msg.mtype = getppid();
      printf("child: message type = %ld\n", msg.mtype);
      msg.mi.sec = 0;
      msg.mi.nanosec = 460000;
      msg.mi.status = 3;
      msgsnd(msqid, &msg, sizeof(msg), 0);
      break;
    }
    else if(x > 0 && x < 4) {
      printf("child: Child %d is not done yet, so put back on ready queue.\n", loc);
      msg.mtype = getppid();
      printf("child: message type = %ld\n", msg.mtype);
      msg.mi.sec = 0;
      msg.mi.nanosec = 10000000;
      msg.mi.status = 0;
      msgsnd(msqid, &msg, sizeof(msg), 0);
    }
    x++;
  }while(true);
  printf("child: Child %d finished\n", loc);
  return 0;
}
