#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 10 // # of threads for program

int amount;            // shared variable between threads
pthread_mutex_t mutex; // mutex for thread function critical sections
sem_t sem_d;           // semaphore for deposit function
sem_t sem_w;           // semaphore for withdraw function

// Function prototypes
void *deposit(void *value);
void *withdraw(void *value);

int main(int argc, char *argv[])
{
    pthread_t tids[NUM_THREADS]; // array to hold thread ids

    if (sem_init(&sem_d, 0, 1) != 0) { // initialize deposit semaphore
        printf("Error in initializing deposit semaphore\n");
        return -1;
    }

    if (sem_init(&sem_w, 0, 0) != 0) { // initialize withdraw semaphore
        printf("Error in initializing withdraw semaphore\n");
        return -1;
    }

    if (pthread_mutex_init(&mutex, NULL) != 0) { // initialize mutex
        printf("Error in initializing mutex\n");
        return -1;
    }

    if (argc != 2 || atoi(argv[1]) != 100) { // ensure argument is provided correctly
        printf("Error: Please provide exactly one argument (100) for the amount to deposit/withdraw.\n");
        return -1;
    }

    for (int i = 0; i < NUM_THREADS; i++) { // create threads (7 deposit, 3 withdraw)
        if (i < 7) {
            if (pthread_create(&tids[i], NULL, deposit, argv[1]) != 0) {
                printf("Error in creating deposit thread\n"); // check for thread creation error
                return -1;
            }
        } else {
            if (pthread_create(&tids[i], NULL, withdraw, argv[1]) != 0) {
                printf("Error in creating withdraw thread\n");
                return -1;
            }
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) { // join all threads for completion
        pthread_join(tids[i], NULL);
    }

    printf("Final amount: %d\n", amount); // print final amount after all operations
    return 0;
}

// Deposit Function
void *deposit(void *value)
{
    printf("Executing deposit function\n");
    sem_wait(&sem_d); // wait on deposit semaphore
    int val = atoi((char *)value);
    pthread_mutex_lock(&mutex); // lock mutex for critical section
    int prev = amount;
    amount += val;
    printf("Deposit amount: %d\n", amount);

    if (prev <= 0 && amount > 0) { // enable withdraw semaphore if amount was <= 0 before deposit
        sem_post(&sem_w);
    }

    if (amount < 400) { // enable deposit semaphore if amount is still < 400
        sem_post(&sem_d);
    }

    pthread_mutex_unlock(&mutex); // unlock mutex after critical section
    pthread_exit(0);
}

// Withdraw Function
void *withdraw(void *value)
{
    printf("Executing withdraw function\n");
    sem_wait(&sem_w); // wait on withdraw semaphore
    int val = atoi((char *)value);
    pthread_mutex_lock(&mutex); // lock mutex for critical section
    int prev = amount;
    amount -= val;
    printf("Withdrawal amount: %d\n", amount);

    if (prev >= 400 && amount < 400) { // enable deposit semaphore if amount was >= 400 before withdraw
        sem_post(&sem_d);
    }

    if (amount > 0) { // enable withdraw semaphore is amount is still > 0
        sem_post(&sem_w);
    }

    pthread_mutex_unlock(&mutex); // unlock mutex after critical section
    pthread_exit(0);
}