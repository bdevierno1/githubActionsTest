.PHONY: clean build all

# compilation settings
CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic -std=gnu99 -pedantic -Wmissing-prototypes -Wstrict-prototypes -Wold-style-definition -g

all: hostComms

test: 
	hostComms
	./hostComms
 
clean:
	rm -f *.o
	rm -f hostComms 
