#ifndef RB_TREE
#define RB_TREE

#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

const int BLACK = 0;
const int RED = 1;




class satellite {
public:
    satellite(){
        mutex = new pthread_mutex_t();
        pthread_mutex_init(mutex,NULL);
    }

    virtual ~satellite() {
        pthread_mutex_destroy(mutex);
        delete mutex;
    }
    int mutex_trylock() { return pthread_mutex_trylock(mutex); }

    void mutex_lock() { pthread_mutex_lock(mutex); }

    void mutex_unlock() { pthread_mutex_unlock(mutex); }
private:
    pthread_mutex_t *mutex;
};


/*the node of the red&black tree*/
class node {
public:
    node(long _key = 0, satellite *_p = 0) : key(_key), parent(0), left(0), right(0), color(BLACK), p(_p){}

    ~node() {
        delete p;        //automatically release memory in heap
    }

    void free_tree(){
        if(left)
            left->free_tree();
        if(right)
            right->free_tree();
        delete this;
    }

    long key;
    node *parent;
    node *left;
    node *right;
    int color;

    satellite *p;
};


/*the tree*/
class rb_tree {
public:
    rb_tree() : nil(0), root(0), count(0), rwlock(0) {
        nil = new node();
        root = nil;
        rwlock = new pthread_rwlock_t();
        pthread_rwlock_init(rwlock, NULL);
    }

    ~rb_tree() {
        rwlock_wrlock();
        if(root!=nil){
            root->free_tree();
        }
        delete nil;
        rwlock_unlock();
        pthread_rwlock_destroy(rwlock);
        delete rwlock;
    }

    /**
     * 查找一条数据，未找到时返回NULL，找到时返回指向数据的指针
     */
    satellite *search(long _key, bool auto_lock = false) {
        rwlock_rdlock();
        node *result = 0;
        if (rb_search(_key, &result) == -1){
        	rwlock_unlock();
            return NULL;
        }
        if (auto_lock){
            result->p->mutex_lock();
        }
        rwlock_unlock();
        return result->p;
    }

    /**
     * 查找数据，
     * lock=0 不上锁           找到数据时返回0，否则返回-1
     * lock=1 上锁             找到数据时返回0，否则返回-1   可能阻塞
     * lock=2 尝试上锁         未找到数据时返回-1，找到数据但上锁失败时返回1，找到数据并上锁成功时返回0
     */
    int search_try(long _key, satellite **result,int lock=0){
        rwlock_rdlock();
        node *node_p;
        if (rb_search(_key, &node_p) == -1){
            rwlock_unlock();
            return -1;
        }
        int ret=0;
        if (lock==1){
            node_p->p->mutex_lock();
        }else if(lock==2 && node_p->p->mutex_trylock()!=0){
            ret=1;
        }
        rwlock_unlock();
        (*result)=node_p->p;
        return ret;
    }

    /**
     * 插入一条数据，key重复时返回1，并覆盖原数据，正常时返回0
     */
    int insert(long _key, satellite *data, bool auto_lock = false) {
        rwlock_wrlock();
        int result_code = rb_insert(_key, data);
        if (auto_lock)
            data->mutex_lock();
        if (result_code == 0)
            count++;
        rwlock_unlock();
        return result_code;
    }


    /**
     * 尝试插入一条数据，key重复时返回1，并覆盖原数据，
     * 正常时返回0，
     * key重复且插入（上锁）失败时返回-1，同时释放data的内存
     */
    int insert_try(long _key, satellite *data, bool auto_lock = false){
        rwlock_wrlock();
        int result_code = rb_insert_try(_key, data);
        if(result_code==-1){//加锁失败
            delete data;
            return -1;
        }
        if (auto_lock){
            data->mutex_lock();
        }
        if (result_code == 0)
            count++;
        rwlock_unlock();
        return result_code;
    }

    /**
     * 移除一条数据，成功时返回0，未找到指定数据时返回-1
     */
    int remove(long _key) {
        rwlock_wrlock();
        node *result = 0;
        if (rb_search(_key, &result) == 0) {
            rb_delete(result);
            count--;
            rwlock_unlock();
            return 0;
        } else {
            rwlock_unlock();
            return -1;
        }
    }

    /**
     *从小到大，返回第index个数据
     */
    satellite *get(int index, bool auto_lock = false) {
        rwlock_rdlock();
        if (root == nil || index >= count || index < 0) {
            rwlock_unlock();
            return NULL;
        }
        node *result = tree_minimum(root);
        while (index > 0) {
            result = tree_successor(result);
            index--;
        }
        if (auto_lock)
            result->p->mutex_lock();
        rwlock_unlock();
        return result->p;
    }

    int size(){
        return count;
    }



private:
    node *nil;
    node *root;
    int count;
    pthread_rwlock_t *rwlock;

    void rwlock_rdlock() { pthread_rwlock_rdlock(rwlock); }

    void rwlock_wrlock() { pthread_rwlock_wrlock(rwlock); }

    void rwlock_unlock() { pthread_rwlock_unlock(rwlock); }

    void rb_reset_root();

    void left_rotate(node *x);

    void right_rotate(node *x);

    void rb_insert_fixup(node *z);

    void rb_delete_fixup(node *x);

    int rb_delete(node *z);

    int rb_insert(long _key, satellite *data);

    int rb_insert_try(long _key, satellite *data);

    int rb_search(long _key, node **result);

    node *tree_minimum(node *x);

    node *tree_maximum(node *x);

    node *tree_successor(node *x);
};


