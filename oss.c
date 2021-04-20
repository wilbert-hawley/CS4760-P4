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
  int i = 0;
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
  }
  
  // delete message queue
  //free(shmp->pcb[0]);
  msgctl(msqid, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);
  return 0;
}
