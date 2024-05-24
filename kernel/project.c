#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PAGE_SIZE 256   // 각 페이지의 크기
#define VM_SIZE 1024    // 가상 메모리 크기 (바이트 단위)
#define NUM_PAGES (VM_SIZE / PAGE_SIZE)  // 페이지 수
#define PHYSICAL_MEMORY_SIZE 2048 // 물리 메모리 크기 (바이트 단위)
#define NUM_FRAMES (PHYSICAL_MEMORY_SIZE / PAGE_SIZE) // 물리 메모리 프레임 수

// 페이지 구조체
typedef struct {
    int page_id;
    char content[PAGE_SIZE];
} Page;

// 가상 메모리 구조체
typedef struct {
    Page *pages[NUM_PAGES];
} VirtualMemory;

// 페이지 테이블 엔트리 구조체
typedef struct {
    int valid;       // 유효 비트
    int frame_number; // 물리적 프레임 번호
} PageTableEntry;

// 프로세스 구조체
typedef struct {
    int pid;
    VirtualMemory *vm;
    PageTableEntry *page_table[NUM_PAGES];
} Process;

// 물리 메모리 관리
Page *physical_memory[NUM_FRAMES];
int frame_bitmap[NUM_FRAMES];

// 가상 메모리 초기화 함수
VirtualMemory* create_virtual_memory() {
    VirtualMemory *vm = (VirtualMemory*)malloc(sizeof(VirtualMemory));
    for (int i = 0; i < NUM_PAGES; i++) {
        vm->pages[i] = NULL;
    }
    return vm;
}

// 페이지 테이블 초기화 함수
void create_page_table(Process *proc) {
    for (int i = 0; i < NUM_PAGES; i++) {
        proc->page_table[i] = (PageTableEntry*)malloc(sizeof(PageTableEntry));
        proc->page_table[i]->valid = 0; // 초기화 시 모든 엔트리는 유효하지 않음
        proc->page_table[i]->frame_number = -1;
    }
}

// 프로세스 생성 함수
Process* create_process(int pid) {
    Process *proc = (Process*)malloc(sizeof(Process));
    proc->pid = pid;
    proc->vm = create_virtual_memory();
    create_page_table(proc);
    return proc;
}

// 프레임 할당 함수
int allocate_frame() {
    int free_frames[NUM_FRAMES];
    int free_count = 0;

    // 사용 가능한 프레임 목록 작성
    for (int i = 0; i < NUM_FRAMES; i++) {
        if (frame_bitmap[i] == 0) {
            free_frames[free_count++] = i;
        }
    }

    if (free_count == 0) {
        return -1; // 사용 가능한 프레임이 없음
    }

    // 사용 가능한 프레임 목록 출력
    printf("Available frames: ");
    for (int i = 0; i < free_count; i++) {
        printf("%d ", free_frames[i]);
    }
    printf("\n");

    // 랜덤한 프레임 선택
    int random_index = rand() % free_count;
    int frame_number = free_frames[random_index];
    frame_bitmap[frame_number] = 1;

    return frame_number;
}

// 메모리 할당 함수
void allocate_memory(Process *proc, int logical_page_id, const char *content) {
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
    strncpy(page->content, content, PAGE_SIZE);
    physical_memory[frame_number] = page;

    proc->vm->pages[logical_page_id] = page;
    proc->page_table[logical_page_id]->valid = 1;
    proc->page_table[logical_page_id]->frame_number = frame_number;

    printf("Process %d: Allocated Page %d to Frame %d\n", proc->pid, logical_page_id, frame_number);

    printf("Process %d(Page ID %d): Virtual Address [0x%04x - 0x%04x] \n",
           proc->pid, logical_page_id, logical_page_id * PAGE_SIZE, (logical_page_id + 1) * PAGE_SIZE - 1);
}

// 가상 주소 시각적 표시 함수
void print_virtual_address(Process *proc, unsigned int virtual_address) {
    int logical_page_id = (virtual_address / PAGE_SIZE) % NUM_PAGES;
    int offset = virtual_address % PAGE_SIZE;

    if (logical_page_id >= NUM_PAGES || proc->page_table[logical_page_id]->valid == 0) {
        printf("Logical Page ID out of bounds or page not allocated\n");
        return;
    }

    printf("Process %d: Virtual Address 0x%04x (VPN: 0x%02x, Offset: 0x%02x)\n",
           proc->pid, virtual_address, logical_page_id, offset);
}

