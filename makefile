all: unit_tests

unit_tests: unit_tests.c 
	gcc -std=gnu11 -Wall -Wextra unit_tests.c -g -o unit_tests

clean:
	rm unit_tests