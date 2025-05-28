#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define NUM_PROCESS 20
#define MAX_ARRIVAL_TIME 10
#define MAX_CPU_BURST_TIME 20
#define MAX_IO_BURST_TIME 3
#define MAX_IO_REQUEST 4
#define NUM_IO_DEVICES 2
#define TIME_QUANTUM 2

#define NUM_MULTILEVEL 3
#define NUM_ALG 7


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
    short orig_queue;
    short age;
} Process;

typedef struct Node {
    Process* process;
    struct Node *prev;
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
void CompareScheduleAlgorithms();
void MLFQ_Scheduling(Process *p);
Node* NodeRemove(Node** queue, Node* node);
void QueueTraverse(Node** queue);

int unfinished = NUM_PROCESS;
Node *ready_queue = NULL;
Node *io_queue[NUM_IO_DEVICES] = {NULL, };
Node *heap_wait_queue = NULL;
Heap ready_heap = { NULL, };
double performance[NUM_ALG][2];
int idx = 0;
Node* MLFQ[NUM_MULTILEVEL] = { NULL, };

char reason[][40] = {
    "(Scheduling Start)",
    "(Process Finished)",
    "(I/O Requested)",
    "(Time Slice Expired)",
    "(Ready Queue Filled)",
    "(Preempted)",
    "(All Process Finished)",
    "(Higher Priority Queue Filled)"
};

/////////////////////////////////////////////////////////////////////////////////////////

// 전체적으로 주석 추가하기

void main(int argc, char* argv[]) {
    Process p[NUM_PROCESS];
    CreateProcess(p, argc, argv);
    PrintProcess(p);

    // Process arrival check overhead를 줄이기 위해 정렬
    qsort(p, NUM_PROCESS, sizeof(Process), CompareArrival);

    QueueBasedSchedule(p, 'F'); // FCFS Schedule
    QueueBasedSchedule(p, 'R'); // RR Schedule

    MinHeapBasedSchedule(p, 'S', false);    // nonpreemtive SJF
    MinHeapBasedSchedule(p, 'S', true);     // preemtive SJF
    MinHeapBasedSchedule(p, 'P', false);    // nonpreemptive Priority
    MinHeapBasedSchedule(p, 'P', true);    // preemptive Priority

    // EXTRA FUNCTIONS //
    MLFQ_Scheduling(p);
    // END //

    CompareScheduleAlgorithms();
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
        }
        p[i].pid = i;
        p[i].io_burst_time = rand() % MAX_IO_BURST_TIME + 1;
        p[i].remain_cpu_burst_time = p[i].cpu_burst_time;
        p[i].remain_io_burst_time = p[i].io_burst_time;
        p[i].waited_time = 0;
        p[i].finished_time = 0;
        p[i].orig_queue = 0;
        p[i].age = 0;
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
    double avg_wait = (double)sum_waiting / NUM_PROCESS;
    double avg_turnaround = (double)sum_turnaround / NUM_PROCESS;
    printf("\naverage waiting time: %.2lf\n", avg_wait);
    printf("average turnaround time: %.2lf\n", avg_turnaround);
    printf("==============================================\n\n");

    performance[idx][0] = avg_wait;
    performance[idx][1] = avg_turnaround;
    idx++;
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
                else if (mode == 'M')
                    QueuePush(&MLFQ[io_head->orig_queue], io_queue[i]->process);
                QueuePop(&io_queue[i]);
            }
        }
    }
}

void QueuePush(Node** queue, Process* process) {
    Node *new_node = malloc(sizeof(Node));
    new_node->process = process;
    new_node->prev = NULL;
    new_node->next = NULL;
    if (*queue == NULL) {
        *queue = new_node;
    }
    else {
        Node* cursor = *queue;
        while (cursor->next != NULL) 
            cursor = cursor->next;
        cursor->next = new_node;
        new_node->prev = cursor;
    }
    //QueueTraverse(queue);
}

Process* QueuePop(Node** queue) {
    if (*queue == NULL) {
        printf("Queue is Empty!\n");
        return NULL;
    }
    Process* out = (*queue)->process;
    Node* temp = (*queue);
    if ((*queue)->next != NULL) {
        (*queue)->next->prev = NULL;
        (*queue) = (*queue)->next;
    }
    else {
        (*queue) = NULL;
    }
    free(temp);
    //QueueTraverse(queue);
    return out;
}

