// Any fixes for this driver should be forwarded to AGEMAME HQ (http://www.mameworld.net/agemame/)

#ifndef BFM_VFD
#define BFM_VFD

#define MAX_VFDS  3	  // max number of vfd displays emulated

#define VFDTYPE_BFMBD1	0 // Bellfruit BD1 display
#define VFDTYPE_MSC1937 1 // OKI MSC1937   display

void	vfd_init(  int id, int type );		// setup a vfd

void	vfd_reset( int id);					// reset the vfd

void	vfd_shift_data(int id, int data);	// clock in a bit of data

int		vfd_newdata(   int id, int data);	// clock in 8 bits of data (scorpion2 vfd interface)

UINT16	*vfd_get_segments(int id);			// get current segments displayed

char	*vfd_get_string( int id);			// get current string   displayed (not as accurate)

void draw_16seg(mame_bitmap *bitmap,int x,int y,int vfd, int col_on, int col_off );

#endif

