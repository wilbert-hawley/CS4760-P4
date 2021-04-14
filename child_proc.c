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
  struct msgbuf msg;
  // wait for oss to send messag to run
  msgrcv(msqid, &msg, sizeof(msg), getpid(), 0);
  //printf("child_proc: shmp->pcb[0].local_pid = %d\n", shmp->pcb[0]->local_pid); 
  printf("child_proc: Message recieved: %d\n", msg.test);
  printf("child_proc: Time: %ds %dns\n", shmp->sec, shmp->nanosec);
  //printf("child_proc: shmp->pcb[0].local_pid = %d\n", shmp->pcb[0]->local_pid);
  msg.mtype = getppid();
  msg.test = 0;
  
  //sleep(3);
  msgsnd(msqid, &msg, sizeof(msg), 0);

  return 0;
}
