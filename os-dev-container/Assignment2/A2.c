/*
 * Compile: make A2
 * Run:     ./A2 [num_students] [office_hours_seconds]  (defaults: 5 students, 30 seconds)
 */
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define NUM_CHAIRS 3
#define MAX_SLEEP 5 // max random sleep seconds
#define DEFAULT_STUDENTS 5
#define DEFAULT_OFFICE_HOURS 30 // default office hours in seconds

volatile int office_open = 1; // flag for office hours status 

// Global synchronization primitives 
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
sem_t ta_sem;
sem_t chair_sems[NUM_CHAIRS];

// Shared state for waiting students
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
    int office_hours = DEFAULT_OFFICE_HOURS;
    if (argc > 1) {
        num_students = atoi(argv[1]);
        if (num_students <= 0) {
            fprintf(stderr, "Invalid number of students. Using default %d.\n", DEFAULT_STUDENTS);
            num_students = DEFAULT_STUDENTS;
        }
    }
    if (argc > 2) {
        office_hours = atoi(argv[2]);
        if (office_hours < 0) {
            fprintf(stderr, "Invalid office hours. Using default %d.\n", DEFAULT_OFFICE_HOURS);
            office_hours = DEFAULT_OFFICE_HOURS;
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

    // simulate office hours duration
    sleep(office_hours);
    office_open = 0;
    printf("\nOffice hours over. No more help requests accepted.\n\n");

    for (int i = 0; i < num_students; i++) {
        pthread_join(students[i], NULL);
    }

    pthread_join(ta, NULL);

    printf("\nSimulation Complete\n");

    // cleanup
    free(students);
    free(student_ids);
    sem_destroy(&ta_sem);
    for (int i = 0; i < NUM_CHAIRS; i++) {
        sem_destroy(&chair_sems[i]);
    }
    pthread_mutex_destroy(&mutex);

    return 0;
}

/* TA thread - naps until woken, then helps waiting students in FIFO order (no nap between consecutive students) */
void *ta_thread(void *arg)
{
    (void)arg; // unused
    while (1) {
        struct timespec ts;
        clock_gettime(0, &ts);
        ts.tv_sec += 1; // 1 second timeout for checking flag

        int ret = sem_timedwait(&ta_sem, &ts);
        if (ret == -1) {
            if (errno == ETIMEDOUT) {
                if (!office_open) {
                    printf("TA: Office closed. All students helped.\n");
                    break;
                }
                continue;
            } else {
                perror("sem_timedwait");
                continue;
            }
        }

        // got semaphore (student needs help)
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

/* Student thread - continuously programs then seeks help until office closes.
   If chairs full, resumes programming immediately (retries after next programming period) */
void *student_thread(void *arg)
{
    int id = *(int *)arg;
    int helps_received = 0;

    while (1) {
        // programming time
        printf("Student %d is programming.\n", id);
        int prog_time = (rand() % MAX_SLEEP) + 1;
        sleep(prog_time);

        // check if office still open before seeking help
        if (!office_open) {
            break;
        }

        // seek help
        pthread_mutex_lock(&mutex);
        if (waiting < NUM_CHAIRS) {
            int my_chair = next_seat;
            chairs[my_chair] = id;
            next_seat = (next_seat + 1) % NUM_CHAIRS;
            waiting++;

            printf("Student %d sits in chair %d. Waiting students: %d\n", id, my_chair, waiting);
            pthread_mutex_unlock(&mutex);

            sem_post(&ta_sem); // wake TA if sleeping
            sem_wait(&chair_sems[my_chair]); // wait for specific help session

            helps_received++;
            printf("Student %d has finished receiving help (session %d).\n", id, helps_received);
        } else {
            pthread_mutex_unlock(&mutex);
            printf("Student %d: No chairs available, resumes programming.\n", id);
        }
    }

    printf("Student %d finished, received %d help sessions.\n", id, helps_received);
    return NULL;
}