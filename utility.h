#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <stdbool.h>
#include <time.h>
#include <limits.h>

struct msg_info {
  int local_pid;
  int sec;
  int nanosec;
  int status;
};

struct msgbuf {
  long mtype;
  struct msg_info mi;
  //int local_pid; 0
  //int sec; 1
  //int nano_sec; 2
  //int blocked/interupt/finished 3
};

struct proc_ctrl_block {
  unsigned int cpu_sec,
               cpu_nanosec,
               sys_sec,
               sys_nanosec,
               burst_sec,
               burst_nanosec,
               block_sec,
               block_nanosec,
               total_cpu_sec,
               total_cpu_nanosec;
  int local_pid;
  // 0 for io, 1 for cpu
  int type;	
  long real_pid;
  // keep track if it's finished or not
  bool done,
       full,
       blocked;
};

int TYPE_PROB;
int BLOCKED_PROB;
int INTERUPT_PROB;
int type_select(int, int);
//struct proc_ctrl_block* init_pcb(int);
void init_pcb(int, struct proc_ctrl_block*); 

struct shmbuf {
  unsigned int sec;
  unsigned int nanosec;
  struct proc_ctrl_block pcb[19];
};

struct Queue {
    int front, 
        rear, 
        size;
    unsigned capacity;
    int* pcb_index;
};

struct Queue* createQueue(unsigned);
int isFull(struct Queue*);
int isEmpty(struct Queue*);
void enqueue(struct Queue*, int);
int dequeue(struct Queue*);
int front(struct Queue*);
int rear(struct Queue*);
bool empty_blocked_queue(int []);
void time_sub(unsigned, unsigned, unsigned, unsigned, unsigned*, unsigned*);
