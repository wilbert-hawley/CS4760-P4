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
struct msgbuf {
  long mtype;
  int test;
};

struct proc_ctrl_block {
  unsigned int cpu_sec,
               cpu_nanosec,
               sys_sec,
               sys_nanosec,
               burst_sec,
               burst_nanosec;
  int local_pid;
  // 0 for io, 1 for cpu
  int type;	
  long real_pid;
  // keep track if it's finished or not
  bool done,
       full;
};

int type_prob;
int type_select(int);
struct proc_ctrl_block* init_pcb(int);
 

struct shmbuf {
  unsigned int sec;
  unsigned int nanosec;
  struct proc_ctrl_block* pcb[19];
};
