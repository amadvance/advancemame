#include <stdio.h>
#include <dos.h>
#include <mem.h>
#include <conio.h>
#include <assert.h>
/*
simple dos utility to show the complete character set
of graphics chips

memory modell large required
*/

#define CURRENT_MODE *(char*)MK_FP(0x40, 0x49)
#define COLUMNS *(short*)MK_FP(0x40, 0x4a)
#define PORT *(short*)MK_FP(0x40, 0x63)

// use this to show all 16 character lines of the tandy1000hx
// pc junior graphics adapter
#define TANDY1000HX

int main(void)
{
	char screen[25][40][2];
	int x,y,c,i;

	// cprintf bells

	textmode(BW40);
	cprintf("this screen should work on all\x0d\x0a"
		"\"generic\" msdos compatibles\x0d\x0a");
#ifdef TANDY1000HX
	cprintf("!!next screen might not work on all\x0d\x0a"
		"compatibles\x0d\x0a");
#endif
	gettext(1,1,40,25,screen);
	for (y=5,c=0; y<25; y++) {
		for (x=0; x<40; x++) {
			if (c<256 && x<39 ) {
				screen[y][x][0]=c;
				if (x%3==2) c++;
			} else
				screen[y][x][0]=' ';
//			screen[y][x][1]=0x7;
		}
	}
	puttext(1,1,40,25,screen);
	getch();

#ifdef TANDY1000HX
	textmode(BW40);
	gettext(1,1,40,25,screen);
	for (y=0,c=0; y<13; y++) {
		for (x=0; x<40; x++) {
			if (c<256) {
				screen[y][x][0]=c;
				if (x%2==1) c++;
			} else
				screen[y][x][0]=' ';
//			screen[y][x][1]=0x7;
		}
	}
	puttext(1,1,40,25,screen);

/*
  tandy 1000 hx tv mode (8 lines high chars)

$13 = {crtc = 0x89317a0, mode_control = 40 '(', color_select = 48 '0',
  status = 0 '\000', full_refresh = 0, reg = {index = 0 '\000',
	data = "\000\017", '\000' <repeats 15 times>, "\001\002\003\004\005\006\a\b\t\n\013\f\r\016\017"}, bank = 63 '?', pc_blink = 64, pc_framecnt = 28696,
  displayram = 0x40be1000 "B\aI\aO\aS\a \aR\aO\aM\a \av\ae\ar\as\ai\ao\an\a \a0\a2\a.\a0\a0\a.\a0\a0\a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \aC\ao\am\ap\aa\at\ai\ab\ai\al\ai\at\ay\a \aS\ao\af\at\aw\aa\ar\ae\a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \aC\ao\ap\ay\ar\ai\ag\ah\at\a \a(\aC\a)\a \a1\a9\a8\a4\a,\a1\a"...}


  tandy 1000 hx monochrome mode (9? line high chars)
$14 = {crtc = 0x89317a0, mode_control = 41 ')', color_select = 48 '0',
  status = 0 '\000', full_refresh = 0, reg = {index = 0 '\000',
	data = "\000\017", '\000' <repeats 15 times>, "\a\a\a\a\a\a\a\b\017\017\017\017\017\017\017"}, bank = 63 '?', pc_blink = 64, pc_framecnt = 39181,
  displayram = 0x40be1000 "B\aI\aO\aS\a \aR\aO\aM\a \av\ae\ar\as\ai\ao\an\a \a0\a2\a.\a0\a0\a.\a0\a0\a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \a \aC\ao\am\ap\aa\at\ai\ab\ai\al\ai\at\ay\a \aS\ao\af\at\aw\aa\a"...}


/*
  0x3d4 index
  0x3d5 data
   00 h total             (56)
   01 h display           (40)
   02 h sync              (45)
   03 h sync width        (8)
   04 v total             (31)
   05 v 0..3 total adjust (6)
   06 v rows 0..6         (25)
   07 v sync in rows 0..6 (28)
   09 lines in raw -1     (7)
*/
	assert(PORT==0x3d4);
	outp(PORT, 4); outp(PORT+1,15);
	outp(PORT, 5); outp(PORT+1,14);
	outp(PORT, 6); outp(PORT+1,13);
	outp(PORT, 7); outp(PORT+1,14);
	outp(PORT, 9); outp(PORT+1,15);


	getch();
#endif
	textmode(BW40);


	return 0;
}
