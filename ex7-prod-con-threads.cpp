/*******************************************************************
 * ex789-prod-con-threads.cpp
 * Producer-consumer synchronisation problem in C++
 *******************************************************************/

#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <iostream>
#include <chrono>


pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

// PRODUCERS * PROD_COUNT - CONSUME_COUNT * CONSUMERS <= CAPACITY otherwise deadlock
constexpr int PRODUCERS = 2; 
constexpr int CONSUMERS = 1;
const int CAPACITY = 10;
const int PROD_COUNT = 10000;    
const int CONSUME_COUNT = 20000;  


// use a circular array as a queue
int s_ptr = 0, e_ptr = 0;
int size = 0;
int producer_buffer[CAPACITY];
int consumer_sum = 0;

void add(int item) {
	producer_buffer[e_ptr] = item;
	e_ptr ++; e_ptr %= CAPACITY;
	size ++;
}

int remove() {
	int temp = producer_buffer[s_ptr];
	s_ptr ++; s_ptr %= CAPACITY;
	size --;
	return temp;
}



void *producer(void *threadid) {
	// Write producer code here
	
	for (int i = 0; i < PROD_COUNT; i ++) {
		int elem = rand() % 10 + 1;
		pthread_mutex_lock(&lock);
		
		// we must do "busy waiting" reason is when this thread is woken up, 
		// the predicate may not be true anymore,
		// example, got woken up, then before reacquring lock, got switched out.
		// when processer runs this thread again, predicate is not true anymore
		while (size == CAPACITY) {
			pthread_cond_wait(&full, &lock);
		}
		
		add(elem);
		//std::cout << "Elem added: " << elem << std::endl;
		pthread_mutex_unlock(&lock);

		// after adding 1 elem, can wake up 1consumer sleeping on empty
		// actually can we do this outside of CS
		pthread_cond_signal(&empty); 
	}
	
}

void *consumer(void *threadid) {
	// Write consumer code here
	for (int i = 0; i < CONSUME_COUNT; i ++) {
		pthread_mutex_lock(&lock);
		
		while (size == 0) {
			pthread_cond_wait(&empty, &lock);
		}
		
		int elem = remove();
		consumer_sum += elem;
		//std::cout << "Elem removed: " << elem << std::endl;
		pthread_mutex_unlock(&lock);

		// after removing 1 elem, can wake up 1 producer sleeping on full
		// actually can do outside CS
		pthread_cond_signal(&full); 
	}
	
	
}

int main(int argc, char *argv[])
{
	pthread_t producer_threads[PRODUCERS];
	pthread_t consumer_threads[CONSUMERS];
	int producer_threadid[PRODUCERS];
	int consumer_threadid[CONSUMERS];

	int rc;
	int t1, t2;

	auto start_time = std::chrono::high_resolution_clock::now();
	
	// switched the order for fun
	for (t2 = 0; t2 < CONSUMERS; t2++)
	{
		int tid = t2;
		consumer_threadid[tid] = tid;
		//printf("Main: creating consumer %d\n", tid);
		rc = pthread_create(&consumer_threads[tid], NULL, consumer,
							(void *)&consumer_threadid[tid]);
		if (rc)
		{
			printf("Error: Return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	
	for (t1 = 0; t1 < PRODUCERS; t1++)
	{
		int tid = t1;
		producer_threadid[tid] = tid;
		//printf("Main: creating producer %d\n", tid);
		rc = pthread_create(&producer_threads[tid], NULL, producer,
							(void *)&producer_threadid[tid]);
		if (rc)
		{
			printf("Error: Return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	

	for (int i = 0; i < PRODUCERS; i ++) {
		pthread_join(producer_threads[i], NULL);
	}
	
	for (int i = 0; i < CONSUMERS; i ++) {
		pthread_join(consumer_threads[i], NULL);
	}
	
	auto end_time = std::chrono::high_resolution_clock::now();
	
	std::chrono::duration<double> elapsed_time = end_time - start_time;
	std::cout << "Result is: " << consumer_sum << "\n";
	std::cout << "Time taken: " << elapsed_time.count() << "\n";

	pthread_exit(NULL);

	/*
					some tips for this exercise:

					1. you may want to handle SIGINT (ctrl-C) so that your program
									can exit cleanly (by killing all threads, or just calling
		 exit)

					1a. only one thread should handle the signal (POSIX does not define
									*which* thread gets the signal), so it's wise to mask out the
		 signal on the worker threads (producer and consumer) and let the main
		 thread handle it
	*/
}
