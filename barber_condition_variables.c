#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define NO_OF_THREADS 20

pthread_mutex_t mutex_srvCust, mutex_controlling_list;
pthread_cond_t customer_ready, barber_ready;
int not_served_custs = 0, WRoom = 0, chair_cnt, served = 0, all_clients = 0, actual_id = 0, debug = 0, id = 0;
int now_serving=0, next_ticket=0;
void* customer_function(void*);
void* barber_function(void*);

typedef struct ListElement {
    int data;
    struct ListElement * next;
} ListElement_type;
ListElement_type *waiting_room_people;
ListElement_type *head_resign; 

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
void print_results(){
    if(debug == 1){
        printf("\nPeople, who resigned");
        show(head_resign);
        printf("People, in queue");
        show(waiting_room_people);
    }
}
void push_front(ListElement_type **head, int number){
    if( pthread_mutex_lock(&mutex_controlling_list) !=0 ){
        perror("During locking mutex");
        exit(EXIT_FAILURE);
    }
        if(*head == NULL){
            *head = (ListElement_type *)malloc(sizeof(ListElement_type));
            if((*head) == NULL){
                perror("Allocation:");
                exit(EXIT_FAILURE);
            } 
            (*head)->data = number;
            if( pthread_mutex_unlock(&mutex_controlling_list) != 0 ){
                perror("During unlocking mutex");
                exit(EXIT_FAILURE);
            }
            return;
        }
        else{
            ListElement_type *current;
            current=(ListElement_type *)malloc(sizeof(ListElement_type));
            if( current == NULL ){
                perror("Allocation:");
                exit(EXIT_FAILURE);
            }
            current->data=number;
            current->next=(*head);
            *head=current;
            if( pthread_mutex_unlock(&mutex_controlling_list) != 0 ){
                perror("During unlocking mutex");
                exit(EXIT_FAILURE);
            }
            return;
        }
}

void push_back_queue(ListElement_type **head, int number){
    if( pthread_mutex_lock(&mutex_controlling_list) != 0){
        perror("During locking mutex");
        exit(EXIT_FAILURE);
    }
    if(*head == NULL){
        *head = (ListElement_type *)malloc(sizeof(ListElement_type));
        if((*head) == NULL){
            perror("Allocation:");
            exit(EXIT_FAILURE);
        } 
        (*head)->data = number;
        (*head)->next = NULL;
        if( pthread_mutex_unlock(&mutex_controlling_list) != 0 ){
            perror("During unlocking mutex");
            exit(EXIT_FAILURE);
        }
        return;
    }
    else{
        ListElement_type *current = *head;
        while (current->next != NULL)
            current = current->next;
        current->next = (ListElement_type *)malloc(sizeof(ListElement_type));
        if(current->next == NULL){
            perror("Allocation:");
            exit(EXIT_FAILURE);
        } 
        current->next->data = number;
        current->next->next = NULL;
        if( pthread_mutex_unlock(&mutex_controlling_list) != 0){
            perror("During unlocking mutex");
            exit(EXIT_FAILURE);
        }
        return;
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
            tmp=(*head)->next;
            free(*head);
            *head=tmp;
        }
        else{
            free(*head);
            *head = NULL;
        }
        if( pthread_mutex_unlock(&mutex_controlling_list) !=0 ) {
            perror("During unlocking mutex");
            exit(EXIT_FAILURE);
        }
        return i;
    }
    if( pthread_mutex_unlock(&mutex_controlling_list) !=0 ) {
        perror("During unlocking mutex");
        exit(EXIT_FAILURE);
    }
    return -1;
}



