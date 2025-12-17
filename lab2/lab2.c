#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

#define ACT_WARN 0
#define ACT_STOP 1
#define CALL(ACT,FUNC,...)        \
do {                              \
    if ( FUNC ( __VA_ARGS__ ) ) { \
        perror ( #FUNC );         \
        if ( ACT == ACT_STOP )    \
            exit (1);             \
    }                             \
} while(0)

#define HISTORY_SIZE 10
#define NUM_INPUTS 38

typedef struct {
    int T;
    int t0;
    int C;
    int x;
} Input;

typedef struct {
    int state_changed;
    int processing_done;
    long long change_time;
    
    int num_changes;
    long long total_reaction_time;
    long long max_reaction_time;
    int unprocessed_count;
} InputState;

typedef struct {
    int task_id;
    int in_progress;
    int interrupted;
    int periods_used;
    int iterations_done;
    long long start_time;
} TaskState;

typedef struct {
    int completed;
    int interrupted;
    int used_two_periods;
} Statistics;

Input inputs[NUM_INPUTS] = {
    {1000, 0, 30, 1}, {1000, 400, 30, 1}, {1000, 700, 30, 1},
    {5000, 100, 50, 1}, {5000, 500, 50, 1}, {5000, 800, 50, 1},
    {5000, 1100, 50, 1}, {5000, 1500, 50, 1}, {5000, 1800, 50, 1},
    {5000, 2100, 50, 1}, {5000, 2500, 50, 1}, {5000, 2800, 50, 1},
    {5000, 3100, 50, 1}, {5000, 3500, 50, 1}, {5000, 3800, 50, 1},
    {5000, 4100, 50, 1}, {5000, 4500, 50, 1}, {5000, 4800, 50, 1},
    {20000, 900, 50, 1},
    {20000, 1900, 150, 1},
    {20000, 2900, 50, 1},
    {20000, 3900, 150, 1},
    {20000, 4900, 50, 1},
    {20000, 5900, 150, 1},
    {20000, 6900, 50, 1},
    {20000, 7900, 150, 1},
    {20000, 8900, 50, 1},
    {20000, 9900, 150, 1},
    {20000, 10900, 50, 1},
    {20000, 11900, 150, 1},
    {20000, 12900, 50, 1},
    {20000, 13900, 150, 1},
    {20000, 14900, 50, 1},
    {20000, 15900, 150, 1},
    {20000, 16900, 50, 1},
    {20000, 17900, 150, 0},
    {20000, 18900, 150, 0},
    {20000, 19900, 150, 0}
};

InputState states[NUM_INPUTS];
static TaskState current_task = {-1, 0, 0, 0, 0, 0};
static Statistics stats = {0, 0, 0};
static int extension_history[HISTORY_SIZE] = {0};
static int history_index = 0;
volatile int simulation_running = 1;
long long start_time;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int zadA[] = {1, 2, 3};
static int zadB[] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
static int zadC[] = {19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38};

static int* zad[] = {zadA, zadB, zadC};
static int red[] = {0, 1, -1, 0, 1, -1, 0, 1, 2, -1};
static int t = 0;
static int ind[] = {0, 0, 0};
static int MAXTIP[] = {3, 15, 20};

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

int daj_iduci() {
    int sljedeci_zadatak = 0;
    int tip = red[t];
    t = (t + 1) % 10;
    if (tip != -1) {
        sljedeci_zadatak = zad[tip][ind[tip]];
        ind[tip] = (ind[tip] + 1) % MAXTIP[tip];
    }
    return sljedeci_zadatak - 1;
}

int had_recent_extension() {
    for (int i = 0; i < HISTORY_SIZE; i++) {
        if (extension_history[i] == 1)
            return 1;
    }
    return 0;
}

void record_period(int used_extension) {
    extension_history[history_index] = used_extension;
    history_index = (history_index + 1) % HISTORY_SIZE;
}

static void obradi_zadatak(int sig, siginfo_t *info, void *context) {
    pthread_mutex_lock(&mutex);

    if (current_task.in_progress) {
        if (current_task.periods_used == 1 && !had_recent_extension()) {
            current_task.periods_used = 2;
            record_period(1);
            stats.used_two_periods++;
            printf("  [PROSIRENJE] Zadatak %d dobiva 2. period\n", current_task.task_id);
            pthread_mutex_unlock(&mutex);
            return;
        } else {
            printf("  [PREKID] Zadatak %d prekinut\n", current_task.task_id);
            current_task.interrupted = 1;
            stats.interrupted++;
            record_period(0);
        }
    } else {
        record_period(0);
    }
    int next_task = daj_iduci();
    if (next_task == -1) {
        printf("Perioda: prazno\n");
        pthread_mutex_unlock(&mutex);
        return;
    }
    if (inputs[next_task].x == 0) {
        printf("Perioda: Zadatak %d (samo popuna)\n", next_task);
        pthread_mutex_unlock(&mutex);
        return;
    }
    if (!states[next_task].state_changed) {
        printf("Perioda: Zadatak %d - ulaz nije spreman!\n", next_task);
        pthread_mutex_unlock(&mutex);
        return;
    }
    long long reaction_start = get_time_ms() - start_time;
    long long reaction_time = reaction_start - states[next_task].change_time;
    states[next_task].total_reaction_time += reaction_time;
    if (reaction_time > states[next_task].max_reaction_time) {
        states[next_task].max_reaction_time = reaction_time;
    }
    printf("Perioda: Pokrecem zadatak %d (vrijeme reakcije: %lld ms)\n", next_task, reaction_time);
    current_task.task_id = next_task;
    current_task.in_progress = 1;
    current_task.interrupted = 0;
    current_task.periods_used = 1;
    current_task.iterations_done = 0;
    current_task.start_time = get_time_ms() - start_time;
    pthread_mutex_unlock(&mutex);
    int total_iterations = inputs[next_task].C / 5;
    if (total_iterations == 0) total_iterations = 1;
    while (current_task.iterations_done < total_iterations && !current_task.interrupted) {
        usleep(5000);
        current_task.iterations_done++;
    }
    pthread_mutex_lock(&mutex);
    if (!current_task.interrupted) {
        printf("  [GOTOVO] Zadatak %d zavrsen\n", next_task);
        stats.completed++;
        states[next_task].state_changed = 0;
        states[next_task].processing_done = 1;
    }
    
    current_task.in_progress = 0;
    pthread_mutex_unlock(&mutex);
}

void* input_simulator(void* arg) {
    int i = *(int*)arg;
    free(arg);
    long long t = inputs[i].t0;
    while (simulation_running && t < 20000) {
        sleep_until(t);
        pthread_mutex_lock(&mutex);
        if (states[i].state_changed == 1) {
            states[i].unprocessed_count++;
        }
        states[i].state_changed = 1;
        states[i].processing_done = 0;
        states[i].change_time = get_time_ms() - start_time;
        states[i].num_changes++;
        pthread_mutex_unlock(&mutex);
        t += inputs[i].T;
    }
    return NULL;
}

void print_statistics() {
    printf("\n=== STATISTIKA ZADATAKA ===\n");
    printf("Zavrseno: %d\n", stats.completed);
    printf("Prekinuto: %d\n", stats.interrupted);
    printf("Koristilo dva perioda: %d\n", stats.used_two_periods);
    printf("\n=== STATISTIKA ULAZA ===\n");
    long long total_changes = 0;
    long long total_reaction = 0;
    long long max_reaction = 0;
    int total_unprocessed = 0;
    for (int i = 0; i < NUM_INPUTS; i++) {
        if (inputs[i].x == 0) continue;
        int processed = states[i].num_changes - states[i].unprocessed_count;
        long long avg = processed > 0 ? states[i].total_reaction_time / processed : 0;
        printf("\nUlaz-%d:\n", i);
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
    printf("\n=== UKUPNA STATISTIKA ===\n");
    printf("Ukupno promjena: %lld\n", total_changes);
    printf("Prosjecno vrijeme reakcije: %lld ms\n", avg_total);
    printf("Maksimalno vrijeme reakcije: %lld ms\n", max_reaction);
    printf("Ukupno neobradjeno: %d\n", total_unprocessed);
}

int main() {
    struct sigaction act;
    struct itimerval timer;
    pthread_t inputs_threads[NUM_INPUTS];
    printf("Pokretanje LAB2 periodickog rasporedjivaca zadataka...\n\n");
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
    for (int i = 0; i < NUM_INPUTS; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&inputs_threads[i], NULL, input_simulator, id);
    }
    usleep(10000);
    act.sa_sigaction = obradi_zadatak;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO | SA_NODEFER;
    sigaction(SIGALRM, &act, NULL);
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 100000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 100000;
    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        exit(1);
    }
    for (int i = 0; i < 200; i++) {
        pause();
    }
    simulation_running = 0;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);
    
    for (int i = 0; i < NUM_INPUTS; i++) {
        pthread_join(inputs_threads[i], NULL);
    }
    print_statistics();
    return 0;
}
