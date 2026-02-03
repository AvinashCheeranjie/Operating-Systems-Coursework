#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 6 // # of threads for program

int amount;            // shared variable between threads
pthread_mutex_t mutex; // mutex for thread function critical sections

// Function prototypes
void *deposit(void *value_d);
void *withdraw(void *value_w);

int main(int argc, char *argv[])
{
    pthread_t tids[NUM_THREADS]; // array to hold thread ids

    if (pthread_mutex_init(&mutex, NULL) != 0) { // initialize mutex
        printf("Error in initializing mutex\n");
        return -1;
    }

    if (argc != 3) { // ensure arguments are provided correctly
        printf("Error: Please provide exactly two arguments for deposit and withdraw amounts.\n");
        return -1;
    }

    for (int i = 0; i < NUM_THREADS; i++) { // create threads (3 deposit, 3 withdraw)
        if (i % 2 == 0) {
            if (pthread_create(&tids[i], NULL, deposit, argv[1]) != 0) {
                printf("Error in creating deposit thread\n"); // check for thread creation error
                return -1;
            }
        } else {
            if (pthread_create(&tids[i], NULL, withdraw, argv[2]) != 0) {
                printf("Error in creating withdraw thread\n");
                return -1;
            }
        }
    }

    for (int i = 0; i < NUM_THREADS; i++) { // join all threads for completion
        pthread_join(tids[i], NULL);
    }

    printf("Final amount = %d\n", amount); // print final amount after all operations
    return 0;
}

// Deposit Function
void *deposit(void *value_d)
{
    int val = atoi((char *)value_d);
    pthread_mutex_lock(&mutex); // lock mutex for critical section
    amount += val;
    printf("Deposit amount = %d\n", amount);
    pthread_mutex_unlock(&mutex); // unlock mutex after critical section
    pthread_exit(0);
}

// Withdraw Function
void *withdraw(void *value_w)
{
    int val = atoi((char *)value_w);
    pthread_mutex_lock(&mutex); // lock mutex for critical section
    amount -= val;
    printf("Withdrawal amount = -%d\n", amount);
    pthread_mutex_unlock(&mutex); // unlock mutex after critical section
    pthread_exit(0);
}