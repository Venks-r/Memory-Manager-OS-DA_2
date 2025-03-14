
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>

#define TOTAL_MEMORY 20
#define MAX_PROCESSES 10

typedef struct {
    int size;
    int free;
    int process_id;
} MemoryBlock;

MemoryBlock memory[TOTAL_MEMORY];
pthread_mutex_t memory_lock;

void init_memory() {
    for (int i = 0; i < TOTAL_MEMORY; i++) {
        memory[i].size = 1;
        memory[i].free = 1;
        memory[i].process_id = -1;
    }
    pthread_mutex_init(&memory_lock, NULL);
}

int allocate_memory(int process_id, int size) {
    pthread_mutex_lock(&memory_lock);
    int allocated = 0;
    for (int i = 0; i < TOTAL_MEMORY && allocated < size; i++) {
        if (memory[i].free) {
            memory[i].free = 0;
            memory[i].process_id = process_id;
            allocated++;
        }
    }
    pthread_mutex_unlock(&memory_lock);
    return allocated == size; // Return success if full allocation is done
}

void deallocate_memory(int process_id) {
    pthread_mutex_lock(&memory_lock);
    for (int i = 0; i < TOTAL_MEMORY; i++) {
        if (memory[i].process_id == process_id) {
            memory[i].free = 1;
            memory[i].process_id = -1;
        }
    }
    pthread_mutex_unlock(&memory_lock);
}

void garbage_collect() {
    pthread_mutex_lock(&memory_lock);
    for (int i = 0; i < TOTAL_MEMORY; i++) {
        if (!memory[i].free) {
            memory[i].free = 1;
            memory[i].process_id = -1;
        }
    }
    pthread_mutex_unlock(&memory_lock);
}

typedef struct {
    int pid;
    int priority;
    int burst_time;
    time_t arrival_time;
} Process;

Process process_queue[MAX_PROCESSES];
int process_count = 0;
pthread_mutex_t queue_lock;

void add_process(int pid, int priority, int burst_time) {
    pthread_mutex_lock(&queue_lock);
    if (process_count < MAX_PROCESSES) {
        process_queue[process_count].pid = pid;
        process_queue[process_count].priority = priority;
        process_queue[process_count].burst_time = burst_time;
        process_queue[process_count].arrival_time = time(NULL);
        process_count++;
    }
    pthread_mutex_unlock(&queue_lock);
}

void* scheduler_run(void* arg) {
    while (1) {
        pthread_mutex_lock(&queue_lock);
        if (process_count == 0) {
            pthread_mutex_unlock(&queue_lock);
            break;
        }

        Process p = process_queue[0];

        // Shift processes left to maintain queue order
        for (int i = 1; i < process_count; i++) {
            process_queue[i - 1] = process_queue[i];
        }
        process_count--;

        pthread_mutex_unlock(&queue_lock);

        printf("Running process %d with priority %d (Burst: %d, Arrival: %ld)\n", p.pid, p.priority, p.burst_time, p.arrival_time);
        sleep(p.burst_time);

        deallocate_memory(p.pid);  // Free memory after process execution
    }
    return NULL;
}

void visualize_memory() {
    printf("Memory Status: ");
    for (int i = 0; i < TOTAL_MEMORY; i++) {
        printf("[%d]", memory[i].process_id);
    }
    printf("\n");
}

void compact_memory() {
    pthread_mutex_lock(&memory_lock);
    int j = 0;
    for (int i = 0; i < TOTAL_MEMORY; i++) {
        if (!memory[i].free) {
            memory[j] = memory[i];
            j++;
        }
    }
    for (; j < TOTAL_MEMORY; j++) {
        memory[j].free = 1;
        memory[j].process_id = -1;
    }
    pthread_mutex_unlock(&memory_lock);
    printf("Memory compaction completed.\n");
}

void log_process_info(Process p) {
    printf("Process ID: %d, Priority: %d, Burst Time: %d, Arrival Time: %ld\n", p.pid, p.priority, p.burst_time, p.arrival_time);
}

int main() {
    init_memory();
    pthread_t scheduler_thread;
    pthread_mutex_init(&queue_lock, NULL);

    srand(time(NULL));
    for (int i = 0; i < 5; i++) {
        int priority = rand() % 10 + 1;
        int burst = rand() % 3 + 1;
        
        pthread_mutex_lock(&queue_lock);  // Ensure safe process addition
        add_process(i, priority, burst);
        log_process_info(process_queue[process_count - 1]);
        pthread_mutex_unlock(&queue_lock);
        
        allocate_memory(i, rand() % 3 + 1);
    }

    pthread_create(&scheduler_thread, NULL, scheduler_run, NULL);
    pthread_join(scheduler_thread, NULL);

    sleep(2);
    garbage_collect();
    visualize_memory();
    compact_memory();
    visualize_memory();

    return 0;
}

