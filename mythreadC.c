//
// Created by pengf on 2024/11/21.
//
#include "mythreadC.h"
#include <unistd.h>
int NUMEBER = 2;
void addTask(ThreadPool* threadpool, void(* func)(void*), void* arg) {
    pthread_mutex_lock(&threadpool->mutexPool);
    while (threadpool->queueSzie == threadpool->queueCapacity && !threadpool->shutdown) {
        // 阻塞生产者线程（不让生产者线程阻塞任务队列）
        pthread_cond_wait(&threadpool->notFull, &threadpool->mutexPool);
    }
    if (threadpool->shutdown) {
        pthread_mutex_unlock(&threadpool->mutexPool);
        return;
    }

    // 添加任务
    threadpool->taskQ[threadpool->queueRear].function = func;
    threadpool->taskQ[threadpool->queueRear].arg = arg;
    threadpool->queueRear = (threadpool->queueRear + 1) % threadpool->queueCapacity;
    threadpool->queueSzie++;

    pthread_cond_signal(&threadpool->notEmpty);
    pthread_mutex_unlock(&threadpool->mutexPool);
}


ThreadPool* threadpoolCreate(int min, int max, int queuesize)
{
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    do {
        if(pool == NULL){
            printf("创建线程池失败！");
            break;
        }

        pool->threadIDs = (pthread_t*) malloc(sizeof(pthread_t) * max);
        if(pool->threadIDs == NULL) {
            printf("创建线程池失败！");
            break;
        }
        memset(pool->threadIDs, 0, sizeof(pthread_t)*max); // 初始化线程池中的管理线程队列

        pool->minNum = min;
        pool->maxNum = max;
        pool->busyNum = 0;
        pool->liveNum = min;
        pool->exitNum = 0;

        // 初始化锁和条件变量
        if (pthread_mutex_init(&pool->mutexBusy, NULL)  ||
            pthread_mutex_init(&pool->mutexPool, NULL) ||
            pthread_cond_init(&pool->notEmpty, NULL) ||
            pthread_cond_init(&pool->notFull, NULL)) {
            printf("创建线程失败！");
            break;
        }

        // 初始化任务队列
        pool->taskQ = (Task*)malloc(sizeof(Task) * queuesize);
        pool->queueCapacity = queuesize;
        pool->queueFront = 0;
        pool->queueRear = 0;
        pool->queueSzie = 0;

        pool->shutdown = 0;

        // 初始化线程（创建线程）
        pthread_create(&pool->managerID, NULL, manager, pool);
        for(int i = 0; i < min; i++) {
            pthread_create(&pool->threadIDs[i], NULL, worker, pool);
        }
        return pool;
    }while(0);

    if(pool && pool->threadIDs) free(pool->threadIDs);
    if(pool && pool->taskQ) free(pool->taskQ);
    if(pool) free(pool);

    return NULL;
}

int threadPoolDestory(ThreadPool* pool) {
    if (pool == NULL) {
        return -1;
    }

    pool->shutdown = 1; // 关闭线程池
    // 阻塞回收管理者线程
    pthread_join(pool->managerID, NULL);

    // 唤醒消费者线程
    for(int i = 0; i < pool->liveNum; i++) {
        pthread_cond_signal(&pool->notEmpty);
    }

    // 释放堆内存
    if(pool->taskQ) {
        free(pool->taskQ);
    }
    if(pool->threadIDs) {
        free(pool->threadIDs);
    }

    // 释放信号量和锁
    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);

    free(pool);
    pool = NULL;

    return 0;
}

int threadPoolBusyNumber(ThreadPool* pool) {
    pthread_mutex_lock(&pool->mutexBusy);
    int busyNumber = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexBusy);

    return busyNumber;
}

// 获取线程池中活着的线程个数
int threadPoolAliveNumber(ThreadPool* pool) {
    pthread_mutex_lock(&pool->mutexPool);
    int aliveNumber = pool->liveNum;
    pthread_mutex_unlock(&pool->mutexPool);

    return aliveNumber;
}

