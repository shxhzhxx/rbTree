### rbTree

A thread safety map for UNIX ( C++ ).



#### Why do you need it

This project provide a general way for handling multi-thread data access.

If you are troubled with deadlock , dirty read or crash by null pointer use, then you may find this useful.



#### How do we solved problem

First , let's figure out how these problems occurred.

**If you use some thread-safety map , your code may be as follows**:

* search a node and do something with it
  ```c++
  node=map.get(id);
  node.mutex_lock();  //node may be null here. 
  node.execute();
  node.mutex_unlock();
  ```
  node may be null at second line , when this thread stopped at first line ,and other thread removed the node and released its memory.

* delete a node and release its memory

  ```c++
  node.mutex_lock();
  node.mutex_unlock();
  map.remove(node); 
  delete node;  //node may being used by other thread now.
  ```
  node may being used by other threads at fourth line , when this thread stopped at third line , and other thread get the node.



**What we did is integrate node's lock operation into map:**

* search a node and do something with it

  ```c++
  node=map.get(id);
  node.execute();
  node.mutex_unlock();

  //map's member function
  node map::get(int id){
    rwlock_rdlock();  //read lock for map
    node=search(id);  //get node from map according to id
    node.mutex_lock(); //lock node
    rwlock_unlock();  //unlock map
    return node; 
  }
  ```

* delete a node and release its memory

  ```c++
  map.remove(node);

  //map's member function
  void map::remove(Node node){
    map.rwlock_wrlock();  //write lock for map
    node.mutex_lock();
    node.mutex_unlock();
    map._remove(node);  //remove the node from map
    delete node;
    map.rwlock_unlock();
  }
  ```





**Also , you need to obey some programming manners to make it works.**

* Do not call node.mutex_lock() , ever.  map.get() already locked the node it returns , you need not do it yourself . Once you called node.mutex_unlock()  ( which you should call after you have done your work with this node ) , you shouldn't call node.mutex_lock() again .  
* Do not perform any operations on the node until you get the lock . 
* Call node.mutex_unlock() after you have done your work with this node.
* Finish your work with node and unlock it as soon as possible .  Locked node can block other thread's access to this node , and probably block the whole map .




#### How to use it

* Create your own class that extend **satellite** class.
  ```c++
  class user_data : public satellite {
  public:
    user_data(int _fd = 0): fd(_fd) {};  //Constructor
    ~user_data(){ close(fd); }   //Destructor

    int fd; //file descriptor
  };
  ```

* Use map function for performing insert , remove and search

  ```c++
  satellite *search(long key, bool lock = true);
  int search_try(long key, satellite **result);
  int insert(long key, satellite *data, bool lock = true);
  int insert_try(long key, satellite *data, bool lock = true);
  int remove(long key);
  satellite *get(int index, bool lock = true);
  int size();
  ```

  ​

