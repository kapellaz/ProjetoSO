all: mobile sys clean

mobile: mobile.o
	gcc -Wall mobile.o -o mobile

mobile.o: mobile.c
	gcc -c mobile.c

sys: systemmanager.o funcoes.o
	gcc -Wall -pthread -D_REENTRANT systemmanager.o funcoes.o -o sys

systemmanager.o: systemmanager.c funcoes.h
	gcc -c systemmanager.c

funcoes.o: funcoes.c funcoes.h
	gcc -c funcoes.c

clean:
	rm *.o

