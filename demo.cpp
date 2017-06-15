#include "rb_tree.h"
#include <stdio.h>
#include <cstdlib>

/*the user data class,which should be customized according to requirement. of course, you need to extend satellite class */
class user_data : public satellite {
public:
    user_data(int _fd = 0): fd(_fd) {};
    ~user_data(){ close(fd); }

    int fd; //file descriptor
};



void *insert_ex(void *arg){
    rb_tree *data_tree=(rb_tree*)arg;
    user_data *p=0;
    while (true){
        int r=rand();
        usleep(r%100);
        data_tree->insert(r%100000,p=new user_data());
        usleep(r%1000);
        p->mutex_unlock();
    }
}
void *insert(void *arg){
    rb_tree *data_tree=(rb_tree*)arg;
    while (true){
        int r=rand();
        usleep(r%100);
        data_tree->insert(r%100000,new user_data(),false); //insert without locking it
    }
}
void *remove(void *arg){
    rb_tree *data_tree=(rb_tree*)arg;
    while (true){
        int r=rand();
        usleep(r%100);
        data_tree->remove(r%100000);
    }
}
void *search(void *arg){
    rb_tree *data_tree=(rb_tree*)arg;
    user_data *result=0;
    while (true){
        int r=rand();
        if(result=(user_data*)data_tree->search(r%100000)){
            if(r%100==0){
                printf("search:%d\n",data_tree->size());
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
    rb_tree data_tree;
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