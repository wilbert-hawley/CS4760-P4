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
 // msgrcv(msqid, &msg, sizeof(msg), getpid(), 0);
 
  printf("child_proc: Message recieved: %d\n", msg.mi.local_pid);
  printf("child_proc: Time: %ds %dns\n", shmp->sec, shmp->nanosec);
  printf("child: msg.mi.local_id = %d\n", msg.mi.local_pid);
  printf("child: msg.mi.sec = %d\n", msg.mi.sec);
  msg.mtype = getppid();
  //sleep(3);
  msg.mi.sec = 0;
  msg.mi.nanosec = 0;
  msg.mi.sec += 1;
  msg.mi.nanosec += 100;
  printf("chid: here\n"); 
  msgsnd(msqid, &msg, sizeof(msg), 0);

  do {
    msgrcv(msqid, &msg, sizeof(msg), getpid(), 0);
    msg.mi.status = 2;    
    msg.mtype = getppid();
    msgsnd(msqid, &msg, sizeof(msg), 0);
    break;

  }while(true);
  printf("child: here 2\n");
  return 0;
}
