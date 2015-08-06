/***************************************************************************

  vidhrdw\quasar.c

  Functions to emulate the video hardware of the machine.

  Zaccaria S2650 games share various levels of design with the Century Video
  System (CVS) games, and hence some routines are shared from there.

  Shooting seems to mix custom boards from Zaccaria and sound boards from CVS
  hinting at a strong link between the two companies.

  Zaccaria are an italian company, Century were based in Manchester UK

***************************************************************************/

#include "driver.h"
#include "vidhrdw/s2636.h"
#include "cpu/s2650/s2650.h"

#ifdef LSB_FIRST
#define BL0 0
#define BL1 1
#define BL2 2
#define BL3 3
#define WL0 0
#define WL1 1
#else
#define BL0 3
#define BL1 2
#define BL2 1
#define BL3 0
#define WL0 1
#define WL1 0
#endif

// All defined in vidhrdw/cvs.c

VIDEO_START( cvs );

extern unsigned char *bullet_ram;

extern mame_bitmap *collision_bitmap;
extern mame_bitmap *collision_background;


extern int CollisionRegister;


// Used here
//static int    scroll[8];
//static int    scroll_reg = 0;

unsigned char *effectram;
unsigned char *effectdirty;
int           effectcontrol;

mame_bitmap *effect_bitmap;

PALETTE_INIT( quasar )
{
	int i,col,map;

	#define TOTAL_COLORS(gfxn) (Machine->gfx[gfxn]->total_colors * Machine->gfx[gfxn]->color_granularity)
	#define COLOR(gfxn,offs) (colortable[Machine->drv->gfxdecodeinfo[gfxn].color_codes_start + offs])

	// Standard 1 bit per colour palette (background and sprites)

	for(col = 0;col < 8; col++)
	{
		palette_set_color(col,(col & 1) * 255,((col >> 1) & 1) * 255,((col >> 2) & 1) * 255);
	}

	// Address 0-2 from graphic rom
	//         3-5 from colour ram
	//         6-8 from sprite chips (Used for priority)

	for(col = 0; col < 512; col++)
	{
		COLOR(0,col) = color_prom[col] & 7;
	}

	/* Background for collision */

	COLOR(0,512) = 0;			// Black
	for(col=1;col<8;col++)		// White
		COLOR(0,512+col) = 7;

	/* Effects Colour Map */

	for (i = 0;i < 256;i++)
	{
		int bit0,bit1,bit2,r,g,b;

		/* red component */
		bit0 = BIT(i,0);
		bit1 = BIT(i,1);
		bit2 = BIT(i,2);
		r = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* green component */
		bit0 = BIT(i,3);
		bit1 = BIT(i,4);
		bit2 = BIT(i,5);
		g = 0x21 * bit0 + 0x47 * bit1 + 0x97 * bit2;
		/* blue component */
		bit0 = BIT(i,6);
		bit1 = BIT(i,7);
		b = 0x4f * bit0 + 0xa8 * bit1;

		// Intensity 1
  	    palette_set_color(256+i,r>>2,g>>2,b>>2);

		// Intensity 2
 	    palette_set_color(512+i,(r>>2)+(r>>3),(g>>2)+(g>>3),(b>>2)+(b>>2));

		// Intensity 3
  	    palette_set_color(768+i,r>>1,g>>1,b>>1);
	}

    /* Sprites */

    for(map=0;map<8;map++)
    {
    	COLOR(0,map*2 + 2072) = 0;
    	COLOR(0,map*2 + 2073) = 8 + map;
    }

    /* Set Sprite chip offsets */

	s2636_x_offset = -26;
	s2636_y_offset = 3;

	memset(s2636_1_dirty,0,sizeof(s2636_1_dirty));
	memset(s2636_2_dirty,0,sizeof(s2636_2_dirty));
	memset(s2636_3_dirty,0,sizeof(s2636_3_dirty));
}

