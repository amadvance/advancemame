/* Copyright (C) 2002 Charles Sandmann (sandmann@clio.rice.edu)
   ABSOLUTELY NO WARRANTY.  May be redistributed or copied without restriction
   as long as copyright notice kept intact.  Hopefully I'll get this in
   the DJGPP library someday. */

#include <dpmi.h>
#include <sys/farptr.h>
#include <go32.h>
#include "keep.h"

void keep(unsigned char status, unsigned size)
{
  __dpmi_regs regs;

  _farsetsel(_dos_ds);

  /* Keep size default is current PSP block size */
  if(_farnspeekw(_go32_info_block.linear_address_of_original_psp - 15) !=
     _go32_info_block.linear_address_of_original_psp / 16)
    /* Not a real PSP? attempt to continue */
    regs.x.dx = (_go32_info_block.size_of_transfer_buffer + 256) / 16;
  else
    regs.x.dx = _farnspeekw(_go32_info_block.linear_address_of_original_psp - 13);

  /* Default is to keep PSP and transfer buffer, but the user may want to
     not use and release transfer buffer to decrease DOS footprint.  */
  if(size >= 16 && size < regs.x.dx)
    regs.x.dx = size;

  regs.x.ax = 0x3100 + status;
  __dpmi_int(0x21, &regs);
}
