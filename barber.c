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
int actual_id;
int id=0;
sem_t barber_works_semaphore;
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
    if(resign_serviced == 0)
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
    if(resign_serviced == 1)
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
ListElement_type *head_resign; //= (ListElement_type *)malloc(sizeof(ListElement_type));
ListElement_type *head_service; //= (ListElement_type *)malloc(sizeof(ListElement_type));
///////////////////////////////////////////////


void *Barber(void *arg) {
	while(1) {
			/* waits for a customer (sleeps). */
			// down(Customers);
            sem_wait(&Customers);
            sem_wait(&mod_seats);

            printf("3");
            // sem_post(&Customers);
			/* mutex to protect the number of available seats.*/
			// down(Seats);
            // pthread_mutex_lock(&Seats);

			/* a chair gets free.*/
			// FreeSeats++;
            --WRoom;
            // printf("lk");
            printf("%d == WRoom:%d\n", id, WRoom);
            sem_post(&mod_seats);
            // pthread_mutex_unlock(&Seats);
            usleep(rand()%10000000);
			/* bring customer for haircut.*/
			// up(Barber);
            // sem_post(&Barbers);
			sem_wait(&Barbers);
			/* release the mutex on the chair.*/
			// up(Seats);
			/* barber is cutting hair.*/
	}
}

void * Customer(void * arg) {
	while(1) {
            usleep(30000 + rand()%100000);
			/* protects seats so only 1 customer tries to sit
			in a chair if that's the case.*/
			//down(Seats); //This line should not be here.
			// pthread_mutex_lock(&Seats);
            sem_wait(&mod_seats);
            if(WRoom < 10) {
				/* sitting down.*/
				++WRoom;
				++id;
				/* notify the barber. */
				// up(Customers);
                // sem_post(&Customers);
                sem_post(&Customers);
                sem_post(&mod_seats);
				
				/* release the lock */
				// up(Seats);
                // pthread_mutex_unlock(&Seats);
				
				/* wait in the waiting room if barber is busy. */
                // sem_wait(&Barbers);
                sem_post(&Barbers);
                printf("%d == WRoom:%d\n", id, WRoom);
                // usleep(100000);
				// down(Barber);
				// customer is having hair cut
			}
            if(WRoom >= 10)
            {
				/* release the lock */
				// up(Seats);
                printf("%d--=\n", id);
                sem_post(&mod_seats);
                // pthread_mutex_unlock(&Seats);
				// customer leaves
			}
	}
}

int main(int argc, char ** argv){
    srand(time(NULL));
    sem_init(&Barbers, 0, 0);
    sem_init(&Customers, 0, 0);
    sem_init(&mod_seats, 0, 1);

    if(argc >= 2)
        if(strcmp(argv[1], "debug") == 0){
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
    sem_destroy(&barber_works_semaphore);
}