/*///////////////////////////////////////////////////////////////////////////////////////
    # Scheduling Function for FCFS & Round Robin #

    - Pointer p points to an array of processes sorted by arrival time.
    - mode indicates whether it is FCFS or RR.
        | In the case of FCFS, set the time quantum greater than max_cpu_burst_time,
        | so that time slice expiration does not occur.
    - prev indicates pid of process run in previous time unit. initially -2, -1 if it was idle.
    - cur indicates pid of currently running process. -1 if idle.
        | if prev not equals cur, then context switch happened.
    - circum indicates circumstance of context switch
    - time_spent indicates the time of current process running continuously.
        | if time_spent equals timeQuantum, then context switch occur due to time slice expiration.
    - arrived indicates number of arrived process to the ready state for the first time

///////////////////////////////////////////////////////////////////////////////////////*/

void QueueBasedSchedule(Process *p, char mode) {
    int timeQuantum;
    if (mode == 'F') {
        printf("\nStart FCFS scheduling\n\n");
        timeQuantum = MAX_CPU_BURST_TIME + 1;
    }
    else if (mode == 'R') {
        printf("\nStart RR scheduling\n\n");
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
            if (prev == -1)
                printf("%3d |-------- %s\n", time, reason[4]);  // ready queue filled

            if (head->remain_cpu_burst_time == 0) {
                unfinished--;
                head->finished_time = time + 1;
                QueuePop(&ready_queue);
                time_spent = 0;
                circum = 1;     // process finished
                if (unfinished == 0) circum = 6;    // all process finished
            }
            else if (head->io_request_points[head->remain_io_request].point == head->remain_cpu_burst_time) {
                QueuePush(&io_queue[head->io_request_points[head->remain_io_request].device], head); // assign to required IO device wait queue
                QueuePop(&ready_queue);
                time_spent = 0;
                circum = 2; // I/O requested
            }
            else if (time_spent == timeQuantum) { // if mode == 'F', never satisfy this condition.
                QueuePop(&ready_queue);
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
            // slice expired인 경우 다음 반복에서 prev != cur를 반드시 참으로 하여 pid 출력하도록
            // expired 이후에 똑같은 프로세스 실행하더라도 출력.
            if (circum == 3) cur = -2;  
            circum = 0;
        }
        prev = cur;
        time++;
    }
    Evaluate(p);
    ResetProcess(p);
}

