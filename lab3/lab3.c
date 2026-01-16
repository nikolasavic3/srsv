#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sched.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/thread_policy.h>
#endif

#define SIMULATION_MS 20000

typedef struct {
    int T;
    int t0;
    int C;
    int x;
} Input;

typedef struct {
    volatile int state_changed;
    volatile int processing_done;
    long long change_time;
    int num_changes;
    long long total_reaction_time;
    long long max_reaction_time;
    int unprocessed_count;
} InputState;

// #define NUM_INPUTS 6
// Input inputs[NUM_INPUTS] = {
//     {1000,   0,  30, 1},
//     {1000, 400,  30, 1},
//     {2000, 100,  50, 1},
//     {2000, 600,  50, 1},
//     {5000, 200,  80, 1},
//     {5000, 800,  80, 1},
// };

#define NUM_INPUTS 14

Input inputs[NUM_INPUTS] = {
    {1000,   0, 150, 1},   
    {1000, 200, 150, 1},   
    {1000, 400, 150, 1},   
    {1000, 600, 150, 1},   
    {1000, 800, 150, 1},   
    {2000, 100, 200, 1},   
    {2000, 300, 200, 1},   
    {2000, 500, 200, 1},   
    {5000, 150, 250, 1},   
    {5000, 450, 250, 1},   
    {800,  100, 220, 1},   
    {800,  500, 220, 1},   
    {1200, 250, 260, 1},   
    {1500, 750, 300, 1},   
};



InputState states[NUM_INPUTS];
volatile int simulation_running = 1;
volatile long long iterations_per_10ms = 0;
long long start_time;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}

void sleep_until(long long target_ms) {
    long long now = get_time_ms() - start_time;
    long long diff = target_ms - now;
    if (diff > 0) {
        usleep(diff * 1000);
    }
}

void consume_10ms() {
    for (long long i = 0; i < iterations_per_10ms; i++) {
        asm volatile("" ::: "memory");
    }
}

void calibrate() {
    long long broj = 1000000;
    long long t0, t1;
    int running = 1;
    
    printf("Kalibracija...\n");
    
    while (running) {
        t0 = get_time_ms();
        for (long long i = 0; i < broj; i++) {
            asm volatile("" ::: "memory");
        }
        t1 = get_time_ms();
        
        long long elapsed = t1 - t0;
        if (elapsed >= 10) {
            running = 0;
            iterations_per_10ms = (broj * 10) / elapsed;
        } else {
            broj *= 10;
        }
    }
    
    printf("Kalibrirano: %lld iteracija za 10ms\n", iterations_per_10ms);
}

void simulate_work_ms(int C) {
    int iterations = C / 10;
    for (int i = 0; i < iterations; i++) {
        consume_10ms();
    }
}

int get_priority(int T) {
    if (T <= 1000) return 60;
    if (T <= 2000) return 50;
    return 40;
}

void set_thread_priority(pthread_t thread, int priority) {
#ifdef __APPLE__
    mach_port_t mach_thread = pthread_mach_thread_np(thread);
    struct thread_precedence_policy tppolicy;
    tppolicy.importance = priority;
    thread_policy_set(mach_thread, THREAD_PRECEDENCE_POLICY,
        (thread_policy_t)&tppolicy, THREAD_PRECEDENCE_POLICY_COUNT);
#else
    struct sched_param param;
    memset(&param, 0, sizeof(param));
    param.sched_priority = priority;
    pthread_setschedparam(thread, SCHED_FIFO, &param);
#endif
}

void signal_handler(int sig) {
    (void)sig;
    simulation_running = 0;
}

void* input_simulator(void* arg) {
    int i = *(int*)arg;
    free(arg);
    
    long long t = inputs[i].t0;
    
    while (simulation_running && t < SIMULATION_MS) {
        sleep_until(t);
        
        pthread_mutex_lock(&mutex);
        if (states[i].state_changed) {
            states[i].unprocessed_count++;
        }
        states[i].state_changed = 1;
        states[i].processing_done = 0;
        states[i].change_time = get_time_ms() - start_time;
        states[i].num_changes++;
        printf("%lld\tUlaz-%d: promjena\n", states[i].change_time, i);
        pthread_mutex_unlock(&mutex);
        
        t += inputs[i].T;
    }
    return NULL;
}

