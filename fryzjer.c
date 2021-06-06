#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define TRUE 1
#define NO_OF_THREADS 20

pthread_mutex_t mutex, mutex_controlling_list;
pthread_cond_t fill, empty;
int no_served_custs = 0, WRoom = 0, chair_cnt, served = 0, all_clients = 0, actual_id = 0, debug = 0, id = 0;
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
    pthread_mutex_lock(&mutex_controlling_list);
        if(*head == NULL){
            *head = (ListElement_type *)malloc(sizeof(ListElement_type));
            if((*head) == NULL){
                perror("Allocation:");
                exit(EXIT_FAILURE);
            } 
            (*head)->data = number;
            pthread_mutex_unlock(&mutex_controlling_list);
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
            pthread_mutex_unlock(&mutex_controlling_list);
            return;
        }
}

void push_back_queue(ListElement_type **head, int number){
    pthread_mutex_lock(&mutex_controlling_list);
    if(*head == NULL){
        *head = (ListElement_type *)malloc(sizeof(ListElement_type));
        if((*head) == NULL){
            perror("Allocation:");
            exit(EXIT_FAILURE);
        } 
        (*head)->data = number;
        (*head)->next = NULL;
        pthread_mutex_unlock(&mutex_controlling_list);
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
        pthread_mutex_unlock(&mutex_controlling_list);
        return;
    }
}

int pop_front_queue(ListElement_type **head){
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
        return i;
    }
    return -1;
}



void* customer_function(void* arg){

    pthread_mutex_lock(&mutex);
    if (WRoom < chair_cnt){
        int my_ticket = next_ticket++;
        push_back_queue(&waiting_room_people, id);
        ++WRoom;
        ++all_clients;
        ++id;
        pthread_cond_signal(&fill);
        printf("\n\n----->%d", my_ticket);
        //usypianie
        usleep(rand()%1000000);
        while(my_ticket != now_serving)
            pthread_cond_wait(&empty,&mutex);
        ++now_serving;
        printf("\n\n----->%d", now_serving);
                
        printf("\nRes:%d WRomm: %d/%d [in: %d]", no_served_custs, WRoom, chair_cnt, actual_id);
        print_results();
        pthread_mutex_unlock(&mutex);

    }
    else{
        ++all_clients;
        push_front(&head_resign, id);
        ++no_served_custs;
        ++id;
        //usypianie
        usleep(rand()%1000000);

        printf("\nRes:%d WRomm: %d/%d [in: %d]", no_served_custs, WRoom, chair_cnt, actual_id);
        print_results();
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);
}

void* barber_function(void* arg){
    while(TRUE){
        pthread_mutex_lock(&mutex);
        while(WRoom==0)
            pthread_cond_wait(&fill, &mutex);
        actual_id = pop_front_queue(&waiting_room_people);
        --WRoom;
        ++served;
        printf("\nRes:%d WRomm: %d/%d [in: %d]", no_served_custs, WRoom, chair_cnt, actual_id);            
        print_results();
        //usypianie
        usleep(rand()%1000000);
        pthread_cond_broadcast(&empty);
        pthread_mutex_unlock(&mutex);
    }
    pthread_exit(NULL);    
}
int main(int argc, char *argv[]) {
    pthread_t barber;
    srand( time(NULL) );
    
    if(argc >= 2)
        if(strcmp(argv[1], "-debug") == 0)
            debug = 1;
    
    printf("Please enter the number of seats: \n");
    scanf("%d", &chair_cnt);

    pthread_mutex_init(&mutex,NULL);
    pthread_mutex_init(&mutex_controlling_list, NULL);
    pthread_cond_init(&empty, NULL);
    pthread_cond_init(&fill, NULL);

    
    pthread_t customer_thread[NO_OF_THREADS];

    if(pthread_create(&barber, NULL, barber_function, NULL)){
            printf("Failed to create thread.");
            exit(EXIT_FAILURE);
        }
    int tmp;
    while(TRUE){
        for(int counter = 0 ; counter < NO_OF_THREADS; ++counter){
            tmp = pthread_create(&customer_thread[counter], NULL, (void *)customer_function, NULL);
            if(tmp)
                printf("Failed to create thread.");
        }
        for(int counter = 0 ; counter < NO_OF_THREADS ; ++counter)
            pthread_join(customer_thread[counter], NULL);
    }
    return 0;
}