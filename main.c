#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#define NUM_PROCESS 10
#define MAX_ARRIVAL_TIME 15
#define MAX_CPU_BURST_TIME 20
#define MAX_IO_BURST_TIME 5
#define NUM_IO_DEVICES 2


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
} Process;

typedef struct {
    Process* process;
    struct Node *next;
} Node;


int Max(int a, int b);
void PrintArray(IORequest *arr, short size);
void PrintProcess(Process *p);
void FreeMemory(Process *p);
void CreateProcess(Process *p, int argc, char* argv[]);
void ManualCreate(Process *p, int idx);
void FCFS(Process *p);
void SJF(Process *p, bool preemptive);
void Priority(Process *p, bool preemptive);
void RR(Process *p);

int unfinished = NUM_PROCESS;
Node *ready_queue_head = NULL;
Node ready_heap_head[NUM_PROCESS];
Node *io_wait_head[NUM_IO_DEVICES] = {NULL, };


void main(int argc, char* argv[]) {
    Process p[NUM_PROCESS];
    CreateProcess(p, argc, argv);
    // Process를 arrival time순(FCFS)이나 burst time순(SJF) 혹은 priority순으로 정렬해놔도 좋겠다.
    PrintProcess(p);


    FCFS(p);
    SJF(p, false);         // nonpreemtive SJF
    SJF(p, true);          // preemtive SJF
    Priority(p, false);    // nonpreemptive Priority
    Priority(p, true);     // preemptive Priority
    RR(p);

    FreeMemory(p);
    return;
}

void CreateProcess(Process *p, int argc, char* argv[]) {
    srand((unsigned int)time(NULL));
    int num_priority = NUM_PROCESS / 2;

    for (int i = 0; i < NUM_PROCESS; i++) {
        if (argc > 1 && strcmp(argv[1], "manual") == 0) {
            ManualCreate(p, i);
        }
        else {
            p[i].pid = i;
            p[i].priority = rand() % num_priority + 1;
            p[i].arrival_time = rand() % MAX_ARRIVAL_TIME;
            p[i].cpu_burst_time = rand() % MAX_CPU_BURST_TIME + 1;
            p[i].io_burst_time = rand() % MAX_IO_BURST_TIME + 1;
        }
        p[i].remain_cpu_burst_time = p[i].cpu_burst_time;
        p[i].remain_io_burst_time = p[i].io_burst_time;
        p[i].waited_time = 0;
        if (p[i].cpu_burst_time > 1) {
            int num_io_request = rand() % (Max(1, p[i].cpu_burst_time / 2) + 1); // 0번에서 max(1, cpu_burst_time / 2)번 발생.
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
                    p[i].io_request_points[num_io_request].device = rand() % NUM_IO_DEVICES;
                    num_io_request--;
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
    while(true) {
        int duplicate = 0;
        printf("Input PID: ");
        short pid;
        scanf("%hd", &pid);
        for (int j = 0; j < i; j++) {
            if (pid == p[j].pid) {
                duplicate = 1;
                printf("duplicated PID! Choose another pid\n");
                break;
            }
        }
        if (duplicate == 0) {
            p[i].pid = pid;
            break;
        }
    }
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

void FCFS(Process *p) {
    int i = 0;
    Node* head = ready_queue_head;
    while (unfinished > 0) {
        for (int j = 0; j < NUM_PROCESS; j++) {
            if (p[j].arrival_time == i) 
                Push(head, &p[j]);
        }
        if (head != NULL) {
            head->process->remain_cpu_burst_time--;
        }
    }
}
void SJF(Process *p, bool preemptive);
void Priority(Process *p, bool preemptive);
void RR(Process *p);

int Max(int a, int b) {
    return (a > b) ? a : b;
}

void PrintArray(IORequest *arr, short size) {
    printf("[ ");
    for (short i = 1; i < size; i++) {
        printf("(%d, #%d) ", arr[i].point, arr[i].device);
    }
    printf("]\n");
}

void PrintProcess(Process *p) {
    for (int i = 0; i < NUM_PROCESS; i++) {
        printf("pid: %d\n", p[i].pid);
        printf("priority: %d\n", p[i].priority);
        printf("arrival_time: %d\n", p[i].arrival_time);
        printf("cpu_burst_time: %d\n", p[i].cpu_burst_time);
        printf("io_burst_time: %d\n", p[i].io_burst_time);
        printf("num_io_request: %d\n", p[i].num_io_request);
        printf("io_request_points: ");
        PrintArray(p[i].io_request_points, p[i].num_io_request + 1);
        printf("remain_io_request: %d\n", p[i].remain_io_request);
        printf("remain_cpu_burst_time: %d\n", p[i].remain_cpu_burst_time);
        printf("remain_io_burst_time: %d\n", p[i].remain_io_burst_time);
        printf("waited_time: %d\n", p[i].waited_time);
        printf("\n");
    }
}

void FreeMemory(Process *p) {
    for (int i = 0; i < NUM_PROCESS; i++) {
        free(p[i].io_request_points);
    }
}
