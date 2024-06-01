#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define PAGE_SIZE 128
#define VM_SIZE 4096
#define NUM_PAGES (VM_SIZE / PAGE_SIZE)
#define PCB_FRAMES 10
#define QUEUE_SIZE 10
#define PHYSICAL_MEMORY_SIZE 4096
#define TIME_QUANTUM 2
#define PHYSICAL_ADDRESS(frame_number) ((frame_number) * PAGE_SIZE)
#define NUM_FRAMES (PHYSICAL_MEMORY_SIZE / PAGE_SIZE)


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

typedef struct {
    int process_count;
    CircularQueue queue;
    Process **processes;

} Paging_return;





void print_queue(CircularQueue *queue);


void round_robin(Process** processes, int process_count, int time_quantum, CircularQueue *queue, unsigned char *memory) ;

void print_process_info(Process** processes, int process_count) ;

// 실제 메모리 주소를 위한 배열
//unsigned char memory[PHYSICAL_MEMORY_SIZE];

void initialize_queue(CircularQueue *q) ;

int is_empty(CircularQueue *q) ;
int is_full(CircularQueue *q) ;
void enqueue(CircularQueue *q, int value) ;
int dequeue(CircularQueue *q) ;

VirtualMemory* create_virtual_memory() ;

Paging_return paging (unsigned char *memory);
void schedular(unsigned char *memory ,Paging_return result);



void create_page_table(Process *proc) ;
PCB* create_pcb(int pid) ;

int allocate_pcb_frame() ;

int allocate_frame() ;
Process* create_process(int pid,unsigned char *memory) ;
void allocate_memory(Process *proc, int logical_page_id, unsigned char* memory) ;

void print_page_table(Process *proc) ;
void visualize_pcb(PCB *pcb, int frame_number) ;
void free_process(Process *proc) ;

void print_frame_status() ;
Process** initialize_processes(int* process_count, CircularQueue *queue, unsigned char *memory) ;