void* customer_function(void* arg){

    if( pthread_mutex_lock(&mutex_srvCust) != 0){
        perror("During locking mutex");
        exit(EXIT_FAILURE);
    }
    if (WRoom < chair_cnt){
        int my_ticket = next_ticket++;
        push_back_queue(&waiting_room_people, id);
        ++WRoom;
        ++all_clients;
        ++id;
        if( pthread_cond_signal(&customer_ready) != 0 ){
            perror("During emmiting signal");
            exit(EXIT_FAILURE);
        }
        //usypianie
        // usleep(rand()%1000000);
        while(my_ticket != now_serving)
            if( pthread_cond_wait(&barber_ready,&mutex_srvCust) != 0){
                perror("During waiting while conditional");
                exit(EXIT_FAILURE);
            }
        ++now_serving;
                
        printf("\nRes:%d WRomm: %d/%d [in: %d]", not_served_custs, WRoom, chair_cnt, actual_id);
        print_results();
        if( pthread_mutex_unlock(&mutex_srvCust) != 0){
            perror("During unlocking mutex");
            exit(EXIT_FAILURE);
        }

    }
    else{
        ++all_clients;
        push_front(&head_resign, id);
        ++not_served_custs;
        ++id;
        //usypianie
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

void* barber_function(void* arg){
    while(TRUE){
        if( pthread_mutex_lock(&mutex_srvCust) != 0){
            perror("During locking mutex");
            exit(EXIT_FAILURE);
        }
        while(WRoom==0)
            if( pthread_cond_wait(&customer_ready, &mutex_srvCust) != 0 ){
                perror("During conditional waiting");
                exit(EXIT_FAILURE);
            }
        actual_id = pop_front_queue(&waiting_room_people);
        --WRoom;
        ++served;
        printf("\nRes:%d WRomm: %d/%d [in: %d]", not_served_custs, WRoom, chair_cnt, actual_id);            
        print_results();
        //usypianie
        // usleep(20000%+rand()%500000);
        if( pthread_cond_broadcast(&barber_ready) !=0 ){
            perror("During signalling conditional variables");
            exit(EXIT_FAILURE);
        }
        if( pthread_mutex_unlock(&mutex_srvCust) != 0 ){
            perror("During unlocking mutex");
            exit(EXIT_FAILURE);
        }
    }
    pthread_exit(NULL);    
}
int main(int argc, char *argv[]) {
    pthread_t barber;
    srand( time(NULL) );
    
    if(argc >= 2)
        if(strcmp(argv[1], "-debug") == 0)
            debug = 1;
    
    // printf("Please enter the number of seats: \n");
    // scanf("%d", &chair_cnt);
    chair_cnt =5;
    if(chair_cnt < 0){
        printf("Number of seats is innappropriate. Be good");
        exit(EXIT_FAILURE);
    }

    if( pthread_mutex_init(&mutex_srvCust,NULL) != 0){
        perror("Mutex initialization:");
        exit(EXIT_FAILURE);
    }
    if( pthread_mutex_init(&mutex_controlling_list, NULL) != 0 ){
        perror("Mutex initialization:");
        exit(EXIT_FAILURE);
    }
    
    if( pthread_cond_init(&barber_ready, NULL) != 0){
        perror("Condition variables initialization:");
        exit(EXIT_FAILURE);
    }
    if( pthread_cond_init(&customer_ready, NULL) != 0){
        perror("Condition variables initialization:");
        exit(EXIT_FAILURE);
    }

    
    pthread_t customer_thread[NO_OF_THREADS];
    int tmp;
    tmp = pthread_create(&barber, NULL, barber_function, NULL);
    if(tmp){
        perror("While creating thread");
        exit(EXIT_FAILURE);
    }
    while(TRUE){
        for(int counter = 0 ; counter < NO_OF_THREADS; ++counter){
            tmp = pthread_create(&customer_thread[counter], NULL, (void *)customer_function, NULL);
            if(tmp){
                perror("While creating thread");
                exit(EXIT_FAILURE);
            }
        }
        for(int counter = 0 ; counter < NO_OF_THREADS ; ++counter)
            if( pthread_join(customer_thread[counter], NULL) != 0 ){
                perror("While ending thread");
                exit(EXIT_FAILURE);
            }
    }
    if( pthread_join(barber, NULL) != 0 ){
        perror("While ending thread");
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
    
    if( pthread_cond_destroy(&barber_ready) != 0){
        perror("During condition variable destruction:");
        exit(EXIT_FAILURE);
    }
    if( pthread_cond_destroy(&customer_ready) != 0){
        perror("During condition variable destruction:");
        exit(EXIT_FAILURE);
    }
    return 0;
}