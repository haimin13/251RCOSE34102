#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define NUM_PROCESS 8
#define MAX_ARRIVAL_TIME 10
#define MAX_CPU_BURST_TIME 15
#define MAX_IO_BURST_TIME 3
#define MAX_IO_REQUEST 3
#define NUM_IO_DEVICES 2
#define TIME_QUANTUM 4


typedef struct {  
    short point;
    short device;
} IORequest;

typedef struct {
    short pid;
    short priority;
    short arrival_time;
    short cpu_burst_time;
    short io_burst_time;
    short num_io_request;
    IORequest *io_request_points;
    short remain_io_request;
    short remain_cpu_burst_time;
    short remain_io_burst_time;
    short waited_time;
    short finished_time;
} Process;

typedef struct Node {
    Process* process;
    struct Node *next;
} Node;

typedef struct Heap {
    Process* arr[NUM_PROCESS];
    int size;
} Heap;


int Max(int a, int b);
int Min(int a, int b);
void PrintArray(IORequest *arr, short size);
void PrintProcess(Process *p);
int CompareArrival(const void* first, const void* second);
void FreeMemory(Process *p);
void CreateProcess(Process *p, int argc, char* argv[]);
void ManualCreate(Process *p, int idx);
void ResetProcess(Process *p);
void Evaluate(Process *p);
void ProcessIO(char mode);
void QueuePush(Node** queue, Process* process);
Process* QueuePop(Node** queue);
void QueueBasedSchedule(Process *p, char mode);
void MinHeapBasedSchedule(Process* p, char mode, bool preemption);
bool IsFirstBigger(int parent, int child, char mode);
void HeapBuild(char mode);
void HeapPop(char mode);


int unfinished = NUM_PROCESS;
Node *ready_queue = NULL;
Node *io_queue[NUM_IO_DEVICES] = {NULL, };
Node *heap_wait_queue = NULL;
Heap ready_heap = { NULL, };

char reason[][30] = {
    "(Scheduler Start)",
    "(Process Finished)",
    "(I/O Requested)",
    "(Time Slice Expired)",
    "(Ready Queue Filled)",
    "(Preempted)",
    "(All Process Finished)"
};

/////////////////////////////////////////////////////////////////////////////////////////

// 전체적으로 주석 추가하기

void main(int argc, char* argv[]) {
    Process p[NUM_PROCESS];
    CreateProcess(p, argc, argv);
    // Process를 arrival time순으로 정렬해놔도 좋겠다.
    // total processing time이 매우 길어지거나 process 수가 매우 많으면 매번 프로세스 순회해서
    // arrival time 체크하는 오버헤드도 상당할 것이기 때문.

    PrintProcess(p);

    // Process arrival check overhead를 줄이기 위한 함수
    qsort(p, NUM_PROCESS, sizeof(Process), CompareArrival);
    PrintProcess(p);

    QueueBasedSchedule(p, 'F'); // FCFS Schedule
    QueueBasedSchedule(p, 'R'); // RR Schedule

    MinHeapBasedSchedule(p, 'S', false);    // nonpreemtive SJF
    MinHeapBasedSchedule(p, 'S', true);     // preemtive SJF
    MinHeapBasedSchedule(p, 'P', false);    // nonpreemptive Priority
    MinHeapBasedSchedule(p, 'P', true);    // preemptive Priority

    FreeMemory(p);
    return;
}

/////////////////////////////////////////////////////////////////////////////////////////


