//
// Created by pengf on 2024/11/19.
//
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


// 任务结构体
typedef struct Task {
    void(* function)(void* arg);    // 任务函数指针
    void* arg;  // 任务函数的参数
}Task;

// 线程池结构
typedef struct threadPool {
    // 任务队列
    Task* taskQ;
    int queueCapacity;  // 容量, 任务队列更具容量分配--》容量需要指定
    int queueSzie;  // 当前任务个数
    int queueFront; // 对头 -> 取数据
    int queueRear;  // 对尾 -> 存数据

    pthread_t managerID;    // 管理者线程ID
    pthread_t *threadIDs;   // 工作的线程ID
    int minNum;             // 最小线程数---》需要用户传递
    int maxNum;             // 最大线程数---》需要用户传递
    int busyNum;            // 忙的线程个数
    int liveNum;            // 存活的线程个数
    int exitNum;            // 要销毁的线程个数
    pthread_mutex_t mutexPool;  // 锁整个线程池
    pthread_mutex_t mutexBusy;  // 锁busyNum变量

    int shutdown;   // 为1表示要将线程池销毁

    pthread_cond_t notFull;     // 线程池是不是满了
    pthread_cond_t notEmpty;    // 线程池是不是空的
}ThreadPool;

// 创建线程池的函数（初始化）
ThreadPool* threadpoolCreate(int min, int max, int queuesize);

// 销毁线程池的函数
int threadPoolDestory(ThreadPool* pool);

// 添加任务的函数
void addTask(ThreadPool* threadpool, void(* func)(void*), void* arg);

// 获取线程池中的线程个数
int threadPoolBusyNumber(ThreadPool* pool);

// 获取线程池中活着的线程个数
int threadPoolAliveNumber(ThreadPool* pool);

// 消费者任务函数
void* worker(void* arg);

// 管理者线程任务函数
void* manager(void* arg);




void threadExit(ThreadPool* threadPool);



