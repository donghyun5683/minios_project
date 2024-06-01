#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "basic_include.h"

Page *physical_memory[NUM_FRAMES];
int frame_bitmap[NUM_FRAMES];
PCB *pcb_memory[PCB_FRAMES];
int pcb_frame_bitmap[PCB_FRAMES];

// schedular
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

            sleep(time_quantum);
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



void initialize_queue(CircularQueue *q) {
    q->front = -1;
    q->rear = -1;
}

int is_empty(CircularQueue *q) {
    return q->front == -1;
}

int is_full(CircularQueue *q) {
    return (q->rear + 1) % QUEUE_SIZE == q->front;
}

void enqueue(CircularQueue *q, int value) {
    if (is_full(q)) {
        printf("Queue is full\n");
        return;
    }
    if (is_empty(q)) {
        q->front = 0;
    }
    q->rear = (q->rear + 1) % QUEUE_SIZE;
    q->items[q->rear] = value;
}

int dequeue(CircularQueue *q) {
    if (is_empty(q)) {
        printf("Queue is empty\n");
        return -1;
    }
    int value = q->items[q->front];
    if (q->front == q->rear) {
        q->front = q->rear = -1;
    } else {
        q->front = (q->front + 1) % QUEUE_SIZE;
    }
    return value;
}


// paging

VirtualMemory* create_virtual_memory() {
    VirtualMemory *vm = (VirtualMemory*)malloc(sizeof(VirtualMemory));
    for (int i = 0; i < NUM_PAGES; i++) {
        vm->pages[i] = NULL;
    }
    return vm;
}

void create_page_table(Process *proc) {
    for (int i = 0; i < NUM_PAGES; i++) {
        proc->page_table[i] = (PageTableEntry*)malloc(sizeof(PageTableEntry));
        proc->page_table[i]->valid = 0;
        proc->page_table[i]->frame_number = -1;
    }
}

PCB* create_pcb(int pid) {
    PCB *pcb = (PCB*)malloc(sizeof(PCB));
    pcb->pid = pid;
    strncpy(pcb->state, "New", sizeof(pcb->state));
    pcb->arrival_time = rand()%4;
    pcb->burst_time = 2+rand()%6; // 기본 burst time을 10으로 설정
    pcb->completion_time = 0;
    pcb->waiting_time = 0;
    pcb->turnaround_time = 0;
    return pcb;
}

int allocate_pcb_frame() {
    for (int i = 0; i < PCB_FRAMES; i++) {
        if (pcb_frame_bitmap[i] == 0) {
            pcb_frame_bitmap[i] = 1;
            return i;
        }
    }
    return -1;
}

int allocate_frame() {
    int free_frames[NUM_FRAMES - PCB_FRAMES];
    int free_count = 0;

    for (int i = PCB_FRAMES; i < NUM_FRAMES; i++) {
        if (frame_bitmap[i] == 0) {
            free_frames[free_count++] = i;
        }
    }

    if (free_count == 0) {
        return -1;
    }

    printf("Available frames: ");
    for (int i = 0; i < free_count; i++) {
        printf("%d ", free_frames[i]);
    }
    printf("\n");

    int random_index = rand() % free_count;
    int frame_number = free_frames[random_index];
    frame_bitmap[frame_number] = 1;

    return frame_number;
}


//process create

Process* create_process(int pid,unsigned char *memory) {
    Process *proc = (Process*)malloc(sizeof(Process));
    proc->pid = pid;
    proc->vm = create_virtual_memory();
    create_page_table(proc);
    proc->pcb = create_pcb(pid);

    int pcb_frame = allocate_pcb_frame();
    if (pcb_frame == -1) {
        printf("No free PCB frames available\n");
        free(proc->pcb);
        free(proc);
        return NULL;
    }
    proc->pcb_frame_number = pcb_frame;
    pcb_memory[pcb_frame] = proc->pcb;

    // 실제 메모리 주소 계산
    unsigned char *pcb_memory_address = memory + (pcb_frame * PAGE_SIZE);

    printf("Process %d: Allocated PCB to PCB Frame %d at Memory Address %p\n", proc->pid, pcb_frame, pcb_memory_address);
    return proc;
}

