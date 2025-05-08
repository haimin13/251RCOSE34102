#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NUM_PROCESS 10
#define MAX_ARRIVAL_TIME 15
#define MAX_CPU_BURST_TIME 20
#define MAX_IO_BURST_TIME 5

typedef struct {
    unsigned char pid;
    unsigned char priority;
    unsigned short arrival_time;
    unsigned short cpu_burst_time;
    unsigned short io_burst_time;
    unsigned short num_io_request_left;
    unsigned short *io_request_points;
    unsigned short remain_burst_time;
    unsigned short waited_time;
} Process;

int Max(int a, int b);
void PrintArray(unsigned short *arr, unsigned short size);
void PrintProcess(Process p);
void CreateProcess(Process *p);


void main() {
    Process p[NUM_PROCESS];
    CreateProcess(p);
    for (int i = 0; i < NUM_PROCESS; i++) {
        PrintProcess(p[i]);
    }

    for (int i = 0; i < NUM_PROCESS; i++) {
        free(p[i].io_request_points);
    }
    return;
}


int Max(int a, int b) {
    return (a > b) ? a : b;
}

void PrintArray(unsigned short *arr, unsigned short size) {
    if (arr == NULL) {
        printf("[ 0 ]\n");
    }
    else {
        printf("[ ");
        for (unsigned short i = 0; i < size; i++) {
            printf("%u ", arr[i]);
        }
        printf("]\n");
    }
    return;
}

void PrintProcess(Process p) {
    printf("pid: %u\n", p.pid);
    printf("priority: %u\n", p.priority);
    printf("arrival_time: %u\n", p.arrival_time);
    printf("cpu_burst_time: %u\n", p.cpu_burst_time);
    printf("io_burst_time: %u\n", p.io_burst_time);
    printf("num_io_request_left: %u\n", p.num_io_request_left);
    PrintArray(p.io_request_points, p.num_io_request_left + 1);
    printf("remain_burst_time: %u\n", p.remain_burst_time);
    printf("waited_time: %u\n", p.waited_time);
    printf("\n");
    return;
}

void CreateProcess(Process *p) {
    srand((unsigned int)time(NULL));
    int num_priority = NUM_PROCESS / 2;

    for (int i = 0; i < NUM_PROCESS; i++) {
        p[i].pid = i;
        p[i].priority = rand() % num_priority + 1;
        p[i].arrival_time = rand() % MAX_ARRIVAL_TIME;
        p[i].cpu_burst_time = p[i].remain_burst_time = rand() % MAX_CPU_BURST_TIME + 1;
        p[i].io_burst_time = rand() % MAX_IO_BURST_TIME + 1;
        p[i].waited_time = 0;

        p[i].num_io_request_left = 0;
        p[i].io_request_points = NULL;
        if (p[i].cpu_burst_time > 1) {
            int num_io_request = rand() % Max(1, p[i].cpu_burst_time / 2); // 0번에서 max(1, cpu_burst_time / 2)번 발생.
            p[i].num_io_request_left = num_io_request;
            p[i].io_request_points = malloc(sizeof(short) * (num_io_request + 1)); // 계산 편의를 위해 인덱스 0에 가비지 값.
            p[i].io_request_points[0] = 0;
            int temp[p[i].cpu_burst_time];
            memset(temp, 0, sizeof(temp));
            int j = 0;
            while(j < num_io_request) {
                int point = rand() % (p[i].cpu_burst_time - 1) + 1;
                if (temp[point] == 0) {
                    temp[point] = 1;
                    j++;
                }
            }
            for (j = p[i].cpu_burst_time - 1; j > 0; j--) {
                if (temp[j] == 1) {
                    p[i].io_request_points[num_io_request--] = j;
                }
            }
        }
    }
    return;
}
