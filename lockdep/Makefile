main: main.c lockdep.o

CFLAGS := -O2 -I$(TLIB_PATH) -fsanitize=address

main: main.c lockdep.o
	gcc $(CFLAGS) -lstdc++ -o $@ $^

lockdep.o: lockdep.cc
	g++ -std=c++20 $(CFLAGS) -c -o $@ $^

clean:
	rm -f *.o main

