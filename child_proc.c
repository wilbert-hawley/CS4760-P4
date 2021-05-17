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
  srand(time(0) * (loc * 111));
  shmp->pcb[loc].real_pid = getpid();
  shmp->pcb[loc].type = type_select(TYPE_PROB, ((loc * shmp->sec) + getpid()));
  shmp->pcb[loc].full = true;
  shmp->pcb[loc].done = false;
  shmp->pcb[loc].blocked = false;
  shmp->pcb[loc].cpu_sec = 0;
  shmp->pcb[loc].cpu_nanosec = (rand() % 40000000) + 250000;
  printf("Child %d will run on cpu for %d.%d\n", loc, shmp->pcb[loc].cpu_sec,
          shmp->pcb[loc].cpu_nanosec); 
  unsigned finish_sec = 0;
  unsigned finish_nanosec = 0;
  unsigned diff_sec = 0,
           diff_nanosec = 0;
 
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

  int block_prob;
  if(shmp->pcb[loc].type == 1)
    block_prob = CPU_BLOCKED_PROB;
  else
    block_prob = IO_BLOCKED_PROB;

  //srand(time(0) * (loc * 111));
  // messages from child to parent
  struct msgbuf msg1;
  // messages from parent to child
  struct msgbuf msg2;
  fprintf(stderr, "child: Child %d is prepared.\n", loc);
  msg1.mtype = getppid();
  msg1.mi.local_pid = loc;
  msg1.mi.sec = 0;
  msg1.mi.nanosec = 0;
  msgsnd(msqid1, &msg1, sizeof(msg1), 0);
  int x = 0;
  do {
    msgrcv(msqid2, &msg2, sizeof(msg2), getpid(), 0);
    fprintf(stderr, "child: Child %d recieved parent message, proceeding...\n", loc);
    msg1.mtype = getppid();
    msg1.mi.local_pid = loc;   
    time_sub(shmp->pcb[loc].cpu_sec,shmp->pcb[loc].cpu_nanosec, finish_sec,
             finish_nanosec, &diff_sec, &diff_nanosec); 
    //if interupted
    if(type_select(INTERUPT_PROB, x + loc)) {
      printf("child: Child %d is interupted, sending message back.\n", loc);
      msg1.mi.sec = 0;
      msg1.mi.nanosec = rand() % 150000;
      msg1.mi.status = 1;
      shmp->pcb[loc].done = true;
      shmp->pcb[loc].total_cpu_sec = finish_sec;
      shmp->pcb[loc].total_cpu_nanosec = finish_nanosec + msg1.mi.nanosec;
      if(shmp->pcb[loc].total_cpu_nanosec >= 1000000000) {
        shmp->pcb[loc].total_cpu_sec++;
        shmp->pcb[loc].total_cpu_nanosec -= 1000000000;
      }
      msgsnd(msqid1, &msg1, sizeof(msg1), 0);
      break;
    }
    // if blocked: 
    else if(type_select(block_prob, ((x * loc) + 11 + x))) {
      //printf("child: Child %d is blocked, sending message back.\n", loc);
      msg1.mi.sec = 0;
      msg1.mi.nanosec = rand() % 250000;
      msg1.mi.status = 2;
      shmp->pcb[loc].blocked = true;
      shmp->pcb[loc].block_sec = shmp->sec + (rand() % 6);
      shmp->pcb[loc].block_nanosec = shmp->nanosec + (rand() % 1001);
      printf("child: Child %d blocked until %d.%d~~~~~~~~\n", loc, 
             shmp->pcb[loc].block_sec, shmp->pcb[loc].block_nanosec);
      msgsnd(msqid1, &msg1, sizeof(msg1), 0);
    }
    //else if(x < 2) {
    else if(diff_sec > 0 || diff_nanosec >= 10000000) { 
      printf("child: Child %d is not done yet, so put back on ready queue.\n", loc);
      msg1.mi.sec = 0;
      msg1.mi.nanosec = 10000000;
      msg1.mi.status = 0;
      msgsnd(msqid1, &msg1, sizeof(msg1), 0);
      //sleep(1);
    }
    else if(diff_sec == 0 && diff_nanosec < 10000000) {
      printf("child: Child %d is finished, sending message back.\n", loc);
      shmp->pcb[loc].done = true;
      shmp->pcb[loc].total_cpu_sec = finish_sec;
      shmp->pcb[loc].total_cpu_nanosec = finish_nanosec + diff_nanosec;
      if(shmp->pcb[loc].total_cpu_nanosec >= 1000000000) {
        shmp->pcb[loc].total_cpu_sec++;
        shmp->pcb[loc].total_cpu_nanosec -= 1000000000;
      } 
      msg1.mi.sec = 0;
      msg1.mi.nanosec = diff_nanosec;
      msg1.mi.status = 3;
      msgsnd(msqid1, &msg1, sizeof(msg1), 0);
      break;
    }
    else{
      fprintf(stderr, "child: something broke. x = %d\n", x);
    }
    finish_sec += msg1.mi.sec;
    finish_nanosec += msg1.mi.nanosec;
    if(finish_nanosec >= 1000000000) {
      finish_sec++;
      finish_nanosec -= 1000000000;
    }
    x++;
  }while(true);
  if(shmdt(shmp) == -1) {
    printf("~~~~~~~~~~~~~~~Child %d failed to detach from shared memory.\n", loc);
  }
  printf("child: Child %d finished~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", loc);
  return 0;
}
