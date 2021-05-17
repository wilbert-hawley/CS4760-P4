#include "utility.h"

int TYPE_PROB = 70;
int BLOCKED_PROB = 10;
int INTERUPT_PROB = 5;

int IO_BLOCKED_PROB = 55; // 55
int CPU_BLOCKED_PROB = 10; // 10

// funciton to make random decisions
int type_select(int prob, int x)
{
    time_t t;
    srand((unsigned) time(&t) + (getpid() * x));
    int r = rand() % 100;
    if(r < prob)
        return 1;   // cpu, blocked, interupted
    else
        return 0;   // io, not blocked, not interupted
}

//struct proc_ctrl_block* init_pcb(int local) 
void init_pcb(int local, struct proc_ctrl_block* temp)
{
  temp->cpu_sec = 0;
  temp->cpu_nanosec = 0;
  temp->sys_sec = 0;
  temp->sys_nanosec = 0;
  temp->burst_sec = 0;
  temp->burst_nanosec = 0;
  temp->block_sec = 0;
  temp->block_nanosec = 0;
  temp->local_pid = local;
  // 0 for io, 1 for cpu
  temp->type = type_select(TYPE_PROB, local);
  temp->real_pid = getpid();
  // keep track if it's finished or not
  temp->done = false;
  // in use
  temp->full = false;
  temp->blocked = false;
}

// This queue structure was taken (and slightly altered) from:
// https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/
struct Queue* createQueue(unsigned capacity)
{
    struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
 
    queue->rear = capacity - 1;
    queue->pcb_index = (int*)malloc(queue->capacity * sizeof(int));
    return queue;
}

int isFull(struct Queue* queue)
{
    return queue->size == queue->capacity;
}
 

int isEmpty(struct Queue* queue)
{
    return queue->size == 0;
}

void enqueue(struct Queue* queue, int item)
{
    if (isFull(queue))
        return;
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->pcb_index[queue->rear] = item;
    queue->size += 1;
}
 

int dequeue(struct Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    int pcbi = queue->pcb_index[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size -= 1;
    return pcbi;
}

int front(struct Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->pcb_index[queue->front];
}
 

int rear(struct Queue* queue)
{
    if (isEmpty(queue))
        return INT_MIN;
    return queue->pcb_index[queue->rear];
}

// check whether the blocked queue is empty
bool empty_blocked_queue(int arr[])
{
  int i;
  for(i = 0; i < 19; i++) {
    if(arr[i] != -1) {
      return false;
    }
  }
  return true;
}

// funciton to find the difference between two times
void time_sub(unsigned x_sec, unsigned x_nano, unsigned y_sec, unsigned y_nano, 
              unsigned *a_sec, unsigned *a_nano) {
  if(y_sec > x_sec) {
    if(y_nano >= x_nano) 
      *a_nano = y_nano - x_nano;
    else {
      y_sec--;       
      y_nano += 1000000000;
      *a_nano = y_nano - x_nano;
    }
    *a_sec = y_sec - x_sec;
  }
  else if(x_sec > y_sec) {
    if(x_nano >= y_nano) 
      *a_nano = x_nano - y_nano;
    else {
      x_sec--;
      x_nano += 1000000000;
      *a_nano = x_nano - y_nano;
    }
    *a_sec = x_sec - y_sec;
  }
  else{
    *a_sec = 0;
    if(y_nano >= x_nano) 
      *a_nano = y_nano - x_nano;
    else
      *a_nano = x_nano - y_nano;
  }
}

unsigned int maxTimeBetweenNewProcsNS = 1000;
unsigned int maxTimeBetweenNewProcsSecs = 2;
