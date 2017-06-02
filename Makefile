a.out:demo.o rb_tree.o
	g++ demo.o rb_tree.o -lpthread

demo.o:demo.cpp rb_tree.h
	g++ -c demo.cpp

rb_tree.o:rb_tree.cpp rb_tree.h
	g++ -c rb_tree.cpp

.PHONY:clean
clean:
	rm -f a.out rb_tree.o demo.o