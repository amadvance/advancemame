all: pci

pci: pci.cc fbid.h vbeid.h vgaid.h
	g++ -g -Wall -O0 pci.cc -o pci

clean:
	rm -f pci
	rm -f audiocd.d cardcd.d carddos.d cardlinx.d cardwin.d
	rm -f cardcd.lst