inline void rb_tree::rb_reset_root() {
    if (root == nil)
        return;
    while (root->parent != nil)
        root = root->parent;
    return;
}

inline node *rb_tree::tree_minimum(node *x) {
    while (x->left != nil)
        x = x->left;
    return x;
}

inline node *rb_tree::tree_maximum(node *x) {
    while (x->right != nil)
        x = x->right;
    return x;
}

inline node *rb_tree::tree_successor(node *x) {
    if (x->right != nil)
        return tree_minimum(x->right);
    node *y = x->parent;
    while (y != nil && x == y->right) {
        x = y;
        y = y->parent;
    }
    return y;
}


void rb_tree::left_rotate(node *x) {
    node *y;
    y = x->right;
    x->right = y->left;
    if (y->left != nil)
        y->left->parent = x;
    y->parent = x->parent;
    if (x->parent == nil) {
        root = y;
    } else {
        if (x == x->parent->left)
            x->parent->left = y;
        else
            x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
    rb_reset_root();
}

void rb_tree::right_rotate(node *x) {
    node *y;
    y = x->left;
    x->left = y->right;
    if (y->right != nil)
        y->right->parent = x;
    y->parent = x->parent;
    if (x->parent == nil) {
        root = y;
    } else {
        if (x == x->parent->right)
            x->parent->right = y;
        else
            x->parent->left = y;
    }
    y->right = x;
    x->parent = y;
    rb_reset_root();
}

void rb_tree::rb_insert_fixup(node *z) {
    node *y;
    while (z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            y = z->parent->parent->right;
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                right_rotate(z->parent->parent);
            }
        } else {
            y = z->parent->parent->left;
            if (y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                left_rotate(z->parent->parent);
            }
        }
    }
    root->color = BLACK;
}

int rb_tree::rb_insert(long _key, satellite *data) {
    node *z = new node(_key, data);
    z->color = RED;
    z->left = nil;
    z->right = nil;
    z->parent = 0;

    node *y = nil;
    node *x = root;

    while (x != nil) {
        y = x;
        if (z->key < x->key) {
            x = x->left;
        } else if (z->key > x->key) {
            x = x->right;
        } else {
            x->p->mutex_lock();
            x->p->mutex_unlock();
            satellite *temp = x->p;
            x->p = z->p;
            z->p = temp;
            delete z;
            return 1;
        }
    }
    z->parent = y;

    if (y == nil) {
        root = z;
    } else {
        if (z->key < y->key)
            y->left = z;
        else
            y->right = z;
    }
    rb_insert_fixup(z);
    return 0;
}

int rb_tree::rb_insert_try(long _key, satellite *data) {
    node *z = new node(_key, data);
    z->color = RED;
    z->left = nil;
    z->right = nil;
    z->parent = 0;

    node *y = nil;
    node *x = root;

    while (x != nil) {
        y = x;
        if (z->key < x->key) {
            x = x->left;
        } else if (z->key > x->key) {
            x = x->right;
        } else {
            if(x->p->mutex_trylock()==0){
                satellite *temp = x->p;
                x->p = z->p;
                z->p = temp;
                delete z;
                x->p->mutex_unlock();
                return 1;
            }else{
                delete z;
                return -1;
            }   
        }
    }
    z->parent = y;

    if (y == nil) {
        root = z;
    } else {
        if (z->key < y->key)
            y->left = z;
        else
            y->right = z;
    }
    rb_insert_fixup(z);
    return 0;
}


void rb_tree::rb_delete_fixup(node *x) {
    node *w;
    while (x != root && x->color == BLACK) {
        if (x == x->parent->left) {
            w = x->parent->right;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                left_rotate(x->parent);
                w = x->parent->right;
            }
            if (w->left->color == BLACK && w->right->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->right->color == BLACK) {
                    w->left->color = BLACK;
                    w->color = RED;
                    right_rotate(w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->right->color = BLACK;
                left_rotate(x->parent);
                x = root;
            }
        } else {
            w = x->parent->left;
            if (w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                right_rotate(x->parent);
                w = x->parent->left;
            }
            if (w->right->color == BLACK && w->left->color == BLACK) {
                w->color = RED;
                x = x->parent;
            } else {
                if (w->left->color == BLACK) {
                    w->right->color = BLACK;
                    w->color = RED;
                    left_rotate(w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                w->left->color = BLACK;
                right_rotate(x->parent);
                x = root;
            }
        }
    }
    x->color = BLACK;
}

int rb_tree::rb_delete(node *z) {
    node *y;
    node *x;
    if (z->left == nil || z->right == nil) {
        y = z;
    } else {
        y = tree_successor(z);
    }
    if (y->left != nil) {
        x = y->left;
    } else {
        x = y->right;
    }
    x->parent = y->parent;
    if (y->parent == nil) {
        root = x;
    } else {
        if (y == y->parent->left) {
            y->parent->left = x;
        } else {
            y->parent->right = x;
        }
    }
    if (y != z) {
        satellite *p_temp = z->p;
        z->key = y->key;
        z->p = y->p;
        y->p = p_temp;
    }
    if (y->color == BLACK) {
        rb_delete_fixup(x);
    }
    y->p->mutex_lock();
    y->p->mutex_unlock();
    delete y;
    return 0;
}


int rb_tree::rb_search(long _key, node **result) {
    node *_root = root;
    while (_root != nil && _root->key != _key) {
        if (_root->key > _key)
            _root = _root->left;
        else
            _root = _root->right;
    }
    *result = _root;
    if (_root == nil) {
        return -1;
    } else {
        return 0;
    }
}

#endif