void allocate_memory(Process *proc, int logical_page_id, unsigned char* memory) {
    if (logical_page_id >= NUM_PAGES) {
        printf("Logical Page ID out of bounds\n");
        return;
    }
    int frame_number = allocate_frame();
    if (frame_number == -1) {
        printf("No free frames available\n");
        return;
    }
    Page *page = (Page*)malloc(sizeof(Page));
    page->page_id = logical_page_id;
    snprintf(page->content, PAGE_SIZE, "Process %d, Page %d content", proc->pid, logical_page_id);
    physical_memory[frame_number] = page;

    proc->vm->pages[logical_page_id] = page;
    proc->page_table[logical_page_id]->valid = 1;
    proc->page_table[logical_page_id]->frame_number = frame_number;

    // 실제 메모리 주소 계산
    //unsigned char *frame_memory_address = memory + (frame_number * PAGE_SIZE);
    int memory_address = PHYSICAL_ADDRESS(frame_number);
    printf("Process %d: Allocated Page %d to Frame %d at Memory Address %p\n", proc->pid, logical_page_id, frame_number, memory + memory_address);

    printf("Process %d(Page ID %d): Virtual Address [0x%04x - 0x%04x] \n",
           proc->pid, logical_page_id, logical_page_id * PAGE_SIZE, (logical_page_id + 1) * PAGE_SIZE - 1);
}

void print_page_table(Process *proc) {
    printf("Process %d Page Table:\n", proc->pid);
    for (int i = 0; i < NUM_PAGES; i++) {
        if (proc->page_table[i]->valid) {
            printf("Logical Page %d -> Frame %d\n", i, proc->page_table[i]->frame_number);
        }
    }
}

void visualize_pcb(PCB *pcb, int frame_number) {
    printf("PCB in PCB Frame %d - PID: %d, State: %s\n", frame_number, pcb->pid, pcb->state);
}

void free_process(Process *proc) {
    for (int i = 0; i < NUM_PAGES; i++) {
        if (proc->vm->pages[i] != NULL) {
            int frame_number = proc->page_table[i]->frame_number;
            frame_bitmap[frame_number] = 0;
            free(physical_memory[frame_number]);
            proc->vm->pages[i] = NULL;
        }
        free(proc->page_table[i]);
    }

    pcb_frame_bitmap[proc->pcb_frame_number] = 0;
    free(proc->pcb);

    free(proc->vm);
    free(proc);
}

void print_frame_status() {
    printf("PCB Frames:\n");
    for (int i = 0; i < PCB_FRAMES; i++) {
        if (pcb_frame_bitmap[i]) {
            printf("PCB Frame %d: Occupied\n", i);
        } else {
            printf("PCB Frame %d: Free\n", i);
        }
    }

    printf("\nGeneral Frames:\n");
    for (int i = PCB_FRAMES; i < NUM_FRAMES; i++) {
        if (frame_bitmap[i]) {
            printf("Frame %d: Occupied\n", i);
        } else {
            printf("Frame %d: Free\n", i);
        }
    }
}
Process** initialize_processes(int* process_count, CircularQueue *queue, unsigned char *memory) {
    srand(time(NULL));
    int num;
    printf("\n프로세스 개수 입력: ");
   
    scanf("%d", &num) ; // 프로세스 개수 4개로 설정
    *process_count = num;
    for (int i = 0; i < NUM_FRAMES; i++) {
        physical_memory[i] = NULL;
        frame_bitmap[i] = 0;
    }

    for (int i = 0; i < PCB_FRAMES; i++) {
        pcb_memory[i] = NULL;
        pcb_frame_bitmap[i] = 0;
    }
   
    Process** processes = (Process**)malloc(*process_count * sizeof(Process*));

    for (int i = 0; i < *process_count; i++) {
        processes[i] = create_process(i + 1,memory);
        enqueue(queue, processes[i]->pcb_frame_number);
    }

    for(int i =0 ; i< *process_count; i++)
    {
        int page_num = 1+ rand()%4;
        for(int j =0; j < page_num; j++)
        {
            allocate_memory(processes[i],j,memory);
        }
       
    }

    printf("\n");
    print_frame_status();
    printf("\n");
    return processes;
}

