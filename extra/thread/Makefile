all: thdouble thcreate thmono thfifo

thfifo: thfifo.c test.h
	gcc -g -D_REENTRANT -O3 -Wall thfifo.c -o thfifo -lpthread

thmono: thmono.c test.h
	gcc -g -D_REENTRANT -O3 -Wall thmono.c -o thmono -lpthread

thcreate: thcreate.c test.h
	gcc -g -D_REENTRANT -O3 -Wall thcreate.c -o thcreate -lpthread

thdouble: thdouble.c test.h
	gcc -g -D_REENTRANT -O3 -Wall thdouble.c -o thdouble -lpthread