// 물리 주소 시각적 표시 함수
void print_physical_address(Process *proc, unsigned int virtual_address) {
    int logical_page_id = (virtual_address / PAGE_SIZE) % NUM_PAGES;
    int offset = virtual_address % PAGE_SIZE;

    if (logical_page_id >= NUM_PAGES || proc->page_table[logical_page_id]->valid == 0) {
        printf("Logical Page ID out of bounds or page not allocated\n");
        return;
    }

    int frame_number = proc->page_table[logical_page_id]->frame_number;
    unsigned int physical_address = frame_number * PAGE_SIZE + offset;

    printf("Process %d: Virtual Address 0x%04x -> Physical Address 0x%04x (PFN: 0x%02x, Offset: 0x%02x)\n",
           proc->pid, virtual_address, physical_address, frame_number, offset);
}

// 메모리 접근 함수
void access_memory(Process *proc, unsigned int virtual_address) {
    int logical_page_id = (virtual_address / PAGE_SIZE) % NUM_PAGES;
    int offset = virtual_address % PAGE_SIZE;

    if (logical_page_id >= NUM_PAGES || proc->page_table[logical_page_id]->valid == 0) {
        printf("Logical Page ID out of bounds or page not allocated\n");
        return;
    }

    int frame_number = proc->page_table[logical_page_id]->frame_number;
    Page *page = physical_memory[frame_number];
    printf("Process %d: Accessing Virtual Address 0x%04x (Page %d, Frame %d, Offset 0x%02x) - Content: %c\n",
           proc->pid, virtual_address, logical_page_id, frame_number, offset, page->content[offset]);
}

// 페이지 테이블 출력 함수
void print_page_table(Process *proc) {
    printf("Process %d Page Table:\n", proc->pid);
    for (int i = 0; i < NUM_PAGES; i++) {
        if (proc->page_table[i]->valid) {
            printf("Logical Page %d -> Frame %d\n", i, proc->page_table[i]->frame_number);
        }
    }
}

// 메모리 해제 함수
void free_process(Process *proc) {
    for (int i = 0; i < NUM_PAGES; i++) {
        if (proc->vm->pages[i] != NULL) {
            int frame_number = proc->page_table[i]->frame_number;
            frame_bitmap[frame_number] = 0; // 프레임 해제
            free(physical_memory[frame_number]);
            proc->vm->pages[i] = NULL;
        }
        free(proc->page_table[i]);
    }
    free(proc->vm);
    free(proc);
}

int project() {
    srand(time(NULL)); // 랜덤 시드 초기화

    // 물리 메모리 초기화
    for (int i = 0; i < NUM_FRAMES; i++) {
        physical_memory[i] = NULL;
        frame_bitmap[i] = 0; // 모든 프레임을 사용하지 않은 상태로 초기화
    }

    // 프로세스 생성
    Process *proc_add = create_process(1);
    Process *proc_sub = create_process(2);

    // 덧셈 프로세스 메모리 할당
    allocate_memory(proc_add, 0, "Add Result: 5 + 3 = 8");
    allocate_memory(proc_add, 1, "Add Result: 10 + 2 = 12");

    // 페이지 테이블 출력
    printf("\n");
    print_page_table(proc_add);

    printf("\n");
    // 뺄셈 프로세스 메모리 할당
    allocate_memory(proc_sub, 0, "Sub Result: 9 - 4 = 5");
    allocate_memory(proc_sub, 1, "Sub Result: 15 - 6 = 9");

    printf("\n");
    print_page_table(proc_sub);
    printf("\n");
    // 가상 주소 시각적 표시
    print_virtual_address(proc_add, 0x000);
    print_virtual_address(proc_add, 0x100);
    print_virtual_address(proc_sub, 0x000);
    print_virtual_address(proc_sub, 0x100);

    // 물리 주소 시각적 표시
    print_physical_address(proc_add, 0x000);
    print_physical_address(proc_add, 0x100);
    print_physical_address(proc_sub, 0x000);
    print_physical_address(proc_sub, 0x100);

    // 메모리 접근
    access_memory(proc_add, 0x000);
    access_memory(proc_add, 0x100);
    access_memory(proc_sub, 0x000);
    access_memory(proc_sub, 0x100);

    // 프로세스 해제
    free_process(proc_add);
    free_process(proc_sub);

    return 0;
}

