#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>

typedef struct {
    int T;   // Period u ms
    int t0;  // vrijeme pojavljivanja in ms
    int C;   // tranjaje u ms
    int x;   //
} Input;

typedef struct {
    int state_changed;
    int processing_done;
    long long change_time;
    
    // Statistika
    int num_changes;
    long long total_reaction_time;
    long long max_reaction_time;
    int unprocessed_count;
} InputState;

#define NUM_INPUTS 38

Input inputs[NUM_INPUTS] = {
//ovaj redak svake sekunde
{ 1000, 0, 30, 1}, {1000, 400, 30, 1}, {1000, 700, 30, 1}, //a1-a3
//samo jedan od ovih pet redaka svake sekunde
{ 5000,  100, 50, 1}, {5000,  500, 50, 1}, {5000,  800, 50, 1}, //b1-b3
{ 5000, 1100, 50, 1}, {5000, 1500, 50, 1}, {5000, 1800, 50, 1}, //b4-b6
{ 5000, 2100, 50, 1}, {5000, 2500, 50, 1}, {5000, 2800, 50, 1}, //b7-b9
{ 5000, 3100, 50, 1}, {5000, 3500, 50, 1}, {5000, 3800, 50, 1}, //b9-b12
{ 5000, 4100, 50, 1}, {5000, 4500, 50, 1}, {5000, 4800, 50, 1}, //b13-b15
//samo jedan od ovih 20 redaka svake sekunde
{20000,   900,  50, 1},  //c1
{20000,  1900, 150, 1},  //c2
{20000,  2900,  50, 1},  //c3
{20000,  3900, 150, 1},  //c4
{20000,  4900,  50, 1},  //c5
{20000,  5900, 150, 1},  //c6
{20000,  6900,  50, 1},  //c7
{20000,  7900, 150, 1},  //c8
{20000,  8900,  50, 1},  //c9
{20000,  9900, 150, 1},  //c10
{20000, 10900,  50, 1},  //c11
{20000, 11900, 150, 1},  //c12
{20000, 12900,  50, 1},  //c13
{20000, 13900, 150, 1},  //c14
{20000, 14900,  50, 1},  //c15
{20000, 15900, 150, 1},  //c16
{20000, 16900,  50, 1},  //c17
{20000, 17900, 150, 0},  //c18-samo popunjavanje tablice
{20000, 18900, 150, 0},  //c19-samo popunjavanje tablice
{20000, 19900, 150, 0}   //c20-samo popunjavanje tablice
};

InputState states[NUM_INPUTS];
volatile int simulation_running = 1;
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

void signal_handler(int sig) {
    simulation_running = 0;
}

void* input_simulator(void* arg) {
    int i = *(int*)arg;
    free(arg);
    
    long long t = inputs[i].t0;
    
    printf("Ulaz-%d pocetak (T=%d, t0=%d, C=%d)\n", i, inputs[i].T, inputs[i].t0, inputs[i].C);
    
    while (simulation_running && t < 20000) {  //20s max
        sleep_until(t);
        pthread_mutex_lock(&mutex);
        // provjeri neobradjeni dogadjaj
        if (states[i].state_changed == 1) {
            states[i].unprocessed_count++;
            printf("%lld\tUlaz-%d: neobradjeni dogadjaj\n", get_time_ms() - start_time, i);
        }
        
        states[i].state_changed = 1;
        states[i].processing_done = 0;
        states[i].change_time = get_time_ms() - start_time;
        states[i].num_changes++;
        
        printf("%lld\tUlaz-%d: promjena ulaza\n", states[i].change_time, i);
        
        pthread_mutex_unlock(&mutex);
        
        long long timeout = t + inputs[i].T;
        while (simulation_running && !states[i].processing_done && 
               (get_time_ms() - start_time) < timeout) {
            usleep(10000);  // 10ms
        }
        
        pthread_mutex_lock(&mutex);
        if (states[i].processing_done) {
            // Successfully processed
        } else {
            // Timeout - already counted as unprocessed
        }
        pthread_mutex_unlock(&mutex);
        
        t += inputs[i].T;
    }
    
    printf("Ulaz-%d gotov\n", i);
    return NULL;
}

