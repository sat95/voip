all : usr1 usr2

usr1 : USR1.c
	gcc $? -lpulse -lpulse-simple -lrt -o $@

usr2 : USR2.c
	gcc $? -lpulse -lpulse-simple -lrt -o $@

clean : 
	rm -f usr1 usr2

