<<<<<<< HEAD
<<<<<<< HEAD
CC = $(gcc)

all: usr1 usr2

usr1: USR1.c 
	$(CC) $? -lpulse -lpulse-simple -lrt -o $@

usr2: USR2.c 
	$(CC) $? -lpulse -lpulse-simple -lrt -o $@

clean: 
	rm -rf usr1 usr2
=======
=======
>>>>>>> 00c70e3c68cb82d59c132eb9f730340843ffa405
all : usr1 usr2

usr1 : USR1.c
	gcc $? -lpulse -lpulse-simple -o $@

usr2 : USR2.c
	gcc $? -lpulse -lpulse-simple -o $@

clean : 
<<<<<<< HEAD
	rm -f usr1 usr2
>>>>>>> 00c70e3c68cb82d59c132eb9f730340843ffa405
=======
	rm -f usr1 usr2
>>>>>>> 00c70e3c68cb82d59c132eb9f730340843ffa405
