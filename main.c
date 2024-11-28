#include <stdio.h>
#include "mythreadC.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void testFunc(void* arg) {
    int num = *(int*)arg;
    printf("线程id为为：%ld， tid为：%ld\n", pthread_self(), num);
    sleep(1);
}

int main(void) {
    ThreadPool* pool = threadpoolCreate(3, 10, 100);
    for(int i = 0; i < 100; i++) {
        int* num = (int*)malloc(sizeof(int));
        *num = i + 10;
        addTask(pool, testFunc, num);
    }

    sleep(30);
    threadPoolDestory(pool);
    return 0;
}
