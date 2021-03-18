#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>
#include <fcntl.h>  
#include "threadpool.h"

#define NUMBER_OF_THREADS 3
#define QUEUE_SIZE 10

// define semaphore
sem_t *sem;

// define mutex lock
pthread_mutex mutex;

pthread_t tid[NUMBER_OF_THREADS]; /* the thread identifier */

// this represents work that has to be 
// completed by a thread in the pool
typedef struct 
{
    void (*function)(void *p);
	void *data;
}
task;

// the work queue
task clientqueue[QUEUE_SIZE];
int front; // points to the front of the queue, the first job in the queue
int rear; // points to the rear of the queue, the next available position in the queue
int count;

// insert a task into the queue
// returns 1 if successful, 0 is queue is full.
int enqueue(task t) 
{
	int rv = 0;

	pthread_mutex_lock(&mutex);
	/* critical section */
	if (count < QUEUE_SIZE) {
		clientqueue[rear] = t;
		rear = (rear + 1) % QUEUE_SIZE;
		count++;
		rv = 1;
	}
	pthread_mutex_unlock(&mutex);

    return rv;
}

// remove a task from the queue
task dequeue() 
{
    task t;

	pthread_mutex_lock(&mutex);
	/* critical section */
	t = clientqueue[front];
	front = (front + 1) % QUEUE_SIZE;
	pthread_mutex_unlock(&mutex);
	
	return t;
}

/**
 * Executes the task provided to the thread pool
 */
void execute(void (*somefunction)(void *p), void *p)
{
    (*somefunction)(p);
}

// how the client submits work to the thread pool
// returns QUEUE_SUCCESS if work submitted, or QUEUE_REJECTED
// if the queue is full.
int work_submit(void (*somefunction)(void *p), void *p)
{
	// load job into task1
	task task1;

	task1.function = somefunction;
	task1.data = p;

	// add task1 into queue
	if (enqueue(task1)) {
		sem_post(sem); // tell the pool_scheduling that a job is ready
		return QUEUE_SUCCESS;
	} else {
		return QUEUE_REJECTED;
	}
    // clientqueue.function = somefunction;
	// clientqueue.data = p;
	
	return 0;
    
}

// schedule available threads from pool
void *pool_scheduling(void *param)
{
	while (1) {
		printf("Thread %lu is ready ...\n", pthread_self());
		sem_wait(sem); // wait for jobs

		printf("Thread %lu is occupied\n", pthread_self());

		task t = dequeue();

		execute(t.function, t.data);

		pthread_testcancel();
	}
	pthread_exit(0);
}

// initialize the thread pool
void pool_init(void)
{
	// init mutex
	pthread_mutex_init(&mutex, NULL);

	// init sem
	sem_unlink("SEM");
	sem = sem_open("SEM", O_CREAT, 0666, 0);

	// create NUMBER_OF_THREADS of threads
	for (int i = 0; i < NUMBER_OF_THREADS; i++){
		pthread_create(&tid[i], NULL, pool_scheduling, NULL);
	}
}

// shutdown the thread pool
void pool_shutdown(void)
{
	pthread_cancel(tid);
}

