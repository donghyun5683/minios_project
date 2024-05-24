#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMORY_SIZE 1024  // 메모리 크기 (바이트 단위)

// 메모리 배열 선언
unsigned char memory[MEMORY_SIZE];

// 메모리에 쓰기 함수
void write_memory(unsigned int address, unsigned char value) {
    if (address >= MEMORY_SIZE) {
        printf("Error: Address out of bounds\n");
        return;
    }
    memory[address] = value;
}

// 메모리에서 읽기 함수
unsigned char read_memory(unsigned int address) {
    if (address >= MEMORY_SIZE) {
        printf("Error: Address out of bounds\n");
        return 0;
    }
    return memory[address];
}