VIDEO_START( quasar )
{
	effectram   = auto_malloc(0x400);
	effectdirty = auto_malloc(0x400);

	memset(effectdirty,0,sizeof(effectdirty));

	if ((effect_bitmap = auto_bitmap_alloc(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return video_start_cvs();
}

VIDEO_UPDATE( quasar )
{
	int offs,character;
	int sx,sy, ox, oy;
    int forecolor;

	if (get_vh_global_attribute_changed())
		memset(dirtybuffer, 1, videoram_size);

	/* for every character in the Video RAM, check if it has been modified */
	/* since last time and update it accordingly. */

	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
        character = videoram[offs];

		sx = (offs % 32) * 8;
		sy = (offs / 32) * 8;

		// While we have the current character code, draw the effects layer
		// intensity / on and off controlled by latch

		//if (effectdirty[offs]) doesn't work!
		{
			effectdirty[offs] = 0;

			if (effectcontrol == 0x30)
				forecolor = 0;
			else
				forecolor = effectram[offs] + (256 * (((effectcontrol >> 4) ^ 3) & 3));

			for(ox=0;ox<8;ox++)
				for(oy=0;oy<8;oy++)
					plot_pixel(effect_bitmap, sx+ox, sy+oy, forecolor);
		}

		if(dirtybuffer[offs])
		{
			dirtybuffer[offs] = 0;

            /* Main Screen */

 			drawgfx(tmpbitmap,Machine->gfx[0],
				    character,
					colorram[offs],
				    0,0,
				    sx,sy,
				    0,TRANSPARENCY_NONE,0);


            /* background for Collision Detection (it can only hit certain items) */

            if((colorram[offs] & 7) == 0)
            {
 			    drawgfx(collision_background,Machine->gfx[0],
				        character,
					    64,
				        0,0,
				        sx,sy,
				        0,TRANSPARENCY_NONE,0);
            }
		}
	}

    /* Update screen */

	copybitmap(bitmap,effect_bitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_NONE,0);
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->visible_area,TRANSPARENCY_PEN,0);

    /* 2636's */

	fillbitmap(s2636_1_bitmap,0,0);
	Update_Bitmap(s2636_1_bitmap,s2636_1_ram,s2636_1_dirty,2,collision_bitmap);

	fillbitmap(s2636_2_bitmap,0,0);
	Update_Bitmap(s2636_2_bitmap,s2636_2_ram,s2636_2_dirty,3,collision_bitmap);

	fillbitmap(s2636_3_bitmap,0,0);
	Update_Bitmap(s2636_3_bitmap,s2636_3_ram,s2636_3_dirty,4,collision_bitmap);

    /* Bullet Hardware */

    for (offs = 8; offs < 256; offs++ )
    {
        if(bullet_ram[offs] != 0)
        {
        	int ct;
            for(ct=0;ct<1;ct++)
            {
            	int bx=255-9-bullet_ram[offs]-ct;

            	/* Bullet/Object Collision */

				if (read_pixel(s2636_1_bitmap, bx, offs) != 0) CollisionRegister |= 4;
                if (read_pixel(s2636_3_bitmap, bx, offs) != 0) CollisionRegister |= 8;

	            plot_pixel(bitmap,bx,offs,Machine->pens[7]);
            }
        }
    }

    /* Update 2636 images */

	if (bitmap->depth == 16)
    {
        UINT32 S1,S2,S3,SB,pen;

        for(sx=255;sx>7;sx--)
        {
        	UINT32 *sp1 = (UINT32 *)s2636_1_bitmap->line[sx];
	    	UINT32 *sp2 = (UINT32 *)s2636_2_bitmap->line[sx];
		    UINT32 *sp3 = (UINT32 *)s2636_3_bitmap->line[sx];
	        UINT64 *dst = (UINT64 *)bitmap->line[sx];
		    UINT8  *spb = (UINT8  *)collision_background->line[sx];

            for(offs=0;offs<62;offs++)
            {
        	     S1 = (*sp1++);
                 S2 = (*sp2++);
                 S3 = (*sp3++);

        	     pen = S1 | S2 | S3;

                 if(pen)
                 {
             	    UINT16 *address = (UINT16 *)dst;
				    if (pen & 0xff000000) address[BL3] = Machine->pens[(pen >> 24) & 15];
				    if (pen & 0x00ff0000) address[BL2] = Machine->pens[(pen >> 16) & 15];
				    if (pen & 0x0000ff00) address[BL1] = Machine->pens[(pen >>  8) & 15];
				    if (pen & 0x000000ff) address[BL0] = Machine->pens[(pen & 15)];

                    /* Collision Detection */

                    SB = 0;
				    if (spb[BL3] != Machine->pens[0]) SB =  0x08000000;
				    if (spb[BL2] != Machine->pens[0]) SB |= 0x00080000;
				    if (spb[BL1] != Machine->pens[0]) SB |= 0x00000800;
				    if (spb[BL0] != Machine->pens[0]) SB |= 0x00000008;

                    if (SB)
                    {
    			        if (S1 & SB) CollisionRegister |= 1;
       	                if (S3 & SB) CollisionRegister |= 2;
                    }
                 }

           	     dst++;
                 spb+=4;
            }
        }
    }
    else
	{
        for(sx=255;sx>7;sx--)
        {
	        UINT32 *sp1 = (UINT32 *)s2636_1_bitmap->line[sx];
	        UINT32 *sp2 = (UINT32 *)s2636_2_bitmap->line[sx];
	        UINT32 *sp3 = (UINT32 *)s2636_3_bitmap->line[sx];
            UINT32 *dst = (UINT32 *)bitmap->line[sx];
	        UINT8  *spb = (UINT8  *)collision_background->line[sx];

            UINT32 S1,S2,S3,SB,pen;

            for(offs=0;offs<62;offs++)
            {
        	     S1 = (*sp1++);
                 S2 = (*sp2++);
                 S3 = (*sp3++);

        	     pen = S1 | S2 | S3;

                 if(pen)
                 {
             	    UINT8 *address = (UINT8 *)dst;
				    if (pen & 0xff000000) address[BL3] = Machine->pens[(pen >> 24) & 15];
				    if (pen & 0x00ff0000) address[BL2] = Machine->pens[(pen >> 16) & 15];
				    if (pen & 0x0000ff00) address[BL1] = Machine->pens[(pen >>  8) & 15];
				    if (pen & 0x000000ff) address[BL0] = Machine->pens[(pen & 15)];

                    /* Collision Detection */

                    SB = 0;
				    if (spb[BL3] != Machine->pens[0]) SB =  0x08000000;
				    if (spb[BL2] != Machine->pens[0]) SB |= 0x00080000;
				    if (spb[BL1] != Machine->pens[0]) SB |= 0x00000800;
				    if (spb[BL0] != Machine->pens[0]) SB |= 0x00000008;

                    if (SB)
                    {
    			        if (S1 & SB) CollisionRegister |= 1;
       	                if (S3 & SB) CollisionRegister |= 2;
                    }
                 }

           	     dst++;
                 spb+=4;
            }
        }
    }
}
