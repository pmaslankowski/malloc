all: unit_tests malloc

unit_tests: unit_tests.c malloc.c malloc.h  malloc_internals.h
	gcc -std=gnu11 -Wall -Wextra -Wno-missing-braces -pthread unit_tests.c malloc.c munit.c -g -o unit_tests

malloc: malloc.c malloc.h malloc_internals.h
	gcc -c -std=gnu11 -Wall -Wextra -fPIC malloc.c -g 
	gcc -fPIC -shared -pthread -o malloc.so malloc.o

clean:
	rm unit_tests malloc.o malloc.so