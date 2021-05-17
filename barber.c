#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

unsigned int waiting_room_chairs;
pthread_t customer, barber;
unsigned int total_customers;
int res = 0;
int WRoom = 0;
int actual_id = 0;
int id=0;
sem_t Customers;
sem_t Barbers;
sem_t mod_seats;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Seats = PTHREAD_MUTEX_INITIALIZER;

int size_serviced = 0;
int size_resigned = 0;
int debug = 0;
	///////////////////////////////////////////
typedef struct ListElement {
    int data;
    struct ListElement * next;
} ListElement_type;
	
void push_front(ListElement_type **head, int number, int resign_serviced)
{
    if(resign_serviced == 0){
        if(size_serviced == 0){
            (*head)->data = number;
            ++size_serviced;
            return;
        }
        if(size_serviced > 0){
            ListElement_type *current;
            current=(ListElement_type *)malloc(sizeof(ListElement_type));
            current->data=number;
            current->next=(*head);
            *head=current;
            ++size_serviced;
        }
    }
    if(resign_serviced == 1){
        if(size_resigned == 0){
            (*head)->data = number;
            ++size_resigned;
            return;
        }
        if(size_resigned > 0){
            ListElement_type *current;
            current=(ListElement_type *)malloc(sizeof(ListElement_type));
            current->data=number;
            current->next=(*head);
            *head=current;
            ++size_resigned;
        }
    }
}


//resign 0, serviced 1
void show(ListElement_type *head, int resign_serviced)
{
    printf("\n");
    if(head==NULL) printf("List is empty");
    else
    {
        ListElement_type *current=head;
        if(resign_serviced == 0)
            printf("People, whose resigned:");
        if(resign_serviced == 1)
            printf("People serviced:");
        do {
            printf("%i ", current->data);
            current = current->next;
        }while (current != NULL);
        printf("\n");
    }
}
ListElement_type *head_resign; 
ListElement_type *head_service;
///////////////////////////////////////////////
void print_results(int resign0_serviced1){
    pthread_mutex_lock(&Seats);
    if(actual_id == 0)
        printf("\nRes:%d WRomm: %d/%d [in: none]", res, WRoom, waiting_room_chairs);
    else
        printf("\nRes:%d WRomm: %d/%d [in: %d]", res, WRoom, waiting_room_chairs, actual_id);
    if(debug == 1){
        if(resign0_serviced1 == 0)
            show(head_resign, resign0_serviced1);
        if(resign0_serviced1 == 1)
            show(head_service, resign0_serviced1);
    }
    pthread_mutex_unlock(&Seats);

}

void *Barber(void *arg) {
	while(1) {
			/* waits for a customer (sleeps). */
            sem_wait(&Customers);
            sem_wait(&mod_seats);

            --WRoom;
            actual_id = id;

            print_results(1);
            sem_post(&mod_seats);
            usleep(30000 + rand()%150000);

	}
}

void * Customer(void * arg) {
	while(1) {
            usleep(rand()%200000);
			/* protects seats so only 1 customer tries to sit
			in a chair if that's the case.*/
            sem_wait(&mod_seats);
            if(WRoom < waiting_room_chairs) {
				/* sitting down.*/
				++WRoom;
				++id;
                push_front( &head_service, id, 1);
				/* notify the barber. */
                sem_post(&Customers);
                sem_post(&mod_seats);
				
				/* release the lock */
				
				/* wait in the waiting room if barber is busy. */
                // printf("%d == WRoom:%d\n", id, WRoom);
                print_results(1);
				// customer is having hair cut
			}
            // if(WRoom >= waiting_room_chairs)
            else
            {
				/* release the lock */
                ++id;
                ++res;
                print_results(0);
                push_front( &head_resign, id, 0);
                sem_post(&mod_seats);
                // pthread_mutex_unlock(&Seats);
			}
	}
}

int main(int argc, char ** argv){
    srand(time(NULL));
    sem_init(&Barbers, 0, 0);
    sem_init(&Customers, 0, 0);
    sem_init(&mod_seats, 0, 1);

    if(argc >= 2)
        if(strcmp(argv[1], "-debug") == 0){
            debug = 1;
            printf("ssssssssssssssssssssssssssssssssssssssssssssssssssssssssss");
        }
    head_resign = (ListElement_type *)malloc(sizeof(ListElement_type));
    head_service = (ListElement_type *)malloc(sizeof(ListElement_type));
    printf("Please enter the number of seats: \n");
    scanf("%d", &waiting_room_chairs);
    
    printf("Please enter the total customers: \n");
    scanf("%d", &total_customers);

    if(total_customers <= 0 || waiting_room_chairs <= 0){
        printf("Entered numbers aren't positive.");
        exit(EXIT_FAILURE);
    }


    if( (pthread_create(&barber, NULL, &Barber, NULL) ) ){
        printf("Thread creation failed: \n");
        return -1;
    }
    if( (pthread_create(&customer, NULL, &Customer, NULL) ) ){
        printf("Thread creation failed: \n");
        return -1;
    }
    pthread_join(customer, NULL);
    pthread_join(barber, NULL);
    sem_destroy(&Customers);
    sem_destroy(&Barbers);

}