void* controller_thread(void* arg) {
    int i = *(int*)arg;
    free(arg);
    
    long long t = inputs[i].t0 + 5;
    
    while (simulation_running && t < SIMULATION_MS + 1000) {
        sleep_until(t);
        
        pthread_mutex_lock(&mutex);
        if (states[i].state_changed) {
            long long now = get_time_ms() - start_time;
            long long reaction = now - states[i].change_time;
            
            states[i].total_reaction_time += reaction;
            if (reaction > states[i].max_reaction_time) {
                states[i].max_reaction_time = reaction;
            }
            
            printf("%lld\tUpr-%d: pocinjem obradu (reakcija: %lld ms)\n", now, i, reaction);
            
            states[i].state_changed = 0;
            pthread_mutex_unlock(&mutex);
            
            simulate_work_ms(inputs[i].C);
            
            pthread_mutex_lock(&mutex);
            states[i].processing_done = 1;
            printf("%lld\tUpr-%d: gotovo\n", get_time_ms() - start_time, i);
        }
        pthread_mutex_unlock(&mutex);
        
        t += inputs[i].T;
    }
    return NULL;
}

void print_statistics() {
    printf("\n=== STATISTIKA ===\n");
    
    long long total_changes = 0;
    long long total_reaction = 0;
    long long max_reaction = 0;
    int total_unprocessed = 0;
    
    for (int i = 0; i < NUM_INPUTS; i++) {
        int processed = states[i].num_changes - states[i].unprocessed_count;
        long long avg = processed > 0 ? states[i].total_reaction_time / processed : 0;
        
        printf("\nUlaz-%d (T=%d, C=%d):\n", i, inputs[i].T, inputs[i].C);
        printf("  Promjene: %d\n", states[i].num_changes);
        printf("  Prosjecno vrijeme reakcije: %lld ms\n", avg);
        printf("  Maksimalno vrijeme reakcije: %lld ms\n", states[i].max_reaction_time);
        printf("  Neobradjeno: %d\n", states[i].unprocessed_count);
        
        total_changes += states[i].num_changes;
        total_reaction += states[i].total_reaction_time;
        if (states[i].max_reaction_time > max_reaction) {
            max_reaction = states[i].max_reaction_time;
        }
        total_unprocessed += states[i].unprocessed_count;
    }
    
    int total_processed = total_changes - total_unprocessed;
    long long avg_total = total_processed > 0 ? total_reaction / total_processed : 0;
    
    printf("\n=== UKUPNO ===\n");
    printf("Promjene: %lld\n", total_changes);
    printf("Prosjecno vrijeme reakcije: %lld ms\n", avg_total);
    printf("Maksimalno vrijeme reakcije: %lld ms\n", max_reaction);
    printf("Neobradjeno: %d\n", total_unprocessed);
}

int main() {
    pthread_t input_threads[NUM_INPUTS];
    pthread_t ctrl_threads[NUM_INPUTS];
    
    signal(SIGINT, signal_handler);
    
    for (int i = 0; i < NUM_INPUTS; i++) {
        memset(&states[i], 0, sizeof(InputState));
    }
    
    calibrate();
    
    printf("\nPokretanje simulacije (%d ms)...\n\n", SIMULATION_MS);
    start_time = get_time_ms();
    
    for (int i = 0; i < NUM_INPUTS; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&input_threads[i], NULL, input_simulator, id);
        set_thread_priority(input_threads[i], 70);
    }
    
    for (int i = 0; i < NUM_INPUTS; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&ctrl_threads[i], NULL, controller_thread, id);
        set_thread_priority(ctrl_threads[i], get_priority(inputs[i].T));
    }
    
    for (int i = 0; i < NUM_INPUTS; i++) {
        pthread_join(input_threads[i], NULL);
    }
    
    simulation_running = 0;
    
    for (int i = 0; i < NUM_INPUTS; i++) {
        pthread_join(ctrl_threads[i], NULL);
    }
    
    print_statistics();
    
    return 0;
}