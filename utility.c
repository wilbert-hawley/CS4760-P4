#include "utility.h"

int type_prob = 90;

int type_select(int x)
{
    time_t t;
    srand((unsigned) time(&t) + (x*100));
    int r = rand() % 100;
    if(r < type_prob)
        return 1;   // cpu
    else
        return 0;   // io
}

//struct proc_ctrl_block* init_pcb(int local) 
void init_pcb(int local, struct proc_ctrl_block* temp)
{
  printf("here2\n");
  temp->cpu_sec = 0;
  temp->cpu_nanosec = 0;
  temp->sys_sec = 0;
  temp->sys_nanosec = 0;
  temp->burst_sec = 0;
  temp->burst_nanosec = 0;
  temp->local_pid = local;
  // 0 for io, 1 for cpu
  temp->type = type_select(local);
  temp->real_pid = getpid();
  // keep track if it's finished or not
  temp->done = false;
  // in use
  temp->full = true;
}
