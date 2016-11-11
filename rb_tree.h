#ifndef RB_TREE
#define RB_TREE

#include <unistd.h>
#include <pthread.h>

const int BLACK = 0;
const int RED = 1;

/*the node of the red&black tree*/
template<class T>
class node {
public:
    node(long _key = 0, T *_pointer = 0) : key(_key), parent(0), left(0), right(0), color(BLACK), pointer(_pointer),
                                           mutex(0) {
        mutex = new pthread_mutex_t();
        pthread_mutex_init(mutex, NULL);
    }

    ~node() {
        delete pointer;        //automatically release memory in heap
        pthread_mutex_destroy(mutex);
        delete mutex;
    }

    void mutex_lock() { pthread_mutex_lock(mutex); }

    void mutex_unlock() { pthread_mutex_unlock(mutex); }

    long key;
    node *parent;
    node *left;
    node *right;
    int color;
    T *pointer;
private:
    pthread_mutex_t *mutex;
};


/*the tree*/
template<class T>
class rb_tree {
public:
    rb_tree() : nil(0), root(0), count(0), rwlock(0) {
        nil = new node<T>();
        root = nil;
        rwlock = new pthread_rwlock_t();
        pthread_rwlock_init(rwlock, NULL);
    }

    ~rb_tree() {
        pthread_rwlock_destroy(rwlock);
        delete rwlock;
        delete nil;
    }

    /**
     * 查找一条数据，未找到时返回NULL，找到时返回指向数据的指针
     */
    node<T> *search(long _key, bool auto_lock = false) {
        rwlock_rdlock();
        node<T> *result = 0;
        int result_code = rb_search(_key, &result);
        if (result_code == 1){
        	rwlock_unlock();
            return NULL;
        }
        if (auto_lock){
            result->mutex_lock();
        }
        rwlock_unlock();
        return result;
    }

    /**
     * 插入一条数据，key重复时返回1，并覆盖原数据，正常时返回0
     * result不为null时，将result指向新插入的数据
     */
    int insert(long _key, T *data, node<T> **result = NULL, bool auto_lock = false) {
        rwlock_wrlock();
        int result_code = rb_insert(_key, data, result);
        if (auto_lock)
            (*result)->mutex_lock();
        if (result_code == 0)
            count++;
        rwlock_unlock();
        return result_code;
    }

    /**
     * 移除一条数据，成功时返回0，未找到指定数据时返回1
     */
    int remove(long _key) {
        rwlock_wrlock();
        node<T> *result = 0;
        if (rb_search(_key, &result) == 0) {
            rb_delete(result);
            count--;
            rwlock_unlock();
            return 0;
        } else {
            rwlock_unlock();
            return 1;
        }
    }

    node<T> *get(int index, bool auto_lock = false) {
        rwlock_rdlock();
        if (root == nil || index >= count || index < 0) {
            rwlock_unlock();
            return NULL;
        }
        node<T> *result = tree_minimum(root);
        while (index > 0) {
            result = tree_successor(result);
            index--;
        }
        if (auto_lock)
            result->mutex_lock();
        rwlock_unlock();
        return result;
    }

    node<T> *getNext(node<T> *x, bool auto_lock = false) {
        rwlock_rdlock();
        node<T> *result = tree_successor(x);
        if (result == nil) {
            rwlock_unlock();
            return NULL;
        }
        if (auto_lock)
            result->mutex_lock();
        rwlock_unlock();
        return result;
    }

    int getCount() { return count; }


private:
    node<T> *nil;
    node<T> *root;
    int count;
    pthread_rwlock_t *rwlock;

    void rwlock_rdlock() { pthread_rwlock_rdlock(rwlock); }

    void rwlock_wrlock() { pthread_rwlock_wrlock(rwlock); }

    void rwlock_unlock() { pthread_rwlock_unlock(rwlock); }

    void rb_reset_root();

    void left_rotate(node<T> *x);

    void right_rotate(node<T> *x);

    void rb_insert_fixup(node<T> *z);

    void rb_delete_fixup(node<T> *x);

    int rb_delete(node<T> *z);

    int rb_insert(long _key, T *data, node<T> **result);

    int rb_search(long _key, node<T> **result);

    node<T> *tree_minimum(node<T> *x);

    node<T> *tree_maximum(node<T> *x);

    node<T> *tree_successor(node<T> *x);
};


template<class T>
inline void rb_tree<T>::rb_reset_root() {
    if (root == nil)
        return;
    while (root->parent != nil)
        root = root->parent;
    return;
}

template<class T>
inline node<T> *rb_tree<T>::tree_minimum(node<T> *x) {
    while (x->left != nil)
        x = x->left;
    return x;
}

template<class T>
inline node<T> *rb_tree<T>::tree_maximum(node<T> *x) {
    while (x->right != nil)
        x = x->right;
    return x;
}

template<class T>
inline node<T> *rb_tree<T>::tree_successor(node<T> *x) {
    if (x->right != nil)
        return tree_minimum(x->right);
    node<T> *y = x->parent;
    while (y != nil && x == y->right) {
        x = y;
        y = y->parent;
    }
    return y;
}


template<class T>
void rb_tree<T>::left_rotate(node<T> *x) {
    node<T> *y;
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

template<class T>
void rb_tree<T>::right_rotate(node<T> *x) {
    node<T> *y;
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

template<class T>
void rb_tree<T>::rb_insert_fixup(node<T> *z) {
    node<T> *y;
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

template<class T>
int rb_tree<T>::rb_insert(long _key, T *data, node<T> **result) {
    node<T> *z = new node<T>(_key, data);
    z->color = RED;
    z->left = nil;
    z->right = nil;
    z->parent = 0;

    node<T> *y = nil;
    node<T> *x = root;

    while (x != nil) {
        y = x;
        if (z->key < x->key) {
            x = x->left;
        } else if (z->key > x->key) {
            x = x->right;
        } else {
            x->mutex_lock();
            x->mutex_unlock();
            T *temp = x->pointer;
            x->pointer = z->pointer;
            z->pointer = temp;
            delete z;
            if (result != NULL) {
                *result = x;
            }
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
    if (result != NULL) {
        *result = z;
    }
    return 0;
}


template<class T>
void rb_tree<T>::rb_delete_fixup(node<T> *x) {
    node<T> *w;
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

template<class T>
int rb_tree<T>::rb_delete(node<T> *z) {
    node<T> *y;
    node<T> *x;
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
        T *temp = z->pointer;
        z->key = y->key;
        z->pointer = y->pointer;
        y->pointer = temp;
    }
    if (y->color == BLACK) {
        rb_delete_fixup(x);
    }
    y->mutex_lock();
    y->mutex_unlock();
    delete y;
    return 0;
}


template<class T>
int rb_tree<T>::rb_search(long _key, node<T> **result) {
    node<T> *_root = root;
    while (_root != nil && _root->key != _key) {
        if (_root->key > _key)
            _root = _root->left;
        else
            _root = _root->right;
    }
    *result = _root;
    if (_root == nil) {
        return 1;
    } else {
        return 0;
    }
}

#endif

