all: hq2x.dat hq3x.dat hq4x.dat

hq2x: hq2x.cc common.h
	g++ -O0 -Wall hq2x.cc -o hq2x

hq3x: hq3x.cc common.h
	g++ -O0 -Wall hq3x.cc -o hq3x

hq4x: hq4x.cc common.h
	g++ -O0 -Wall hq4x.cc -o hq4x

hq2x.dat: hq2x
	./hq2x > hq2x.dat
	./hq2x lq > lq2x.dat

hq3x.dat: hq3x
	./hq3x > hq3x.dat
	./hq3x lq > lq3x.dat

hq4x.dat: hq4x
	./hq4x > hq4x.dat
	./hq4x lq > lq4x.dat

clean:
	rm -f hq2x hq3x hq4x
	rm -f hq2x.dat hq3x.dat hq4x.dat
	rm -f lq2x.dat lq3x.dat lq4x.dat
