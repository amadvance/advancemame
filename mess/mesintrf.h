/*********************************************************************

  mesintrf.h

  MESS supplement to usrintrf.c.

*********************************************************************/

#ifndef MESINTRF_H
#define MESINTRF_H

#include "osdepend.h"
#include "palette.h"
#include "usrintrf.h"

int mess_ui_active(void);
void mess_ui_update(void);

/* image info screen */
int ui_sprintf_image_info(char *buf);
UINT32 ui_menu_image_info(UINT32 state);

/* file manager */
int filemanager(int selected);

/* tape control */
int tapecontrol(int selected);

#endif /* MESINTRF_H */
