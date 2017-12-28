all: unit_tests

unit_tests: unit_tests.c malloc.c
	gcc -std=gnu11 -Wall -Wextra unit_tests.c malloc.c munit.c -g -o unit_tests

malloc: malloc.c
	gcc -std=gnu11 -Wall -Wextra malloc.c -g -o malloc

clean:
	rm unit_tests malloc