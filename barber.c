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

#define NO_OF_THREADS 500
void * barber_function(void *idp);
void * customer_function(void *idp);
void service_customer();
void * make_customer_function();

//mutexy
pthread_mutex_t srvCust, mutex_controlling_list;
//semafory
sem_t barber_ready, customer_ready;
//wszystkie zmienne
int chair_cnt, ended = 0, all_clients = 0, id = 0, WRoom=0, actual_id = 0, no_served_custs = 0, served = 0, debug = 0;

//lista trzymajaca klientow - kolejka
typedef struct ListElement {
    int data;
    struct ListElement * next;
} ListElement_type;

//deklaracja list
ListElement_type *head_resign; 
ListElement_type *head_service;
ListElement_type *waiting_room_people;

void show(ListElement_type *head){
    printf("\n");
    if(head==NULL)
        printf("List is empty");
    else{
        ListElement_type *current=head;
        do{
            printf("%i ", current->data);
            current = current->next;
        }while (current != NULL);
        printf("\n");
    }
}

void push_front(ListElement_type **head, int number, int resign_serviced){
    pthread_mutex_lock(&mutex_controlling_list);
    if(resign_serviced == 0){
        if(no_served_custs == 0){
            (*head)->data = number;
            pthread_mutex_unlock(&mutex_controlling_list);
            return;
        }
        if(no_served_custs > 0){
            ListElement_type *current;
            current=(ListElement_type *)malloc(sizeof(ListElement_type));
            current->data=number;
            current->next=(*head);
            *head=current;
            pthread_mutex_unlock(&mutex_controlling_list);
            return;
        }
    }
    if(resign_serviced == 1){
        if(served == 0){
            (*head)->data = number;
            pthread_mutex_unlock(&mutex_controlling_list);
            return;
        }
        if(served > 0){
            ListElement_type *current;
            current = (ListElement_type *)malloc(sizeof(ListElement_type));
            current->data = number;
            current->next = (*head);
            *head = current;
            pthread_mutex_unlock(&mutex_controlling_list);
            return;
        }
    }
}

void print_results(){
    if(debug == 1){
        printf("\nPeople, who resigned");
        show(head_resign);
        printf("People, who was serviced");
        show(head_service);
    }
}

void push_back_queue(ListElement_type **head, int number){
    pthread_mutex_lock(&mutex_controlling_list);
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
        pthread_mutex_unlock(&mutex_controlling_list);
}

int pop_front_queue(ListElement_type **head){
    int i;
    ListElement_type * tmp=NULL;
    if (*head!=NULL && WRoom != 0) {
        i = (*head)->data;
        if( (*head)->next !=NULL){
   	        tmp=(*head)->next;
            free(*head);
           *head=tmp;
        }
        return i;
	}
    return -1;
}

void * barber_function(void *idp){        
    while(1){
        sem_wait(&customer_ready);
        pthread_mutex_lock(&srvCust);
        if(errno != 0){
            perror("Hi");
            exit(EXIT_FAILURE);
        }
        actual_id = pop_front_queue(&waiting_room_people);
        push_front(&head_service, actual_id, 1);
        --WRoom; 
        ++all_clients; 
        ++served;
        printf("\nRes:%d WRomm: %d/%d [in: %d], Served:%d. All:%d", no_served_custs, WRoom, chair_cnt, actual_id, served, all_clients);
        print_results();

        pthread_mutex_unlock(&srvCust);

        sem_post(&barber_ready);        
        
    }
    pthread_exit(NULL);    
}

void * customer_function(void *idp){  
    pthread_mutex_lock(&srvCust);

    if(WRoom < chair_cnt)
    {
        push_back_queue(&waiting_room_people, id);
        ++WRoom;
        ++id;
        printf("\nRes:%d WRomm: %d/%d [in: %d], Served:%d. All:%d", no_served_custs, WRoom, chair_cnt, actual_id, served, all_clients);            
        print_results();
        sem_post(&customer_ready);

        pthread_mutex_unlock(&srvCust);

        sem_wait(&barber_ready); 
    }
    else
    {
        ++all_clients;
        push_front(&head_resign, id, 0);
        ++id;
        ++no_served_custs;
        printf("\nRes:%d WRomm: %d/%d [in: %d], Served:%d. All:%d", no_served_custs, WRoom, chair_cnt, actual_id, served, all_clients);
        print_results();
        pthread_mutex_unlock(&srvCust);
    }
    pthread_exit(NULL);
}


int main(int argc, char**argv){
    if(argc >= 2)
        if(strcmp(argv[1], "-debug") == 0)
            debug = 1;
    
    printf("Please enter the number of seats: \n");
    scanf("%d", &chair_cnt);

    if(chair_cnt < 0){
        printf("Number of seats is innappropriate. Be good");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    pthread_t barber_1;
    int tmp;
    waiting_room_people = (ListElement_type *)malloc(sizeof(ListElement_type));
    head_resign = (ListElement_type *)malloc(sizeof(ListElement_type));
    head_service = (ListElement_type *)malloc(sizeof(ListElement_type));

    pthread_mutex_init(&srvCust, NULL);

    sem_init(&customer_ready, 0, 0);
    sem_init(&barber_ready, 0, 0);
    pthread_mutex_init(&srvCust,NULL);
    pthread_mutex_init(&mutex_controlling_list, NULL);
    
    

    tmp = pthread_create(&barber_1, NULL, (void *)barber_function, NULL);  
    if(tmp)
        printf("Failed to create thread."); 
    while(1){
        pthread_t customer_thread[NO_OF_THREADS];
        for(int counter = 0 ; counter < NO_OF_THREADS; ++counter){
            tmp = pthread_create(&customer_thread[counter], NULL, (void *)customer_function, NULL);
            if(tmp)
                printf("Failed to create thread.");
        }
        for(int counter = 0 ; counter < NO_OF_THREADS ; ++counter)
            pthread_join(customer_thread[counter], NULL);
    }
     
    pthread_join(barber_1, NULL);
    exit(EXIT_SUCCESS);
}