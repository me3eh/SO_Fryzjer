#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>

#define NO_OF_THREADS 20
#define TRUE 1
void * barber_function(void *idp);
void * customer_function(void *idp);

//mutexy
pthread_mutex_t mutex_srvCust, mutex_controlling_list;
//semafory
sem_t barber_ready, customer_ready;
//wszystkie zmienne
int chair_cnt, id = 0, WRoom=0, actual_id = 0, not_served_custs = 0, served = 0, debug = 0;

//lista trzymajaca klientow - kolejka
typedef struct ListElement {
    int data;
    struct ListElement * next;
} ListElement_type;

//deklaracja list
ListElement_type *head_resign;
ListElement_type *waiting_room_people;

void show(ListElement_type *head){
    printf("\n");
    if(head==NULL)
        printf("List is empty\n");
    else{
        ListElement_type *current=head;
        do{
            printf("%i ", current->data);
            current = current->next;
        }while (current != NULL);
        printf("\n");
    }
}

void push_front(ListElement_type **head, int number){
    if( pthread_mutex_lock(&mutex_controlling_list) != 0 ){
        perror("During locking mutex");
        exit(EXIT_FAILURE);
    }
    if(*head == NULL){
        *head = (ListElement_type *)malloc(sizeof(ListElement_type));
        if(*head == NULL){
            perror("Allocation");
            exit(EXIT_FAILURE);
        } 
        (*head)->data = number;
        if( pthread_mutex_unlock(&mutex_controlling_list) != 0){
            perror("During unlocking mutex");
            exit(EXIT_FAILURE);
        }
        return;
    }
    else{
        ListElement_type *current;
        current=(ListElement_type *)malloc(sizeof(ListElement_type));
        if(current == NULL){
            perror("Allocation");
            exit(EXIT_FAILURE);
        }
        current->data=number;
        current->next=(*head);
        *head=current;
        if ( pthread_mutex_unlock(&mutex_controlling_list) != 0 ){
            perror("During unlocking mutex");
            exit(EXIT_FAILURE);
        }
        return;
    }
}

void print_results(){
    if(debug == 1){
        printf("\nPeople, who resigned");
        show(head_resign);
        printf("People, in queue");
        show(waiting_room_people);
    }
}

void push_back_queue(ListElement_type **head, int number){
    if( pthread_mutex_lock(&mutex_controlling_list) != 0){
        perror("During locking mutex");
        exit(EXIT_FAILURE);
    }
    if(*head == NULL){
        *head = (ListElement_type *)malloc(sizeof(ListElement_type));
        if (*head == NULL){
            perror("Allocation:");
            exit(EXIT_FAILURE);
        }
        (*head)->data = number;
        (*head)->next = NULL;
    }
    else{
        ListElement_type *current = *head;
        while (current->next != NULL)
            current = current->next;
        current->next = (ListElement_type *)malloc(sizeof(ListElement_type));
        if (current->next == NULL){
            perror("Allocation:");
            exit(EXIT_FAILURE);
        }
        current->next->data = number;
        current->next->next = NULL;
    }
    if( pthread_mutex_unlock(&mutex_controlling_list) != 0 ){
        perror("During unlocking mutex");
        exit(EXIT_FAILURE);
    }
}

int pop_front_queue(ListElement_type **head){
    if( pthread_mutex_lock(&mutex_controlling_list) !=0 ) {
        perror("During locking mutex");
        exit(EXIT_FAILURE);
    }

    int i;
    ListElement_type * tmp=NULL;
    if (*head!=NULL) {
        i = (*head)->data;
        if( (*head)->next !=NULL){
   	        tmp = (*head)->next;
            free(*head);
           *head = tmp;
        }
        else{
            free(*head);
            *head = NULL;
        }
        if( pthread_mutex_unlock(&mutex_controlling_list) != 0){
            perror("During unlocking mutex");
            exit(EXIT_FAILURE);
        }
        return i;
	}
    if( pthread_mutex_unlock(&mutex_controlling_list) != 0){
        perror("During unlocking mutex");
        exit(EXIT_FAILURE);
    }
    return -1;
}

