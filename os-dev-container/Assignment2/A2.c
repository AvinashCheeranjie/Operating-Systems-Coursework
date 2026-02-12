/*
 * Compile:  make A2
 * Run:     ./A2 [num_students] [simulation_time_seconds]  (defaults: 5 students, 30 seconds)
 */

#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NUM_CHAIRS 3
#define MAX_SLEEP 5 // max random sleep seconds
#define DEFAULT_STUDENTS 5
#define DEFAULT_SIMULATION_TIME 30 

// Global synchronization objects
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t ta_sem;
sem_t chair_sems[NUM_CHAIRS];

// Shared state for the waiting queue
int chairs[NUM_CHAIRS]; // holds student ID in each chair (FIFO queue)
int next_seat = 0;      // next position to enqueue student
int next_help = 0;      // next position to dequeue
int waiting = 0;        // current number of students waiting

// Function prototypes
void *ta_thread(void *arg);
void *student_thread(void *arg);

int main(int argc, char *argv[])
{
    int num_students = DEFAULT_STUDENTS;
    int simulation_time = DEFAULT_SIMULATION_TIME;
    if (argc > 1) {
        num_students = atoi(argv[1]);
        if (num_students <= 0) {
            fprintf(stderr, "Invalid number of students. Using default %d.\n", DEFAULT_STUDENTS);
            num_students = DEFAULT_STUDENTS;
        }
    }
    if (argc > 2) {
        simulation_time = atoi(argv[2]);
        if (simulation_time < 0) {
            fprintf(stderr, "Invalid simulation time. Using default %ds.\n", DEFAULT_SIMULATION_TIME);
            simulation_time = DEFAULT_SIMULATION_TIME;
        }
    }

    srand(time(NULL));

    sem_init(&ta_sem, 0, 0);
    for (int i = 0; i < NUM_CHAIRS; i++) {
        sem_init(&chair_sems[i], 0, 0);
    }

    pthread_t ta;
    pthread_t *students = malloc(num_students * sizeof(pthread_t));
    int *student_ids = malloc(num_students * sizeof(int));

    pthread_create(&ta, NULL, ta_thread, NULL);

    for (int i = 0; i < num_students; i++) {
        student_ids[i] = i + 1;
        pthread_create(&students[i], NULL, student_thread, &student_ids[i]);
    }

    // Run simulation for office hours duration
    sleep(simulation_time);

    printf("\nOffice hours over. Simulation complete.\n");

    // Exit terminates threads
    return 0;
}

/* TA thread - naps until woken, then helps waiting students in FIFO order (no nap between consecutive students). */
void *ta_thread(void *arg)
{
    (void)arg; // unused
    while (1) {
        sem_wait(&ta_sem); // nap until a student sits down

        pthread_mutex_lock(&mutex);
        if (waiting <= 0) {
            pthread_mutex_unlock(&mutex);
            continue;
        }

        int chair_idx = next_help;
        int stud_id = chairs[chair_idx];
        next_help = (next_help + 1) % NUM_CHAIRS;
        waiting--;

        printf("TA starts helping Student %d (chair %d). Students still waiting: %d\n",
               stud_id, chair_idx, waiting);
        pthread_mutex_unlock(&mutex);

        int help_time = (rand() % MAX_SLEEP) + 1;
        sleep(help_time);

        printf("TA finished helping Student %d.\n", stud_id);
        sem_post(&chair_sems[chair_idx]); // signal the specific student
    }
    return NULL;
}

/* Student thread - continuously programs then seeks help. */
void *student_thread(void *arg)
{
    int id = *(int *)arg;

    while (1) {
        // Programming phase
        printf("Student %d is programming.\n", id);
        int prog_time = (rand() % MAX_SLEEP) + 1;
        sleep(prog_time);

        // Seek help
        pthread_mutex_lock(&mutex);
        if (waiting < NUM_CHAIRS) {
            int my_chair = next_seat;
            chairs[my_chair] = id;
            next_seat = (next_seat + 1) % NUM_CHAIRS;
            waiting++;

            printf("Student %d sits in chair %d. Waiting students: %d\n", id, my_chair, waiting);
            pthread_mutex_unlock(&mutex);

            sem_post(&ta_sem);               // wake TA if sleeping
            sem_wait(&chair_sems[my_chair]); // wait for specific help session

            printf("Student %d has finished receiving help.\n", id);
        } else {
            pthread_mutex_unlock(&mutex);
            printf("Student %d: No chairs available, resumes programming.\n", id);
        }
    }
    return NULL;
}