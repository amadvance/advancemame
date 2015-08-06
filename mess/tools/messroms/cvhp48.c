/* this is a quite simple utility for
   converting the hp48 romfile from its format
00000: 0123456789abcdef
into plain binary */

/* params:
   inputstream hp48 romfile ascii
   outputstream hp48 romfile binary
*/

#include <stdio.h>

int main(void)
{
	int addr=0, neu, line, i;
	int v,v1;

	for (line=0; line<0x8000; line++, addr+=0x10) { // hp48s/sx
//	for (line=0; line<0x10000; line++, addr+=0x10) { //hp48g/gx
		if (1!=fscanf(stdin, "%05x:\n", &neu) ) {
			fprintf(stderr, "look into this 20 line source\n");
			exit(1);
		}
		
		if ( neu!=addr ) {
			fprintf(stderr, "%.5x %.5x\n",neu,addr);
			exit(1);
		}
		fprintf(stderr,"%.5x:", addr);
		for (i=0; i<0x8; i++) {
			if (2!=fscanf(stdin,"%1x%1x",&v,&v1)) ;
			v=v|(v1<<4);
			fprintf(stderr,"%02x", v);			
			fwrite(&v, 1, 1, stdout);
		}
		fprintf(stderr, "\n");
	}

	return 0;
}