void * barber_function(void *idp){        
    while(TRUE){
        if( sem_wait(&customer_ready) != 0){
            perror("During waiting semaphor");
            exit(EXIT_FAILURE);
        }
        if( pthread_mutex_lock(&mutex_srvCust) !=0 ){
            perror("During locking mutex");
            exit(EXIT_FAILURE);
        }

        actual_id = pop_front_queue(&waiting_room_people);

        --WRoom; 

        ++served;
        printf("\nRes:%d WRomm: %d/%d [in: %d]", not_served_custs, WRoom, chair_cnt, actual_id);
        print_results();

        if( pthread_mutex_unlock(&mutex_srvCust) != 0){
            perror("During unlocking mutex");
            exit(EXIT_FAILURE);
        }
        // usleep(20000%+rand()%500000);

        if( sem_post(&barber_ready) !=0 ){
            perror("During signalling semaphor");
            exit(EXIT_FAILURE);
        }      
        
    }
    pthread_exit(NULL);    
}

void * customer_function(void *idp){  
    if( pthread_mutex_lock(&mutex_srvCust) !=0 ) {
        perror("During locking mutex");
        exit(EXIT_FAILURE);
    }

    if(WRoom < chair_cnt){
        if( sem_post(&customer_ready) !=0 ){
            perror("During semaphor signalling");
            exit(EXIT_FAILURE);
        }
        ++WRoom;
        // usleep(rand()%1000000);

        push_back_queue(&waiting_room_people, id);
        ++id;
        printf("\nRes:%d WRomm: %d/%d [in: %d]", not_served_custs, WRoom, chair_cnt, actual_id);            
        print_results();

        if( pthread_mutex_unlock(&mutex_srvCust) !=0 ){
            perror("During unlocking mutex");
            exit(EXIT_FAILURE);
        }
        if( sem_wait(&barber_ready) !=0 ){
            perror("During semaphor waiting");
            exit(EXIT_FAILURE);
        } 

    }
    else{
        push_front(&head_resign, id);
        ++id;
        ++not_served_custs;
        // usleep(rand()%1000000);
        printf("\nRes:%d WRomm: %d/%d [in: %d]", not_served_custs, WRoom, chair_cnt, actual_id);
        print_results();
        if( pthread_mutex_unlock(&mutex_srvCust) != 0){
            perror("During unlocking mutex");
            exit(EXIT_FAILURE);
        }
    }

    pthread_exit(NULL);
}


int main(int argc, char**argv){
    if(argc >= 2)
        if(strcmp(argv[1], "-debug") == 0)
            debug = 1;
    
    // printf("Please enter the number of seats: \n");
    // scanf("%d", &chair_cnt);
    chair_cnt = 5;
    // if(chair_cnt < 0){
        // printf("Number of seats is innappropriate. Be good");
        // exit(EXIT_FAILURE);
    // }

    pthread_t barber_1;
    int tmp;

    if( pthread_mutex_init(&mutex_srvCust, NULL) != 0 ){
        perror("Condition variables initialization:");
        exit(EXIT_FAILURE);
    }
    if( sem_init(&customer_ready, 0, 0) != 0) {
        perror("Condition variables initialization:");
        exit(EXIT_FAILURE);
    }
    if( sem_init(&barber_ready, 0, 0) != 0) {
        perror("Condition variables initialization:");
        exit(EXIT_FAILURE);
    }
    if( pthread_mutex_init(&mutex_controlling_list, NULL) !=0 ){
        perror("Condition variables initialization:");
        exit(EXIT_FAILURE);
    }
    
    

    tmp = pthread_create(&barber_1, NULL, (void *)barber_function, NULL);  
    if(tmp){
        printf("While creating thread");
        exit(EXIT_FAILURE);
    } 
    while(TRUE){
        pthread_t customer_thread[NO_OF_THREADS];
        for(int counter = 0 ; counter < NO_OF_THREADS; ++counter){
            tmp = pthread_create(&customer_thread[counter], NULL, (void *)customer_function, NULL);
            if(tmp){
                perror("While creating thread");
                exit(EXIT_FAILURE);
            }
        }
        for(int counter = 0 ; counter < NO_OF_THREADS ; ++counter)
            if( pthread_join(customer_thread[counter], NULL) !=0 ){
                perror("While ending thread");
                exit(EXIT_FAILURE);
            }
    }
     
    if( pthread_join(barber_1, NULL) !=0 ){
        perror("While ending thread");
        exit(EXIT_FAILURE);
    }
    if( sem_destroy(&barber_ready) != 0){
        perror("During semaphor destruction:");
        exit(EXIT_FAILURE);
    }
    if( sem_destroy(&customer_ready) != 0){
        perror("During semaphor destruction:");
        exit(EXIT_FAILURE);
    }
    if( pthread_mutex_destroy(&mutex_srvCust) != 0){
        perror("During mutex destruction:");
        exit(EXIT_FAILURE);
    }
    if( pthread_mutex_destroy(&mutex_controlling_list) != 0 ){
        perror("During mutex destruction:");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}