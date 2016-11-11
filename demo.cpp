#include "rb_tree.h"
#include <stdio.h>
#include <cstdlib>

/*the satellite data class,which should be customized according to requirement.*/
class satellite {
public:
    satellite(int _fd = 0) : fd(_fd) {}

    ~satellite() { close(fd); }

    int fd;//file descriptor
};


void *insert_ex(void *arg){
    rb_tree<satellite> *data_tree=(rb_tree<satellite> *)arg;
    node<satellite> *new_item=0;
    while (true){
        int r=rand();
        usleep(r%100);
        data_tree->insert(r%100000,new satellite(),&new_item,true);
        //maybe you want to do something after insert a new item to tree,then call insert with 3rd and 4th parameter.
        usleep(r%1000);
        new_item->mutex_unlock();
    }
}
void *insert(void *arg){
    rb_tree<satellite> *data_tree=(rb_tree<satellite> *)arg;
    while (true){
        int r=rand();
        usleep(r%100);
        data_tree->insert(r%100000,new satellite());
    }
}
void *remove(void *arg){
    rb_tree<satellite> *data_tree=(rb_tree<satellite> *)arg;
    while (true){
        int r=rand();
        usleep(r%100);
        data_tree->remove(r%100000);
    }
}
void *search(void *arg){
    rb_tree<satellite> *data_tree=(rb_tree<satellite> *)arg;
    node<satellite> *result=0;
    while (true){
        int r=rand();
        if(result=data_tree->search(r%100000,true)){
            if(r%100==0){
                printf("search:%d\n",data_tree->getCount());
            }
            //do something with result here,mutex lock guarantee safety use of result.
            usleep(r%1000);
            result->mutex_unlock();
        }else{
        	usleep(r%10);
        }
    }
}



int main(int argc,char *argv[]){
    rb_tree<satellite> data_tree;
    pthread_t tid_insert,tid_insert_ex,tid_remote,tid_search_1,tid_search_2,tid_search_3;

    pthread_create(&tid_insert,NULL,insert,(void*)&data_tree);
    pthread_create(&tid_insert_ex,NULL,insert_ex,(void*)&data_tree);
    pthread_create(&tid_remote,NULL,remove,(void*)&data_tree);

    pthread_create(&tid_search_1,NULL,search,(void*)&data_tree);
    pthread_create(&tid_search_2,NULL,search,(void*)&data_tree);
    pthread_create(&tid_search_3,NULL,search,(void*)&data_tree);


    pthread_join(tid_insert,NULL);
    pthread_join(tid_remote,NULL);
    pthread_join(tid_search_1,NULL);
    pthread_join(tid_search_2,NULL);
    pthread_join(tid_search_3,NULL);
}