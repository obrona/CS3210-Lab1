#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <iostream>
#include <chrono>
const int CAPACITY = 10;
const int NUM_OF_PRODUCERS = 2;
const int NUM_OF_CONSUMERS = 1;
const int PROD_COUNT = 10;
const int CONSUMER_COUNT = 20;



struct Queue {
    // no need to share this as this is constant
    int size; 
    // info[0] is s, info[1] is e. Everything must be shared
    int *info; 
    int *array;
    int shmid_1;
    int shmid_2;

    
    Queue(int size): size(size) {
        shmid_1 = shmget(IPC_PRIVATE, sizeof(int) * 3, 0644 | IPC_CREAT);
        info = (int*) shmat(shmid_1, NULL, 0);
       
        shmid_2 = shmget(IPC_PRIVATE, sizeof(int) * size, 0644 | IPC_CREAT);
        array = (int*) shmat(shmid_2, NULL, 0);
    }

    void add(int elem) {
        array[info[1]] = elem;
        info[1] ++; info[1] %= size;
    }

    int remove() {
        int temp = array[info[0]];
        info[0] ++; info[0] %= size;
        return temp;
    }

    ~Queue() {
        shmdt(info);
        shmctl(shmid_1, IPC_RMID, 0);

        shmdt(array);
        shmctl(shmid_2, IPC_RMID, 0);
    }
}; 
  

// create shared memory to store consumed_sum;
int shmid;
int sem1_id;
int sem2_id;
int sem3_id;

int *consumer_sum;
sem_t *empty_slots;
sem_t *filled_slots;
sem_t *lock;


void initialise() {
    shmid = shmget(IPC_PRIVATE, sizeof(int), 0644 | IPC_CREAT);
    consumer_sum = (int*) shmat(shmid, NULL, 0);

    sem1_id = shmget(IPC_PRIVATE, sizeof(sem_t), 0644 | IPC_CREAT);
    empty_slots = (sem_t*) shmat(sem1_id, NULL, 0);
    sem_init(empty_slots, 1, CAPACITY);

    sem2_id = shmget(IPC_PRIVATE, sizeof(sem_t), 0644 | IPC_CREAT);
    filled_slots = (sem_t*) shmat(sem2_id, NULL, 0);
    sem_init(filled_slots, 1, 0);

    sem3_id = shmget(IPC_PRIVATE, sizeof(sem_t), 0644 | IPC_CREAT);
    lock = (sem_t*) shmat(sem3_id, NULL, 0);
    sem_init(lock, 1, 1);
}

void clean() {
    sem_destroy(empty_slots);
    sem_destroy(filled_slots);
    sem_destroy(lock);
    
    shmdt(consumer_sum);
    shmdt(empty_slots);
    shmdt(filled_slots);
    shmdt(lock);
    
    shmctl(shmid, IPC_RMID, 0);
    shmctl(sem1_id, IPC_RMID, 0);
    shmctl(sem2_id, IPC_RMID, 0);
    shmctl(sem3_id, IPC_RMID, 0);
}


void producer(Queue *queue) {
    for (int i = 0; i < PROD_COUNT; i ++) {
        int elem = rand() % 10 + 1;
        sem_wait(empty_slots); 
        sem_wait(lock);
        
        //std::cout << "Index added: " << queue->info[1] << std::endl;
        queue->add(elem);
        std::cout << "Elem added: " << elem << std::endl;
        
        sem_post(lock);

        // actually can put this outside
        sem_post(filled_slots);
    }
}

void consumer(Queue *queue) {
    for (int i = 0; i < CONSUMER_COUNT; i ++) {
        sem_wait(filled_slots);
        sem_wait(lock);
        
        //std::cout << "Index removed: " << queue->info[0] << std::endl;
        int elem = queue->remove();
        std::cout << "Elem removed: " << elem << std::endl;
        *consumer_sum = *consumer_sum + elem;
        
        sem_post(lock);

        sem_post(empty_slots);
    }
}


int main() {
    initialise();
    Queue *queue = new Queue(CAPACITY);

    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < NUM_OF_CONSUMERS; i ++) {
        int pid = fork();
        if (pid == 0) {
            consumer(queue);
            exit(0);
        }
    }

    for (int i = 0; i < NUM_OF_PRODUCERS; i ++) {
        int pid = fork();
        if (pid == 0) {
            producer(queue);
            exit(0);
        }
    }
    
    for (int i = 0; i < NUM_OF_CONSUMERS + NUM_OF_PRODUCERS; i ++) {
        wait(NULL);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = end_time - start_time;
    
    std::cout << "Time taken: " << elapsed_time.count() << std::endl;
    std::cout << "Result is: " << *consumer_sum << std::endl;
    delete queue;
    clean();

}