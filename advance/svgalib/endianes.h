#ifndef __ENDIANES_H
#define __ENDIANES_H

#ifndef __MSDOS__
#error This module is for MSDOS only
#endif

static __inline__ unsigned LE32(unsigned _val) {
	return _val;
}

static __inline__ unsigned BE32(unsigned _val) {
	return (_val << 24) | ((_val&0xff00) << 8) | ((_val&0xff0000) >> 8) | (_val >> 24);
}

#endif
