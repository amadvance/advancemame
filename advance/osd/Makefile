all: lexyy.c y_tab.c

lexyy.c: parser.l
	flex -olexyy.c parser.l

y_tab.c: parser.y
	bison --no-lines --defines=y_tab.h --output-file=y_tab.c parser.y

clean:
	rm -f lexyy.c y_tab.c y_tab.h
