#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <time.h>

#define TRUE 1

pthread_mutex_t mutex;
pthread_cond_t fill, empty;
int no_served_custs = 0, WRoom = 0, chair_cnt, served = 0, all_clients = 0, actual_id = 0, int debug = 0;
void* producer(void*);
void* consumer(void*);

void initialize_variables();
void insert(void);
void remove_item(void);

typedef struct ListElement {
    int data;
    struct ListElement * next;
} ListElement_type;
ListElement_type *waiting_room_people;
ListElement_type *head_resign; 
ListElement_type *head_service;
//dla rezygnujacych resign_serviced == 0, dla obsluzenych resign_service == 1
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

void push_back_queue(ListElement_type **head, int number){
    if(*head == NULL){
        *head = (ListElement_type *)malloc(sizeof(ListElement_type));
        (*head)->data = number;
        (*head)->next = NULL;
    }
    else{
        ListElement_type *current = *head;
        while (current->next != NULL)
            current = current->next;
        current->next = (ListElement_type *)malloc(sizeof(ListElement_type));
        current->next->data = number;
        current->next->next = NULL;
    }
    ++WRoom;
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
        --WRoom;
        return i;
    }
    return -1;
}



void* producer(void* arg){
    pthread_mutex_lock(&mutex);
    if (WRoom < chair_cnt){
        // while(WRoom == chair_cnt){
        //     pthread_cond_wait(&empty,&mutex);
        // }
        push_back_queue(&waiting_room_people, all_clients);
        ++all_clients;
        printf("\nRes:%d WRomm: %d/%d [in: %d], Served:%d. All:%d", no_served_custs, WRoom, chair_cnt, actual_id, served, all_clients);
        pthread_mutex_unlock(&mutex);
    }
    else{
        ++no_served_custs;
        ++all_clients;
        printf("\nRes:%d WRomm: %d/%d [in: %d], Served:%d. All:%d", no_served_custs, WRoom, chair_cnt, actual_id, served, all_clients);
        pthread_cond_signal(&fill);
        pthread_mutex_unlock(&mutex);

    }
    pthread_exit(NULL);
}

void* consumer(void* arg){
    int kid;
    while(TRUE){
        pthread_mutex_lock(&mutex);
            while(WRoom==0){
                pthread_cond_wait(&fill,&mutex);
            }

            kid = pop_front_queue(&waiting_room_people);
            actual_id = kid;
            ++served;
            printf("\nRes:%d WRomm: %d/%d [in: %d], Served:%d. All:%d", no_served_custs, WRoom, chair_cnt, kid, served, all_clients);            
            pthread_cond_broadcast(&empty);

        pthread_mutex_unlock(&mutex);

    }
}
int main(int argc, char *argv[]) {

    pthread_t barber;
    waiting_room_people = (ListElement_type *)malloc(sizeof(ListElement_type));
    if(waiting_room_people == NULL){
        perror("While allocating memory");
        exit(1);
    }
    srand( time(NULL) );
    
    if(argc >= 2)
        if(strcmp(argv[1], "-debug") == 0)
            debug = 1;
    
    printf("Please enter the number of seats: \n");
    scanf("%d", &chair_cnt);

    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&empty, NULL);
    pthread_cond_init(&fill, NULL);

    if(pthread_create(&barber, NULL, consumer, NULL))
        exit(2);

    while(TRUE){
        pthread_t customer[500];
        for(int i=0; i< 500; ++i){
            if(pthread_create(&customer[i],NULL,producer,NULL))
                exit(1);
        }
        for(int i=0; i< 500; ++i){
            pthread_join(customer[i], NULL);
        }
    }

    printf("Exit the program\n");
    return 0;
}