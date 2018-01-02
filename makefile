all: unit_tests malloc

functional_tests: functional_tests.c malloc.c malloc.h malloc_constants.h malloc_internals.h check_integrity.c check_integrity.h
	gcc -std=gnu11 -Wall -Wextra -pthread malloc.c functional_tests.c check_integrity.c -o functional_tests

unit_tests: unit_tests.c malloc.c malloc.h malloc_constants.h malloc_internals.h
	gcc -std=gnu11 -Wall -Wextra -pthread unit_tests.c malloc.c munit.c -g -o unit_tests

malloc: malloc.c malloc.h malloc_constants.h malloc_internals.h
	gcc -c -std=gnu11 -Wall -Wextra -fpic malloc.c -g 
	gcc -shared -pthread -o malloc.so malloc.o

clean:
	rm unit_tests