void CreateProcess(Process *p, int argc, char* argv[]) {
    srand((unsigned int)time(NULL));
    int num_priority = NUM_PROCESS / 2;

    for (int i = 0; i < NUM_PROCESS; i++) {
        if (argc > 1 && strcmp(argv[1], "manual") == 0) {
            ManualCreate(p, i);
            // NUM_PROCESS, MAX값들도 manually set 할 수 있게 바꿔야 하나
        }
        else {
            p[i].priority = rand() % num_priority + 1;
            p[i].arrival_time = rand() % MAX_ARRIVAL_TIME;
            p[i].cpu_burst_time = rand() % MAX_CPU_BURST_TIME + 1;
            p[i].io_burst_time = rand() % MAX_IO_BURST_TIME + 1;
        }
        p[i].pid = i;
        p[i].remain_cpu_burst_time = p[i].cpu_burst_time;
        p[i].remain_io_burst_time = p[i].io_burst_time;
        p[i].waited_time = 0;
        p[i].finished_time = 0;
        if (p[i].cpu_burst_time > 1) {
            int num_io_request = rand() % (Min(MAX_IO_REQUEST, Max(1, p[i].cpu_burst_time - 2)) + 1); // 0번에서 가능한 경우 최대 3번 발생.
            p[i].num_io_request = num_io_request;
            p[i].remain_io_request = num_io_request;
            p[i].io_request_points = malloc(sizeof(IORequest) * (num_io_request + 1)); // 계산 편의를 위해 인덱스 0에 가비지 값.
            int temp[p[i].cpu_burst_time];
            memset(temp, 0, sizeof(temp));
            int j = 0;
            while(j < num_io_request) {
                // 1 ~ cpu_burst_time - 1의 범위에서 io request 횟수 만큼 중복 없이 수 뽑기.
                int point = rand() % (p[i].cpu_burst_time - 1) + 1;
                // num_io_request이 높으면 중복 자주 뽑혀서 시간 오래걸릴 수도 있다.
                // 더 효율적인 알고리즘이 있을까?
                if (temp[point] == 0) { 
                    temp[point] = 1;
                    j++;
                }
            }
            for (j = p[i].cpu_burst_time - 1; j > 0; j--) {
                if (temp[j] == 1) {
                    p[i].io_request_points[num_io_request].point = j;
                    p[i].io_request_points[num_io_request--].device = rand() % NUM_IO_DEVICES;
                }
            }
        }
        else {
            p[i].num_io_request = 0;
            p[i].remain_io_request = 0;
            p[i].io_request_points = malloc(sizeof(IORequest));
        }
        p[i].io_request_points[0].point = -1;
        p[i].io_request_points[0].device = -1;
    }
    return;
}

void ManualCreate(Process* p, int i) {
    printf("Input priority: ");
    scanf("%hd", &(p[i].priority));
    printf("Input arrival time (max: %d): ", MAX_ARRIVAL_TIME);
    scanf("%hd", &(p[i].arrival_time));
    printf("Input CPU burst time (max: %d): ", MAX_CPU_BURST_TIME);
    scanf("%hd", &(p[i].cpu_burst_time));
    printf("Input IO burst time (max: %d): ", MAX_IO_BURST_TIME);
    scanf("%hd", &(p[i].io_burst_time));
    printf("\n");
}

void ResetProcess(Process *p) {
    for (int i = 0; i < NUM_PROCESS; i++) {
        p[i].remain_io_request = p[i].num_io_request;
        p[i].remain_cpu_burst_time = p[i].cpu_burst_time;
        p[i].waited_time = 0;
        p[i].finished_time = 0;
    }
    unfinished = NUM_PROCESS;
}

void Evaluate(Process *p) {
    int sum_waiting = 0, sum_turnaround = 0;
    for (int i = 0; i < NUM_PROCESS; i++) {
        sum_waiting += p[i].waited_time;
        sum_turnaround += (p[i].finished_time - p[i].arrival_time);
    }
    printf("\naverage waiting time: %.2lf\n", (double)sum_waiting / NUM_PROCESS);
    printf("average turnaround time: %.2lf\n", (double)sum_turnaround / NUM_PROCESS);
}

void ProcessIO(char mode) {
    for (int i = 0; i < NUM_IO_DEVICES; i++) {
        if (io_queue[i] != NULL) {
            Process* io_head = io_queue[i]->process;
            io_head->remain_io_burst_time--;
            if (io_head->remain_io_burst_time == 0) {
                io_head->remain_io_request--;
                io_head->remain_io_burst_time = io_head->io_burst_time;
                if (mode == 'F' || mode == 'R')
                    QueuePush(&ready_queue, io_queue[i]->process);
                else if (mode == 'S' || mode == 'P') 
                    QueuePush(&heap_wait_queue, io_queue[i]->process);
                QueuePop(&io_queue[i]);
            }
        }
    }
}

void QueuePush(Node** queue, Process* process) {
    Node *new_node = malloc(sizeof(Node));
    new_node->process = process;
    new_node->next = NULL;
    if (*queue == NULL) {
        *queue = new_node;
    }
    else {
        Node* cursor = *queue;
        while (cursor->next != NULL) {
            cursor = cursor->next;
        }
        cursor->next = new_node;
    }
}

