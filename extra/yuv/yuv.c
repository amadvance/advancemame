#include <stdio.h>

void test(int r, int g, int b)
{
	int y, u, v;

	/* MMX implementation */
	y = (76*r + 150*g + 29*b) >> 8;
	u = ((-43*r - 84*g + 128*b) >> 8) + 128;
	v = ((128*r - 107*g - 20*b) >> 8) + 128;

	if (y<0 || y>255 || u<0 || u>255 || v<0 || v>255) {
		printf("MMX error at %d %d %d %d %d %d\n", r, g, b, y, u, v);
	}

	/* C implementation */
/*
      Y =  0.299  R + 0.587  G + 0.114  B
      U = -0.1687 R - 0.3313 G + 0.5    B + 128
      V =  0.5    R - 0.4187 G - 0.0813 B + 128
*/
	y = ((19595*r + 38469*g + 7471*b) >> 16);
	u = ((-11055*r - 21712*g + 32768*b) >> 16) + 128;
	v = ((32768*r - 27439*g - 5328*b) >> 16) + 128;

	if (y<0 || y>255 || u<0 || u>255 || v<0 || v>255) {
		printf("C error at %d %d %d %d %d %d\n", r, g, b, y, u, v);
	}
}

int main() {
	int r, g, b;
	for(r=0;r<256;++r) {
		for(g=0;g<256;++g) {
			for(b=0;b<256;++b) {
				test(r,g,b);
			}
		}
	}
	return 0;
}
