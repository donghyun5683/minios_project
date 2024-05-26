// schedular.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PAGE_SIZE 128
#define VM_SIZE 1024
#define NUM_PAGES (VM_SIZE / PAGE_SIZE)
#define PCB_FRAMES 4
#define QUEUE_SIZE 10
#define TIME_QUANTUM 1
#define PHYSICAL_ADDRESS(frame_number) ((frame_number) * PAGE_SIZE)

typedef struct {
    int page_id;
    char content[PAGE_SIZE];
} Page;

typedef struct {
    Page *pages[NUM_PAGES];
} VirtualMemory;

typedef struct {
    int valid;
    int frame_number;
} PageTableEntry;

typedef struct {
    int pid;
    char state[16];
    int arrival_time;
    int burst_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
} PCB;

typedef struct {
    int pid;
    VirtualMemory *vm;
    PageTableEntry *page_table[NUM_PAGES];
    PCB *pcb;
    int pcb_frame_number;
} Process;

typedef struct {
    int items[QUEUE_SIZE];
    int front;
    int rear;
} CircularQueue;

extern Process** initialize_processes(int* process_count, CircularQueue *queue, unsigned char *memory);
extern  void print_frame_status();
extern void free_process(Process *proc);
extern void print_page_table(Process *proc);

void print_queue(CircularQueue *queue) {
    if (is_empty(queue)) {
        printf("Queue is empty\n");
        return;
    }

    printf("Circular Queue: ");
    int i = queue->front;
    while (1) {
        printf("%d ", queue->items[i]);
        if (i == queue->rear) break;
        i = (i + 1) % QUEUE_SIZE;
    }
    printf("\n");
    printf("\n");
}

void round_robin(Process** processes, int process_count, int time_quantum, CircularQueue *queue, unsigned char *memory) {
    int remaining_burst_times[process_count];
    for (int i = 0; i < process_count; i++) {
        remaining_burst_times[i] = processes[i]->pcb->burst_time;
    }

    int current_time = 0;
    int remaining_processes = process_count;

    while (remaining_processes > 0) {
        int pcb_frame_number = dequeue(queue);
        if (pcb_frame_number == -1) {
            printf("No PCB frame to schedule\n");
            return;
        }

        Process *proc = NULL;
        for (int i = 0; i < process_count; i++) {
            if (processes[i]->pcb_frame_number == pcb_frame_number) {
                proc = processes[i];
                break;
            }
        }

        if (proc == NULL) {
            printf("No process found for PCB frame %d\n", pcb_frame_number);
            return;
        }

        if (remaining_burst_times[proc->pid - 1] > 0) {
            int frame_number = proc->pcb_frame_number;
            int memory_address = PHYSICAL_ADDRESS(frame_number);

            printf("Time %d: Running process %d, Remaining Burst Time: %d, Frame Number: %d, Memory Address: %p\n",
                   current_time, proc->pid, remaining_burst_times[proc->pid - 1], frame_number, memory + memory_address);

            // sleep(time_quantum);
            current_time += time_quantum;
            remaining_burst_times[proc->pid - 1] -= time_quantum;

            if (remaining_burst_times[proc->pid - 1] <= 0) {
                processes[proc->pid - 1]->pcb->completion_time = current_time;
                processes[proc->pid - 1]->pcb->turnaround_time = current_time - processes[proc->pid - 1]->pcb->arrival_time;
                processes[proc->pid - 1]->pcb->waiting_time = processes[proc->pid - 1]->pcb->turnaround_time - processes[proc->pid - 1]->pcb->burst_time;
                printf("Process %d finished at time %d\n", proc->pid, current_time);
                remaining_processes--;
            } else {
                enqueue(queue, pcb_frame_number);
            }
        }
    }
}

void print_process_info(Process** processes, int process_count) {
    printf("\nProcess\tArrival Time\tBurst Time\tCompletion Time\tWaiting Time\tTurnaround Time\n");
    for (int i = 0; i < process_count; i++) {
        PCB *pcb = processes[i]->pcb;
        printf("%d\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n",
               pcb->pid,
               pcb->arrival_time,
               pcb->burst_time,
               pcb->completion_time,
               pcb->waiting_time,
               pcb->turnaround_time);
    }
}

int process(unsigned char *memory) {
    CircularQueue queue;
    initialize_queue(&queue);

    int process_count;
    Process** processes = initialize_processes(&process_count, &queue, memory);

    // 큐의 내용을 출력
    print_queue(&queue);

    // 각 프로세스의 페이지 테이블 출력
    for (int i = 0; i < process_count; i++) {
        print_page_table(processes[i]);
    }

    round_robin(processes, process_count, TIME_QUANTUM, &queue, memory);

    print_process_info(processes, process_count);

    for (int i = 0; i < process_count; i++) {
        free_process(processes[i]);
    }
    free(processes);

    return 0;
}