Process* QueuePop(Node** queue) {
    if (*queue == NULL) {
        printf("Queue is Empty!\n");
        return NULL;
    }
    Node* temp = *queue;
    Process* out = (*queue)->process;
    *queue = (*queue)->next;
    free(temp);
    return out;
}

void QueueBasedSchedule(Process *p, char mode) {
    int timeQuantum;
    if (mode == 'F') {
        printf("\n\nStart FCFS scheduling\n\n");
        timeQuantum = MAX_CPU_BURST_TIME + 1;
    }
    else if (mode == 'R') {
        printf("\n\nStart RR scheduling\n\n");
        timeQuantum = TIME_QUANTUM;
    }
    int prev = -2;
    int cur = -1;
    int circum = 0;
    int time = 0;
    int time_spent = 0;
    int arrived = 0;

    printf("time| Process\n");
    printf("%3d |-------- %s\n", time, reason[0]);

    while (unfinished > 0) {
        while (arrived < NUM_PROCESS) {
            if (p[arrived].arrival_time == time) {
                QueuePush(&ready_queue, &p[arrived]);
                arrived++;
            }
            else break;
        }
        if (ready_queue != NULL) {
            Process* head = ready_queue->process;

            head->remain_cpu_burst_time--;
            time_spent++;
            Node* cursor = ready_queue->next;
            while (cursor != NULL) {
                cursor->process->waited_time++;
                cursor = cursor->next;
            }
            ProcessIO(mode);

            cur = head->pid;
            if (prev == -1) printf("%3d |-------- %s\n", time, reason[4]);  // ready queue filled

            if (head->remain_cpu_burst_time == 0) {
                unfinished--;
                head->finished_time = time + 1;
                QueuePop(&ready_queue);
                time_spent = 0;
                circum = 1; // process finished
                if (unfinished == 0) circum = 6;    // all process finished
            }
            else if (head->io_request_points[head->remain_io_request].point == head->remain_cpu_burst_time) {
                QueuePush(&io_queue[head->io_request_points[head->remain_io_request].device], head);
                QueuePop(&ready_queue);
                time_spent = 0;
                circum = 2; // I/O requested
            }
            else if (time_spent == timeQuantum) { // if mode == 'F', never satisfy this condition.
                QueuePop(&ready_queue);
                if (ready_queue == NULL) 
                QueuePush(&ready_queue, head);
                time_spent = 0;
                circum = 3; // time slice expired
            }
        }
        else {
            ProcessIO(mode);
            cur = -1;
        }
        if (prev != cur) {
            if (cur == -1) printf("    | idle\n");
            else printf("    | P_%d\n", cur);
        }
        if (circum) {
            printf("%3d |-------- %s\n", time + 1, reason[circum]);
            circum = 0;
        }
        prev = cur;
        time++;
    }
    Evaluate(p);
    ResetProcess(p);
}

void MinHeapBasedSchedule(Process* p, char mode, bool preemption) {
    if (preemption == true) printf("\n\nStart preemptive ");
    else printf("\n\nStart nonpreemptive ");
    if (mode == 'S') printf("SJF scheduling\n\n");
    else if (mode == 'P') printf("Start Priority scheduling\n\n");
    
    int prev = -2;
    int cur = -1;
    int circum = 0;
    int time = 0;
    int arrived = 0;
    bool popped = true;

    printf("time| Process\n");
    printf("%3d |-------- %s\n", time, reason[0]);
    while (unfinished > 0) {
        while (arrived < NUM_PROCESS) {
            if (p[arrived].arrival_time == time) {
                QueuePush(&heap_wait_queue, &p[arrived]);
                arrived++;
            }
            else break;
        }
        if (preemption || ready_heap.size == 0 || popped) {
            HeapBuild(mode);
        }
        if (ready_heap.arr[0] != NULL) {
            Process* head = ready_heap.arr[0];
 
            head->remain_cpu_burst_time--;
            int i = 1;
            while (ready_heap.arr[i] != NULL) {
                ready_heap.arr[i]->waited_time++;
                i++;
                if (i >= ready_heap.size) break;
            }
            Node* cursor = heap_wait_queue;
            while (cursor != NULL) { 
                cursor->process->waited_time++;
                cursor = cursor->next;
            }
            ProcessIO(mode);

            cur = head->pid;
            if (prev == -1) printf("%3d |-------- %s\n", time, reason[4]);  // ready queue filled
            else if (!popped && (prev != cur)) {
                printf("%3d |-------- %s\n", time, reason[5]);  // preempted
            }

            if (head->remain_cpu_burst_time == 0) {
                unfinished--;
                head->finished_time = time + 1;
                HeapPop(mode);
                popped = true;
                circum = 1; // process finished
                if (unfinished == 0) circum = 6; // all process finished
            }
            else if (head->io_request_points[head->remain_io_request].point == head->remain_cpu_burst_time) {
                QueuePush(&io_queue[head->io_request_points[head->remain_io_request].device], head);
                HeapPop(mode);
                popped = true;
                circum = 2; // I/O requested
            }
            else popped = false;
        }
        else {
            ProcessIO(mode);
            cur = -1;
        }
        if (prev != cur) {
            if (cur == -1) printf("    | idle\n");
            else printf("    | P_%d\n", cur);
        }
        if (circum) {
            printf("%3d |-------- %s\n", time + 1, reason[circum]);
            circum = 0;
        }
        prev = cur;
        time++;
    }
    Evaluate(p);
    ResetProcess(p);
}

