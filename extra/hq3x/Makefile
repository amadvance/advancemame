all: hq2x.dat hq2x3.dat hq2x4.dat hq3x.dat hq4x.dat

hq2x: hq2x.cc common.h
	g++ -O0 -Wall hq2x.cc -o hq2x

hq2x3: hq2x3.cc common.h
	g++ -O0 -Wall hq2x3.cc -o hq2x3

hq2x4: hq2x4.cc common.h
	g++ -O0 -Wall hq2x4.cc -o hq2x4

hq3x: hq3x.cc common.h
	g++ -O0 -Wall hq3x.cc -o hq3x

hq4x: hq4x.cc common.h
	g++ -O0 -Wall hq4x.cc -o hq4x

hq2x.dat: hq2x
	cat lr.txt > hq2x.dat
	cat ld.txt > lq2x.dat
	cat ld.txt > sq2x.dat
	./hq2x 0 >> hq2x.dat
	./hq2x 1 >> lq2x.dat
	./hq2x 2 >> sq2x.dat

hq2x3.dat: hq2x3
	cat ld.txt > hq2x3.dat
	cat ld.txt > lq2x3.dat
	cat ld.txt > sq2x3.dat
	./hq2x3 0 >> hq2x3.dat
	./hq2x3 1 >> lq2x3.dat
	./hq2x3 2 >> sq2x3.dat

hq2x4.dat: hq2x4
	cat ld.txt > hq2x4.dat
	cat ld.txt > lq2x4.dat
	cat ld.txt > sq2x4.dat
	./hq2x4 0 >> hq2x4.dat
	./hq2x4 1 >> lq2x4.dat
	./hq2x4 2 >> sq2x4.dat

hq3x.dat: hq3x
	cat lr.txt > hq3x.dat
	cat ld.txt > lq3x.dat
	cat ld.txt > sq3x.dat
	./hq3x 0 >> hq3x.dat
	./hq3x 1 >> lq3x.dat
	./hq3x 2 >> sq3x.dat

hq4x.dat: hq4x
	cat lr.txt > hq4x.dat
	cat ld.txt > lq4x.dat
	cat ld.txt > sq4x.dat
	./hq4x 0 >> hq4x.dat
	./hq4x 1 >> lq4x.dat
	./hq4x 2 >> sq4x.dat

clean:
	rm -f hq2x hq2x3 hq2x4 hq3x hq4x
	rm -f hq2x.dat hq2x3.dat hq2x4.dat hq3x.dat hq4x.dat
	rm -f lq2x.dat lq2x3.dat lq2x4.dat lq3x.dat lq4x.dat
	rm -f sq2x.dat sq2x3.dat sq2x4.dat sq3x.dat sq4x.dat

