all: hq2x.dat hq3x.dat hq4x.dat

hq2x: hq2x.cc common.h
	g++ -O0 -Wall hq2x.cc -o hq2x

hq3x: hq3x.cc common.h
	g++ -O0 -Wall hq3x.cc -o hq3x

hq4x: hq4x.cc common.h
	g++ -O0 -Wall hq4x.cc -o hq4x

hq2x.dat: hq2x
	./hq2x 0 > hq2x.dat
	./hq2x 1 > lq2x.dat
	./hq2x 2 > sq2x.dat

hq3x.dat: hq3x
	./hq3x 0 > hq3x.dat
	./hq3x 1 > lq3x.dat
	./hq3x 2 > sq3x.dat

hq4x.dat: hq4x
	./hq4x 0 > hq4x.dat
	./hq4x 1 > lq4x.dat
	./hq4x 2 > sq4x.dat

clean:
	rm -f hq2x hq3x hq4x
	rm -f hq2x.dat hq3x.dat hq4x.dat
	rm -f lq2x.dat lq3x.dat lq4x.dat
	rm -f sq2x.dat sq3x.dat sq4x.dat