bool IsFirstBigger(int parent, int child, char mode) {
    if (mode == 'S')
        return (ready_heap.arr[parent]->remain_cpu_burst_time > ready_heap.arr[child]->remain_cpu_burst_time) ? true: false;
    else if (mode == 'P')
        return (ready_heap.arr[parent]->priority > ready_heap.arr[child]->priority) ? true: false;
}

void HeapBuild(char mode){  // reHeapUp
    while (heap_wait_queue != NULL) {
        if (ready_heap.size >= NUM_PROCESS) {
            printf("\nheap is FULL. cannot insert.\n\n");
            return;
        }
        ready_heap.arr[ready_heap.size] = QueuePop(&heap_wait_queue);
        int child = ready_heap.size;
        while (child > 0) {
            int parent = (child - 1) / 2;
            if (IsFirstBigger(parent, child, mode)) { // first가 크면 true, second가 크면 false
                Process* temp = ready_heap.arr[child];
                ready_heap.arr[child] = ready_heap.arr[parent];
                ready_heap.arr[parent] = temp;
                child = parent;
            }
            else break;
        }
        ready_heap.size++;
    }
}

void HeapPop(char mode) {   // reHeapDown
    if (ready_heap.size == 0) {
        printf("\nheap is EMPTY. cannot delete.\n\n");
        return;
    }
    ready_heap.arr[0] = ready_heap.arr[ready_heap.size - 1];
    ready_heap.arr[ready_heap.size - 1] = NULL;
    ready_heap.size--; 
    int parent = 0;
    while (parent < (ready_heap.size / 2)) {
        int first_child = 2 * parent + 1;
        int second_child = 2 * parent + 2;
        int child = first_child;
        if (second_child < ready_heap.size)
            child = IsFirstBigger(first_child, second_child, mode) ? second_child: first_child;

        if (IsFirstBigger(parent, child, mode)) { // first가 크면 true, second가 크면 false
            Process* temp = ready_heap.arr[child];
            ready_heap.arr[child] = ready_heap.arr[parent];
            ready_heap.arr[parent] = temp;
            parent = child;
        }
        else break;
    }
}

int Max(int a, int b) {
    return (a > b) ? a : b;
}

int Min(int a, int b) {
    return (a <= b) ? a: b; 
}

void PrintArray(IORequest *arr, short size) {
    printf("[ ");
    for (short i = 1; i < size; i++) {
        printf("(%d, #%d) ", arr[i].point, arr[i].device);
    }
    printf("]\n");
}

void PrintProcess(Process *p) {
    printf("---------------------------------------------------------\n");
    printf("| PID | Priority | Arrival | Burst time | # I/O Request |\n");
    for (int i = 0; i < NUM_PROCESS; i++) {
        printf("|-------------------------------------------------------|\n");
        printf("| %3d | %8d | %7d | %10d | %13d |\n", p[i].pid, p[i].priority,
            p[i].arrival_time, p[i].cpu_burst_time, p[i].num_io_request);
    }
    printf("---------------------------------------------------------\n\n");
}

int CompareArrival(const void* first, const void* second) {
    short at_a = ((Process *)first)->arrival_time;
    short at_b = ((Process *)second)->arrival_time;
    return at_a - at_b;
}

void FreeMemory(Process *p) {
    for (int i = 0; i < NUM_PROCESS; i++) {
        free(p[i].io_request_points);
    }
}