void* worker(void* arg) {
    ThreadPool* threadPool = (ThreadPool*)arg;
    while(1) {
        // 从线程池里拿数据
        pthread_mutex_lock(&threadPool->mutexPool);
        // 线程池任务数量为0且线程池开着的
        while(threadPool->queueSzie == 0 && !threadPool->shutdown) {
            // 阻塞工作线程
            pthread_cond_wait(&threadPool->notEmpty, &threadPool->mutexPool);
            // 线程自杀
            if (threadPool->exitNum > 0) {
                threadPool->exitNum--;
                if(threadPool->liveNum > threadPool->minNum) {
                    threadPool->liveNum--;
                    pthread_mutex_unlock(&threadPool->mutexPool);
                    threadExit(threadPool);
                }
            }
        }

        if(threadPool->shutdown) {
            pthread_mutex_unlock(&threadPool->mutexPool);
            threadExit(threadPool);
        }

        // 从任务队列中取出数据
        Task task;
        task.function = threadPool->taskQ[threadPool->queueFront].function;
        task.arg = threadPool->taskQ[threadPool->queueFront].arg;

        threadPool->queueSzie--;
        threadPool->queueFront = (threadPool->queueFront + 1) % threadPool->queueCapacity;

        pthread_cond_signal(&threadPool->notFull);
        pthread_mutex_unlock(&threadPool->mutexPool);

        pthread_mutex_lock(&threadPool->mutexBusy);
        threadPool->busyNum++;
        pthread_mutex_unlock(&threadPool->mutexBusy);

        task.function(task.arg);    // 执行函数
        free(task.arg);
        task.arg = NULL;

        pthread_mutex_lock(&threadPool->mutexBusy);
        threadPool->busyNum--;
        pthread_mutex_unlock(&threadPool->mutexBusy);
    }
    return NULL;
}

void* manager(void* arg) {
    ThreadPool* threadPool = (ThreadPool*)arg;
    while (!threadPool->shutdown) {
        // 看是需要创建线程
        // 每间隔3秒钟检测一次
        sleep(3);

        // 取出线程池中任务数量和线程数量、忙着的线程数量
        pthread_mutex_lock(&threadPool->mutexPool);
        int taskNum = threadPool->queueSzie;
        int liveNum = threadPool->liveNum;
        int busyNum = threadPool->busyNum;
        pthread_mutex_unlock(&threadPool->mutexPool);

        // 满足一定条件时，才可以添加线程，条件可以自己设置
        if(taskNum > liveNum && busyNum < threadPool->maxNum) {
            int counter = 0;    //  添加线程的次数
            pthread_mutex_lock(&threadPool->mutexPool);
            for (int i = 0; i < threadPool->maxNum && counter < NUMEBER && threadPool->liveNum < threadPool->maxNum; i++) {
                if (threadPool->threadIDs[i] == 0) {
                    pthread_create(&threadPool->threadIDs[i], NULL, worker, threadPool);
                    counter++;
                    threadPool->liveNum++;
                }
            }
            pthread_mutex_unlock(&threadPool->mutexPool);

        }

        // 看是否需要销毁线程,也是需要一定的条件
        if (busyNum * 2 < liveNum && liveNum > threadPool->minNum) {
            pthread_mutex_lock(&threadPool->mutexPool);
            threadPool->exitNum = NUMEBER;
            pthread_mutex_unlock(&threadPool->mutexPool);
            // 唤醒因为任务数量为0而阻塞的线程，让这些线程自杀
            for(int i = 0; i < NUMEBER; i++) {
                pthread_cond_signal(&threadPool->notEmpty);
            }
        }
    }
    return NULL;
}

void threadExit(ThreadPool* threadPool) {
    pthread_t tid = pthread_self();
    for(int i = 0; i < threadPool->maxNum; i++) {
        if (tid == threadPool->threadIDs[i]) {
            threadPool->threadIDs[i] = 0;
            break;
        }
    }
    printf("=====================线程 %ld 退出=====================\n", pthread_self());
    pthread_exit(NULL);
}