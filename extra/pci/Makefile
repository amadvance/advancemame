all: pci pcismall.txt

pci: pci.cc fbid.h vbeid.h vgaid.h alsaid.h
	g++ -g -Wall -O0 pci.cc -o pci

clean:
	rm -f pci
	rm -f audiocd.d cardcd.d carddos.d cardlinx.d cardwin.d
	rm -f cardcd.lst

alsa:
	awk '{ print "{ " $$2 ", " $$3 ", \"\" }, /* " $$1 " */ " } ' < modules.pcimap > alsaid.h.new

pcismall.txt: pcidevs.txt
	grep -E "^(D|V)" pcidevs.txt > pcismall.txt
