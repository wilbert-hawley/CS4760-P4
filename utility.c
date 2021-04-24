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
  //printf("here2\n");
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