void MinHeapBasedSchedule(Process* p, char mode, bool preemption) {
    if (preemption == true) printf("\nStart preemptive ");
    else printf("\nStart nonpreemptive ");
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
            if (prev == -1)
                printf("%3d |-------- %s\n", time, reason[4]);  // ready queue filled
            else if (!popped && (prev != cur))
                printf("%3d |-------- %s\n", time, reason[5]);  // preempted

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
    // I/O request point와 device 출력하는 함수
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

void CompareScheduleAlgorithms() {
    char algorithms[NUM_ALG][25] = {
        "FCFS",
        "Round Robin",
        "non-pre SJF",
        "preemptive SJF",
        "non_pre priority",
        "preemptive priority",
        "MLFQ"
    };

    printf("-----------------------------------------------------------------\n");
    printf("| Algorithm           | Avg. waited time | Avg. turnaround time |\n");
    for (int i = 0; i < NUM_ALG; i++) {
        printf("|---------------------------------------------------------------|\n");
        printf("| %-19s | %16.3lf | %20.3lf |\n", algorithms[i], performance[i][0], performance[i][1]);
    }
    printf("-----------------------------------------------------------------\n");
}

void MLFQ_Scheduling(Process *p) {
    printf("\nStart Multi Level Feedback Queue Scheduling\n\n");
    
    int prev = -2;
    int cur = -1;
    int circum = 0;
    int time = 0;
    int time_spent = 0;
    int arrived = 0;
    int cur_queue = 0;
    int starvation = 100;
    int timeQuantum[NUM_MULTILEVEL] = {TIME_QUANTUM, TIME_QUANTUM * 2, MAX_CPU_BURST_TIME + 1};

    printf("time| Process\n");
    printf("%3d |-------- %s\n", time, reason[0]);

    while (unfinished > 0) {
        while (arrived < NUM_PROCESS) {
            if (p[arrived].arrival_time == time) {
                QueuePush(&MLFQ[0], &p[arrived]);
                arrived++;
            }
            else break;
        }
        for (int i = 0; i < 3; i++) {
            cur_queue = i;
            if (MLFQ[i] != NULL) break;
        }
        if (MLFQ[cur_queue] != NULL) {
            Process* head = MLFQ[cur_queue]->process;

            head->remain_cpu_burst_time--;
            time_spent++;
            
            for (int i = cur_queue; i < 3; i++) {
                Node* cursor;
                if (i == cur_queue)
                    cursor = MLFQ[i]->next;
                else
                    cursor = MLFQ[i];
                while (cursor != NULL) {
                    cursor->process->waited_time++;
                    cursor->process->age++;
                    if (i > 0 && cursor->process->age >= starvation) {

                        Process* temp = cursor->process;
                        Node* temp2 = cursor;
                    
                        cursor = cursor->next;
                        NodeRemove(&MLFQ[i], temp2);

                        temp->age = 0;
                        QueuePush(&MLFQ[i - 1], temp);
                    }
                    else
                        cursor = cursor->next;
                }
            }
            ProcessIO('M');

            cur = head->pid;
            if (prev == -1) printf("%3d |-------- %s (Queue: %d)\n", time, reason[4], cur_queue);  // ready queue filled

            if (head->remain_cpu_burst_time == 0) {
                unfinished--;
                head->finished_time = time + 1;
                QueuePop(&MLFQ[cur_queue]);
                time_spent = 0;
                circum = 1;         // process finished
                if (unfinished == 0)
                    circum = 6;     // all process finished
            }
            else if (head->io_request_points[head->remain_io_request].point == head->remain_cpu_burst_time) {
                head->orig_queue = cur_queue;
                QueuePush(&io_queue[head->io_request_points[head->remain_io_request].device], head);
                QueuePop(&MLFQ[cur_queue]);
                time_spent = 0;
                circum = 2;         // I/O requested
            }
            else if (time_spent == timeQuantum[cur_queue]) { // if mode == 'F', never satisfy this condition.
                QueuePop(&MLFQ[cur_queue]);
                if (cur_queue < NUM_MULTILEVEL - 1)
                    QueuePush(&MLFQ[cur_queue + 1], head);
                else
                    QueuePush(&MLFQ[cur_queue], head);
                time_spent = 0;
                circum = 3;         // time slice expired
            }
            else {
                for (int i = 0; i < cur_queue; i++) { // higher priority queue has been filled.
                    if (MLFQ[i] != NULL) {
                        QueuePop(&MLFQ[cur_queue]);
                        QueuePush(&MLFQ[cur_queue], head);
                        time_spent = 0;
                        circum = 7;
                        break;
                    }
                }
            }
        }
        else {
            ProcessIO('M');
            cur = -1;
        }
        if (prev != cur) {
            if (cur == -1)
                printf("    | idle\n");
            else
                printf("    | P_%d\n", cur);
        }
        if (circum) {
            printf("%3d |-------- %s (Queue: %d)\n", time + 1, reason[circum], cur_queue);
            if (circum == 3) cur = -2;  
            circum = 0;
        }
        prev = cur;
        time++;
    }
    Evaluate(p);
    ResetProcess(p);
}

Node* NodeRemove(Node** queue, Node* node) {
    if (node == NULL) return NULL;

    Node* next = node->next;

    if (node->prev == NULL) 
        *queue = node->next;
    else {
        node->prev->next = node->next;
    }
    if (node->next != NULL)
        node->next->prev = node->prev;
    
    free(node);
    //QueueTraverse(queue);
    return next;
}

void QueueTraverse(Node** queue) {
    if ((*queue) == NULL) return;
    Node* cursor = *queue;
    while (cursor != NULL) {
        printf("%d ", cursor->process->pid);
        cursor = cursor->next;
    }
    printf("\n");
}