void* controller(void* arg) {
    while (simulation_running) {
        for (int i = 0; i < NUM_INPUTS; i++) {
            pthread_mutex_lock(&mutex);
            if (states[i].state_changed == 1) {
                long long reaction_start = get_time_ms() - start_time;
                long long reaction_time = reaction_start - states[i].change_time;
                printf("%lld\tUpravljac: započinjem obradu ulaza-%d (vrijeme reakcije: %lld ms)\n", 
                       reaction_start, i, reaction_time);
                states[i].total_reaction_time += reaction_time;
                if (reaction_time > states[i].max_reaction_time) {
                    states[i].max_reaction_time = reaction_time;
                }
                
                pthread_mutex_unlock(&mutex);
                
                // simulacija  procesa
                usleep(inputs[i].C * 1000 * 2);
                
                pthread_mutex_lock(&mutex);
                
                printf("%lld\tUpravljac: gotov s ulaz-%d\n", 
                       get_time_ms() - start_time, i);
                
                states[i].state_changed = 0;
                states[i].processing_done = 1;
            }
            
            pthread_mutex_unlock(&mutex);
        }
        usleep(1000000);
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

        printf("\nUlaz-%d:\n", i);
        printf("  Broj promjena stanja: %d\n", states[i].num_changes);
        printf("  Prosječno vrijeme reakcije na promjenu stanja: %lld ms\n", avg);
        printf("  Maksimalno vrijeme reakcije na promjenu stanja: %lld ms\n", states[i].max_reaction_time);
        printf("  Broj neobrađenih događaja: %d\n", states[i].unprocessed_count);
        
        
        total_changes += states[i].num_changes;
        total_reaction += states[i].total_reaction_time;
        if (states[i].max_reaction_time > max_reaction) {
            max_reaction = states[i].max_reaction_time;
        }
        total_unprocessed += states[i].unprocessed_count;
    }
    
    int total_processed = total_changes - total_unprocessed;
    long long avg_total = total_processed > 0 ? total_reaction / total_processed : 0;
    double postotak_neobradjenih = (double)total_unprocessed / (double)total_changes * 100;
    printf("\n=== UKUPNA STATISTIKA ===\n");
    printf("Broj promjena stanja: %lld\n", total_changes);
    printf("Prosječno vrijeme reakcije na promjenu stanja: %lld ms\n", avg_total);
    printf("Maksimalno vrijeme reakcije na promjenu stanja: %lld ms\n", max_reaction);
    printf("Broj neobrađenih događaja: %d\n", total_unprocessed);
    printf("Postotak neobrađenih događaja: %f\n", postotak_neobradjenih);
}

int main() {
    signal(SIGINT, signal_handler);
    
    start_time = get_time_ms();
    
    for (int i = 0; i < NUM_INPUTS; i++) {
        states[i].state_changed = 0;
        states[i].processing_done = 0;
        states[i].change_time = 0;
        states[i].num_changes = 0;
        states[i].total_reaction_time = 0;
        states[i].max_reaction_time = 0;
        states[i].unprocessed_count = 0;
    }
    
    pthread_t ctrl;
    pthread_t inputs_threads[NUM_INPUTS];
    
    pthread_create(&ctrl, NULL, controller, NULL);
    
    for (int i = 0; i < NUM_INPUTS; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&inputs_threads[i], NULL, input_simulator, id);
    }
    
    for (int i = 0; i < NUM_INPUTS; i++) {
        pthread_join(inputs_threads[i], NULL);
    }
    
    simulation_running = 0;
    pthread_join(ctrl, NULL);
    
    print_statistics();
    
    return 0;
}