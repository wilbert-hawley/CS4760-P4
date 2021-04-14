#include "utility.h"

struct shmbuf *shmp;
int shmid;

int main(int argc, char** argv) {

  printf("Compiled in oss.c\n");

  struct msgbuf msg;
  msg.test = 1;

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
  
  shmp->pcb[0] = init_pcb(0);

  int child = fork();

  if(child == 0)
    execl("./child_proc", "./child_proc", NULL);
  else {
    msg.mtype = child;
    shmp->nanosec += 10;
    // consider function to add to sec and subtract from nano
    msgsnd(msqid, &msg, sizeof(msg), 0);  
    msgrcv(msqid, &msg, sizeof(msg), getpid(), 0);
    //wait(NULL);
    printf("Child is done\n");
    printf("msg.test = %d", msg.test);
  }

  // delete message queue
  free(shmp->pcb[0]);
  msgctl(msqid, IPC_RMID, NULL);
  shmctl(shmid, IPC_RMID, NULL);
  return 0;
}
